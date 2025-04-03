//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file health_monitor_init.c
 * This file contains the config/initialization for Health Monitor
 */

/*------------- Includes -----------------*/
#include <atu_api.h>
#include <fpfw_init.h>
#include <health_monitor.h>
#include <hm_cli.h>
#include <idhw.h>
#include <idsw.h>
#include <mscp_exp_rmss_memory_map.h> // debug purpose
#include <semaphore_lib.h>

/*-- Symbolic Constant Macros (defines) --*/
// Ascii of 'H''M'
#define HM_SEMAPHORE_KEY(die_id) (((die_id) << 16) | (0x686d & 0xFFFF))

/*-------------- Functions ---------------*/

FPFW_INIT_COMPONENT(hm_svc, FPFW_INIT_DEPENDENCIES("std_io", "hw_ver"))
{
    // TO DO - Define Error Injection Memory Addr on DDR space.
    // https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2438322/
    static ras_einj_info_t_temp einj_payload_temp = {0};

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
    hm_config.mscp_error_injection_addr_base = &einj_payload_temp;

    // To Do
    // SVP Bug on IOSS Semaphore -  https://azurecsi.visualstudio.com/1P-SoC-Modeling/_workitems/edit/2327121
    hm_config.semaphore_id = IS_PLATFORM_SVP() ? SEM_ID_MSCP_EXP_1 : SEM_ID_DIE0_IOSS_1;

    hm_config.semaphore_key = HM_SEMAPHORE_KEY(idsw_get_die_id());
    hm_config.is_primary = (idsw_get_die_id() == DIE_0);

    hm_pre_ddr_init(&hm_config);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &hm_config};
}

FPFW_INIT_COMPONENT(hm_post_init, FPFW_INIT_DEPENDENCIES("hm_svc", "atu_svc", "mesh", "ddr", "gpio_dev", "hw_sem"))
{
    hm_post_ddr_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(hm_scp, FPFW_INIT_DEPENDENCIES("hm_svc", "icc_d2dmbx"))
{
    hm_post_intercore_init(HM_INTERCORE_SCP, fpfw_init_get_handle("icc_d2dmbx"));
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(hm_mcp, FPFW_INIT_DEPENDENCIES("hm_post_init", "icc_mscp2mscp"))
{
    hm_post_intercore_init(HM_INTERCORE_MCP, fpfw_init_get_handle("icc_mscp2mscp"));
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(hm_cli_init, FPFW_INIT_DEPENDENCIES("hm_post_init", "hm_scp"))
{
    hm_cli_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

// Activate Intercore error domain - comment out for now as lack of CODE memory
// ADO - https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2119856/
/*
FPFW_INIT_COMPONENT(hm_svc_ap_init,
                    FPFW_INIT_DEPENDENCIES("hm_post_init", "icc_mscp2apns"))
{
    hm_config_t* hm_config = (hm_config_t*)fpfw_init_get_handle("hm_svc");
    hm_config->icc_apcore = fpfw_init_get_handle("icc_mscp2apns");

    hm_post_intercore_init(HM_INTERCORE_APCORE);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(hm_svc_mcp_init,
                    FPFW_INIT_DEPENDENCIES("hm_post_init", "icc_mscp2mscp"))
{
    hm_config_t* hm_config = (hm_config_t*)fpfw_init_get_handle("hm_svc");
    hm_config->icc_mcp = fpfw_init_get_handle("icc_mscp2mscp");

    hm_post_intercore_init(HM_INTERCORE_MCP);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(hm_svc_hsp_init,
                    FPFW_INIT_DEPENDENCIES("hm_post_init", "icc_hspmbx"))
{
    hm_config_t* hm_config = (hm_config_t*)fpfw_init_get_handle("hm_svc");
    hm_config->icc_hsp = fpfw_init_get_handle("icc_hspmbx");

    hm_post_intercore_init(HM_INTERCORE_HSP);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}*/

FPFW_INIT_COMPONENT(hm_svc_sdm_init, FPFW_INIT_DEPENDENCIES("hm_post_init", "icc_sdm_mbx"))
{
    hm_post_intercore_init(HM_INTERCORE_SDM, fpfw_init_get_handle("icc_sdm_mbx"));
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(hm_svc_cded_init, FPFW_INIT_DEPENDENCIES("hm_post_init", "icc_cded_mbx"))
{
    hm_post_intercore_init(HM_INTERCORE_CDED, fpfw_init_get_handle("icc_cded_mbx"));
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
