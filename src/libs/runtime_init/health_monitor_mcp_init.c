//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file health_monitor_mcp_init.c
 * This file contains the config/initialization for Health Monitor, MCP
 */

/*------------- Includes -----------------*/
#include <atu_api.h>
#include <einj.h>
#include <fpfw_init.h>
#include <health_monitor.h>
#include <hm_cli.h>
#include <idhw.h>
#include <idsw.h>
#include <mscp_exp_rmss_memory_map.h>
#include <semaphore_lib.h>

/*-- Symbolic Constant Macros (defines) --*/
// Ascii of 'H''M'
#define HM_SEMAPHORE_KEY(die_id) (((die_id) << 16) | (0x686d & 0xFFFF))

/*-------------- Functions ---------------*/

FPFW_INIT_COMPONENT(hm_svc, FPFW_INIT_DEPENDENCIES("icc_mscp2mscp", "icc_die2die", "atu_svc", "cd_drv"))
{
    uintptr_t mscp_ghes_base = MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(RAS_GHES_TABLE_BLOCK_BASE);
    uintptr_t mscp_ghes_error_record_addr_table_base =
        MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(RAS_ERROR_RECORD_ADDRESS_TABLE_BASE);
    uintptr_t mscp_ghes_ack_addr_table_base =
        MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(RAS_ACK_ADDRESS_TABLE_BASE);
    uintptr_t mscp_ghes_error_record_addr_base =
        MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(RAS_GHES_ERROR_RECORD_BASE);
    uintptr_t einj_payload = MSCP_ATU_AP_WINDOW_ERROR_INJECTION_BASE_ADDR + RAS_EINJ_VENDOR_DEFINED_STRUCT_OFFSET;

    static hm_config_t hm_config = {0};
    hm_config.mscp_ghes_base = (acpi_ghes_t*)mscp_ghes_base;
    hm_config.mscp_ghes_error_record_addr_table_base = (uint32_t*)mscp_ghes_error_record_addr_table_base;
    hm_config.mscp_ghes_ack_addr_table_base = (uint64_t*)mscp_ghes_ack_addr_table_base;
    hm_config.mscp_ghes_error_record_addr_base = (uint32_t*)mscp_ghes_error_record_addr_base;
    hm_config.mscp_ghes_base_apcore_offset = 0;
    hm_config.mscp_error_injection_addr_base = (ras_einj_info_t*)einj_payload;
    hm_config.mscp_hsp_ras_payload_base = (uint8_t*)SCP_EXP_HSP_RAS_PAYLOAD_BASE;
    hm_config.mscp_full_cper_record_base = (uint8_t*)SCP_EXP_MSCP_CPER_REPORT_BASE;

    hm_config.semaphore_id = IS_PLATFORM_SVP() ? SEM_ID_MSCP_EXP_1 : SEM_ID_DIE0_IOSS_1;
    hm_config.semaphore_key = HM_SEMAPHORE_KEY(idsw_get_die_id());
    hm_config.is_primary = (idsw_get_die_id() == DIE_0);
    hm_config.is_mcp = true;

    hm_pre_ddr_init(&hm_config);
    hm_post_ddr_init();
    hm_post_intercore_init(HM_INTERCORE_REMOTE, fpfw_init_get_handle("icc_mscp2mscp"));

    if (!idhw_is_single_die_boot_en())
    {
        hm_post_intercore_init(HM_INTERCORE_LOCAL, fpfw_init_get_handle("icc_die2die"));
    }
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &hm_config};
}

FPFW_INIT_COMPONENT(hm_cli_init, FPFW_INIT_DEPENDENCIES("hm_svc"))
{
    hm_cli_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
