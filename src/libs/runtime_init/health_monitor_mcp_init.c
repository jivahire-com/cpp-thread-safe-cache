//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file health_monitor_mcp_init.c
 * This file contains the config/initialization for Health Monitor, MCP
 */

/*------------- Includes -----------------*/
#include <atu_api.h>
#include <fpfw_init.h>
#include <health_monitor.h>
#include <idhw.h>
#include <idsw.h>
#include <mscp_exp_rmss_memory_map.h>
#include <semaphore_lib.h>

/*-- Symbolic Constant Macros (defines) --*/
// Ascii of 'H''M'
#define HM_SEMAPHORE_KEY(die_id) (((die_id) << 16) | (0x686d & 0xFFFF))

/*-------------- Functions ---------------*/

FPFW_INIT_COMPONENT(hm_svc, FPFW_INIT_DEPENDENCIES("icc_mscp2mscp", "atu_svc"))
{
    uintptr_t mscp_ghes_base = MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(RAS_GHES_TABLE_BLOCK_BASE);
    uintptr_t mscp_ghes_error_record_addr_table_base =
        MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(RAS_ERROR_RECORD_ADDRESS_TABLE_BASE);
    uintptr_t mscp_ghes_ack_addr_table_base =
        MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(RAS_ACK_ADDRESS_TABLE_BASE);
    uintptr_t mscp_ghes_error_record_addr_base =
        MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(RAS_GHES_ERROR_RECORD_BASE);

    static hm_config_t hm_config = {0};
    hm_config.mscp_ghes_base = (acpi_ghes_t*)mscp_ghes_base;
    hm_config.mscp_ghes_error_record_addr_table_base = (uint32_t*)mscp_ghes_error_record_addr_table_base;
    hm_config.mscp_ghes_ack_addr_table_base = (uint64_t*)mscp_ghes_ack_addr_table_base;
    hm_config.mscp_ghes_error_record_addr_base = (uint32_t*)mscp_ghes_error_record_addr_base;
    hm_config.mscp_ghes_base_apcore_offset = 0;
    hm_config.mscp_full_cper_record_base = (uint8_t*)SCP_EXP_MSCP_CPER_REPORT_BASE;

    hm_config.semaphore_id = IS_PLATFORM_SVP() ? SEM_ID_MSCP_EXP_1 : SEM_ID_DIE0_IOSS_1;
    hm_config.semaphore_key = HM_SEMAPHORE_KEY(idsw_get_die_id());
    hm_config.is_primary = (idsw_get_die_id() == DIE_0);

    hm_pre_ddr_init(&hm_config);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &hm_config};
}
