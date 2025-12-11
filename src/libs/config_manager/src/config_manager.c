//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file config_manager.c
 * This file contains the config/initialization for the fpfw configuration manager.
 */

/*------------- Includes -----------------*/
#include <FpFwSpinLock.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <config_manager.h>
#include <config_manager_i.h>
#include <fpfw_cfg_mgr.h>
#include <fpfw_status.h>
#include <icc_platform_defines.h>
#include <idhw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <memory.h>
#include <mpu.h>
#include <mscp_exp_rmss_memory_map.h>
#include <mscp_exp_spi_synchronize_dies.h>
#include <mscp_exp_top_regs.h>
#include <silibs_common.h>
#include <spi_bridge.h>
#include <spi_ctrl_bridge.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <system_info.h>

/*-------------------------------- Typedefs ---------------------------------*/
#define SPI_DATA_ALIGNMENT_SIZE     4
#define SPI_DATA_ALIGN(size)        (FPFW_ALIGN_BY(SPI_DATA_ALIGNMENT_SIZE, size))
#define D2D_MBOX_SPI_CTRL_BASE_ADDR (MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_SPI_CTRL_ADDRESS)
// D2D sync timeout set to 30 seconds;KPI max timeout.
#define CFG_MGR_D2D_SYNC_TIMEOUT (1000 * 30)

/*-- Declarations (Statics and globals) --*/
extern knob_data_t g_knob_data[];
static cached_knob_data_t system_knob_cached[KNOB_MAX] = {0};
static FPFW_SPINLOCK lock;
static uint32_t knob_index = 0;

/*-------------- Functions ---------------*/
void write_override_knob_to_shared_rmss(void* rmss_base_addr, size_t rmss_base_addr_size)
{
    memset(rmss_base_addr, 0, rmss_base_addr_size);

    uint32_t total_override_count = 0;
    uint32_t total_size = sizeof(knob_payload_header_t);

    knob_payload_header_t* knob_payload_start = (knob_payload_header_t*)rmss_base_addr;
    knob_payload_start->signature = CFG_MGR_RELAY_PAYLOAD_SIGNATURE;

    knob_payload_t* current_knob_payload = (knob_payload_t*)((uint8_t*)rmss_base_addr + sizeof(knob_payload_header_t));

    for (uint32_t idx = 0; idx < cached_knob_data_size(); idx++)
    {
        if ((get_cached_knob_data()[idx]).overridden)
        {
            uint32_t knob_data_size = (get_cached_knob_data()[idx]).size;
            uint32_t aligned_size = SPI_DATA_ALIGN(knob_data_size);
            uint32_t required_size = total_size + sizeof(knob_payload_t) + aligned_size;

            BUG_ASSERT_PARAM(required_size <= rmss_base_addr_size, required_size, rmss_base_addr_size);

            current_knob_payload->knob_index = idx;
            current_knob_payload->knob_data_size = knob_data_size;
            uint8_t* current_knob_payload_data = (uint8_t*)current_knob_payload + sizeof(knob_payload_t);

            memcpy(current_knob_payload_data, (get_cached_knob_data()[idx]).data, knob_data_size);

            current_knob_payload = (knob_payload_t*)(current_knob_payload_data + aligned_size);
            total_override_count++;
            total_size = required_size;
        }
    }

    knob_payload_start->total_override_knob_count = total_override_count;

    // Flush the cache for the shared memory region
    SCB_CleanInvalidateDCache_by_Addr((void*)ALIGN_DOWN((uintptr_t)rmss_base_addr, 32), rmss_base_addr_size);
}

void apply_override_knob_from_primary_die(uint32_t rmss_base_addr)
{
    int spi_status = 0;
    uint32_t master_regs = SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SPI_CTRL_ADDRESS;
    uint32_t data = 0;
    uint32_t address = rmss_base_addr;

    // Read the first 4 bytes to get the signature and total override knob count
    spi_status = spi_controller_read_direct_instruction(master_regs, address, 8, &data);
    BUG_ASSERT_PARAM(KNG_SUCCEEDED(spi_status), spi_status, KNG_SUCCESS);

    knob_payload_header_t* knob_payload_start = (knob_payload_header_t*)&data;

    BUG_ASSERT_PARAM(knob_payload_start->signature == CFG_MGR_RELAY_PAYLOAD_SIGNATURE,
                     knob_payload_start->signature,
                     CFG_MGR_RELAY_PAYLOAD_SIGNATURE);

    uint16_t total_override_knob_count = knob_payload_start->total_override_knob_count;

    // Pointer to the first knob payload
    address += sizeof(knob_payload_header_t);

    for (uint32_t current_knob_idx = 0; current_knob_idx < total_override_knob_count; current_knob_idx++)
    {
        // Read the knob payload
        data = 0;
        spi_status = spi_controller_read_direct_instruction(master_regs, address, 8, &data);
        BUG_ASSERT_PARAM(KNG_SUCCEEDED(spi_status), spi_status, KNG_SUCCESS);

        knob_payload_t* current_knob_payload = (knob_payload_t*)&data;
        uint16_t knob_index = current_knob_payload->knob_index;
        uint16_t knob_data_size = current_knob_payload->knob_data_size;

        // Read the knob data
        address += sizeof(knob_payload_t);
        char knob_data[knob_data_size];

        memset(knob_data, 0, knob_data_size);

        for (uint32_t byte_idx = 0; byte_idx < (uint32_t)SPI_DATA_ALIGN(knob_data_size); byte_idx += sizeof(uint32_t))
        {
            spi_status =
                spi_controller_read_direct_instruction(master_regs, address + byte_idx, 8, (uint32_t*)&knob_data[byte_idx]);
            BUG_ASSERT_PARAM(KNG_SUCCEEDED(spi_status), spi_status, KNG_SUCCESS);
        }

        // Override the cached knob data
        memcpy(system_knob_cached[knob_index].data, knob_data, system_knob_cached[knob_index].size);
        system_knob_cached[knob_index].overridden = true;

        // Move to the next knob payload
        address += SPI_DATA_ALIGN(knob_data_size);
    }

    CFG_INFO("Applied override knob value for (%d) knobs\n", total_override_knob_count);
}

