//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file health_monitor_init.c
 * This file contains the config/initialization for Health Monitor
 */

/*------------- Includes -----------------*/
#include <accelerator_ip.h>
#include <accelip_id.h>
#include <atu_api.h>
#include <boot_status.h>
#include <einj.h>
#include <fpfw_init.h>
#include <health_monitor.h>
#include <hm_cli.h>
#include <idhw.h>
#include <idsw.h>
#include <mscp_exp_rmss_memory_map.h>
#include <semaphore_lib.h>
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/
// Ascii of 'H''M'
#define HM_SEMAPHORE_KEY(die_id)               (((die_id) << 16) | (0x686d & 0xFFFF))
#define ARSM_GET_DIE1_NS_REGION_OFFSET(region) (region - D1_MSCP_ARSM_SRAM_NS_BASE)

/*-------------- Functions ---------------*/

PLACED_CODE FPFW_INIT_COMPONENT(hm_svc, FPFW_INIT_DEPENDENCIES("std_io", "hw_ver", "atu_svc"))
{
    uintptr_t mscp_ghes_base =
        MSCP_ATU_AP_WINDOW_ARSM_DIE_1_NS_BASE_ADDR + ARSM_GET_DIE1_NS_REGION_OFFSET(RAS_GHES_TABLE_BLOCK_BASE);
    uintptr_t mscp_ghes_error_record_addr_table_base =
        MSCP_ATU_AP_WINDOW_ARSM_DIE_1_NS_BASE_ADDR + ARSM_GET_DIE1_NS_REGION_OFFSET(RAS_ERROR_RECORD_ADDRESS_TABLE_BASE);
    uintptr_t mscp_ghes_ack_addr_table_base =
        MSCP_ATU_AP_WINDOW_ARSM_DIE_1_NS_BASE_ADDR + ARSM_GET_DIE1_NS_REGION_OFFSET(RAS_ACK_ADDRESS_TABLE_BASE);
    uintptr_t mscp_ghes_error_record_addr_base =
        MSCP_ATU_AP_WINDOW_ARSM_DIE_1_NS_BASE_ADDR + ARSM_GET_DIE1_NS_REGION_OFFSET(RAS_GHES_ERROR_RECORD_BASE);
    uintptr_t mscp_full_cper_record_base =
        MSCP_ATU_AP_WINDOW_ARSM_DIE_1_NS_BASE_ADDR + ARSM_GET_DIE1_NS_REGION_OFFSET(RAS_LAST_CPER_BASE);
    static hm_config_t hm_config = {0};
    hm_config.mscp_ghes_base = (acpi_ghes_t*)mscp_ghes_base;
    hm_config.mscp_ghes_error_record_addr_table_base = (uint64_t*)mscp_ghes_error_record_addr_table_base;
    hm_config.mscp_ghes_ack_addr_table_base = (uint64_t*)mscp_ghes_ack_addr_table_base;
    hm_config.mscp_ghes_error_record_addr_base = (uint64_t*)mscp_ghes_error_record_addr_base;
    hm_config.mscp_error_injection_addr_base = 0;
    hm_config.mscp_hsp_ras_payload_base = (uint8_t*)SCP_EXP_HSP_RAS_PAYLOAD_BASE;
    hm_config.mscp_full_cper_record_base = (uint8_t*)mscp_full_cper_record_base;

    // To Do
    // SVP Bug on IOSS Semaphore -  https://azurecsi.visualstudio.com/1P-SoC-Modeling/_workitems/edit/2327121
    hm_config.semaphore_id = IS_PLATFORM_SVP() ? SEM_ID_MSCP_EXP_1 : SEM_ID_DIE0_IOSS_1;
    hm_config.semaphore_key = HM_SEMAPHORE_KEY(idsw_get_die_id());
    hm_config.is_primary = (idsw_get_die_id() == DIE_0);
    hm_config.is_mcp = false;

    hm_pre_ddr_init(&hm_config);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &hm_config};
}

PLACED_CODE FPFW_INIT_COMPONENT(hm_post_init,
                                FPFW_INIT_DEPENDENCIES("hm_svc", "atu_svc", "mesh_stg_2", "ddr", "gpio_dev", "hw_sem", "cfg_mgr", "boot_stat"))
{
    if (hm_allow_ras_reporting())
    {
        hm_post_ddr_init();
    }

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_HM_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_HM_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

PLACED_CODE FPFW_INIT_COMPONENT(hm_scp, FPFW_INIT_DEPENDENCIES("hm_svc", "icc_die2die"))
{
    if (!idhw_is_single_die_boot_en())
    {
        hm_post_intercore_init(HM_INTERCORE_LOCAL, fpfw_init_get_handle("icc_die2die"));
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

PLACED_CODE FPFW_INIT_COMPONENT(hm_mcp, FPFW_INIT_DEPENDENCIES("hm_post_init", "icc_mscp2mscp"))
{
    hm_post_intercore_init(HM_INTERCORE_REMOTE, fpfw_init_get_handle("icc_mscp2mscp"));
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

PLACED_CODE FPFW_INIT_COMPONENT(hm_sdm, FPFW_INIT_DEPENDENCIES("hm_post_init", "icc_sdm_mbx", "cfg_mgr"))
{
    /* Incase of SDM isolation skip HMM */
    if (!accel_is_isolation_enabled(ACCEL_ID_SDM))
    {
        hm_post_intercore_init(HM_INTERCORE_SDM, fpfw_init_get_handle("icc_sdm_mbx"));
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

PLACED_CODE FPFW_INIT_COMPONENT(hm_cded, FPFW_INIT_DEPENDENCIES("hm_post_init", "icc_cded_mbx", "cfg_mgr"))
{
    /* Incase of CDED isolation skip HMM */
    if (!accel_is_isolation_enabled(ACCEL_ID_CDED))
    {
        hm_post_intercore_init(HM_INTERCORE_CDED, fpfw_init_get_handle("icc_cded_mbx"));
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

PLACED_CODE FPFW_INIT_COMPONENT(hm_hsp, FPFW_INIT_DEPENDENCIES("hm_post_init", "icc_hspmbx"))
{
    hm_post_intercore_init(HM_INTERCORE_HSP, fpfw_init_get_handle("icc_hspmbx"));
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

PLACED_CODE FPFW_INIT_COMPONENT(hm_ap, FPFW_INIT_DEPENDENCIES("hm_post_init", "icc_mscp2apns"))
{
    hm_post_intercore_init(HM_INTERCORE_APCORE, fpfw_init_get_handle("icc_mscp2apns"));
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

PLACED_CODE FPFW_INIT_COMPONENT(hm_cli_init, FPFW_INIT_DEPENDENCIES("hm_post_init", "hm_scp"))
{
    if (hm_allow_ras_reporting())
    {
        hm_cli_init();
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