fpfw_status_t read_knob_from_default_db_cb(const fpfw_cfg_mgr_guid_t* knob_namespace,
                                           const char* knob_name,
                                           uint8_t* data,
                                           size_t data_size,
                                           void* ctx)
{
    FPFW_UNUSED(ctx);

    if (knob_index >= KNOB_MAX)
    {
        return FPFW_STATUS_NOT_FOUND;
    }

    FPFwSpinLockAcquire(&lock);
    system_knob_cached[knob_index].index = (knob_t)knob_index;
    system_knob_cached[knob_index].knob_namespace = knob_namespace;
    system_knob_cached[knob_index].name = knob_name;
    system_knob_cached[knob_index].data = data;
    system_knob_cached[knob_index].size = data_size;
    system_knob_cached[knob_index].overridden = false;
    knob_index++;
    FPFwSpinLockRelease(&lock);

    return FPFW_STATUS_SUCCESS;
}

bool update_knob_data(cached_knob_data_t* current_entry, const uint8_t* data, size_t data_size, update_knob_completion_routine cb, bool permanent)
{
    // Check we are in mission mode or not
    if ((idsw_get_platform_sdv() == PLATFORM_RVP_EVT_SILICON) && system_info_get_mission_mode())
    {
        CFG_ERR("Knob override not allowed in mission mode\n");
        return false;
    }

    // report something wrong.
    if (data_size != current_entry->size)
    {
        return false;
    }

    // if the data is same as the current data, return true
    if (memcmp(current_entry->data, data, data_size) == 0)
    {
        return true;
    }

    if (permanent)
    {
        if (idsw_get_cpu_type() != CPU_SCP || idsw_get_die_id() != DIE_0 || !system_info_is_hsp_present())
        {
            return false;
        }

        FPFwSpinLockAcquire(&lock);
        memcpy(current_entry->data, data, data_size);
        current_entry->overridden = true;
        FPFwSpinLockRelease(&lock);

        write_knob_to_hsp(current_entry, cb);
    }
    else
    {
        FPFwSpinLockAcquire(&lock);
        memcpy(current_entry->data, data, data_size);
        current_entry->overridden = true;
        FPFwSpinLockRelease(&lock);

        if (cb != NULL)
        {
            cb(current_entry, (uint8_t*)current_entry->data, current_entry->size);
        }
    }

    return true;
}

bool update_knob_in_cached_db_cb(const fpfw_cfg_mgr_guid_t* knob_namespace,
                                 const char* knob_name,
                                 const uint8_t* data,
                                 size_t data_size,
                                 void* ctx)
{
    FPFW_UNUSED(knob_namespace);
    FPFW_UNUSED(ctx);

    for (uint32_t idx = 0; idx < knob_index; idx++)
    {
        cached_knob_data_t* current_entry = &system_knob_cached[idx];

        if (0 == strcmp(current_entry->name, knob_name))
        {
            return update_knob_data(current_entry, data, data_size, NULL, false);
        }
    }

    return true;
}

void apply_override_knob_from_hsp(bool mission_mode)
{
    if (mission_mode)
    {
        CFG_INFO("mission mode is enabled, skipping override knob application");
        return;
    }

    CFG_INFO("Query knob data from HSP");

    for (uint32_t idx = 0; idx < cached_knob_data_size(); idx++)
    {
        if (idx % (cached_knob_data_size() / 10) == 0)
        {
            CFG_INFO("%d%% check completed", (idx * 100) / cached_knob_data_size());
        }

        FPFwSpinLockAcquire(&lock);
        read_knob_from_hsp(&(get_cached_knob_data()[idx]));
        FPFwSpinLockRelease(&lock);
    }

    CFG_INFO("Query knob data from HSP Complete");
}

void cfg_mgr_init(fpfw_cfg_mgr_config_t* cfg_mgr_config, var_service_shared_mem_t* var_svc_mem_ctx)
{
    knob_index = 0;
    FPFwSpinLockInitialize(&lock);

    fpfw_cfg_mgr_init(cfg_mgr_config);

    if ((idsw_get_platform_sdv() == PLATFORM_RVP_EVT_SILICON) && system_info_get_mission_mode())
    {
        cfg_mgr_config->mission_mode = true;
    }
    else
    {
        cfg_mgr_config->mission_mode = false;
    }

    // check all knob data properly loaded
    BUG_ASSERT_PARAM(knob_index == KNOB_MAX, knob_index, KNOB_MAX);

    if (system_info_is_hsp_present())
    {
        if (idsw_get_cpu_type() == CPU_SCP)
        {
            if (idhw_is_single_die_boot_en())
            {
                hsp_variable_service_initialize(var_svc_mem_ctx);
                apply_override_knob_from_hsp(cfg_mgr_config->mission_mode);
            }
            else
            {
                if (idsw_get_die_id() == DIE_0)
                {
                    hsp_variable_service_initialize(var_svc_mem_ctx);
                    apply_override_knob_from_hsp(cfg_mgr_config->mission_mode);
                    write_override_knob_to_shared_rmss((void*)SCP_EXP_SCP_CFGMGR_VARIABLE_SERVICE_PAYLOAD_BASE,
                                                       SCP_EXP_SCP_CFGMGR_VARIABLE_SERVICE_PAYLOAD_SIZE);
                }

                d2d_sync_point.value = RMSS_D2D_CFG_MGR_RELAY_SYNC_POINT;
                int d2d_sync_status =
                    mscp_exp_spi_synchronize_dies_timeout(d2d_sync_point, idsw_get_die_id(), CFG_MGR_D2D_SYNC_TIMEOUT);

                if (d2d_sync_status != SILIBS_SUCCESS)
                {
                    //  reset SPI and retry
                    const uint32_t spi_ctrl_base = SCP_TOP_SCP_EXP_ADDRESS + MSCP_EXP_TOP_SPI_CTRL_ADDRESS;
                    const uint32_t spi_bridge_base = SCP_TOP_SCP_EXP_ADDRESS + MSCP_EXP_TOP_SPI_BRIDGE_ADDRESS;

                    d2d_sync_status = spi_controller_bridge_init(spi_ctrl_base, spi_bridge_base, 20);
                    BUG_ASSERT_PARAM(d2d_sync_status == SILIBS_SUCCESS, d2d_sync_status, SILIBS_SUCCESS);

                    d2d_sync_status =
                        mscp_exp_spi_synchronize_dies_timeout(d2d_sync_point, idsw_get_die_id(), CFG_MGR_D2D_SYNC_TIMEOUT);
                    BUG_ASSERT_PARAM(d2d_sync_status == SILIBS_SUCCESS, d2d_sync_status, SILIBS_SUCCESS);
                }

                if (idsw_get_die_id() != DIE_0)
                {
                    apply_override_knob_from_primary_die(SCP_EXP_SCP_CFGMGR_VARIABLE_SERVICE_PAYLOAD_BASE);
                }
            }

            // Copy all cached knob values, including the ones now that got overridden
            // to the shared RAM for MCP to read.
            size_t knob_values_size = fpfw_cfg_mgr_get_cached_knob_values_size();
            BUG_ASSERT_PARAM(knob_values_size <= SCP_EXP_CONFIG_KNOB_CACHE_SIZE, knob_values_size, SCP_EXP_CONFIG_KNOB_CACHE_SIZE);

            fpfw_status_t status =
                fpfw_cfg_mgr_get_cached_knob_values((void*)SCP_EXP_CONFIG_KNOB_CACHE_BASE, knob_values_size);
            BUG_ASSERT_PARAM(FPFW_STATUS_SUCCEEDED(status), status, knob_values_size);
        }
        else
        {
            // Copy all knob values updated by the SCP into the MCP
            size_t knob_values_size = fpfw_cfg_mgr_get_cached_knob_values_size();
            BUG_ASSERT_PARAM(knob_values_size <= SCP_EXP_CONFIG_KNOB_CACHE_SIZE, knob_values_size, SCP_EXP_CONFIG_KNOB_CACHE_SIZE);

            fpfw_status_t status =
                fpfw_cfg_mgr_set_cached_knob_values((void*)SCP_EXP_CONFIG_KNOB_CACHE_BASE, knob_values_size);
            BUG_ASSERT_PARAM(FPFW_STATUS_SUCCEEDED(status), status, knob_values_size);
        }
    }
}

uint32_t cached_knob_data_size(void)
{
    return knob_index;
}

cached_knob_data_t* get_cached_knob_data(void)
{
    return system_knob_cached;
}