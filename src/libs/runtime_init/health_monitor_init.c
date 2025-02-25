//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file health_monitor_init.c
 * This file contains the config/initialization for Health Monitor
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <health_monitor.h>
#include <health_monitor_temporary_ras.h>
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
    // Requirement : mscp_ghes_base (92 byte) * Error Domain Count
    // Requirement : mscp_ghes_error_record_addr_base (580 byte) * Error Domain Count
    // Requirement : mscp_ghes_error_record_addr_table_base (sizeof(uint64_t)) * Error Domain Count
    // Requirement : mscp_ghes_ack_addr_table_base (sizeof(uint64_t)) * Error Domain Count

    /*
    static hm_config_t hm_config = {.mscp_ghes_base_addr = (acpi_ghes_t*)MSCP_ACPI_GHES_BASE,
                                    .mscp_ghes_record_addr_table_base =
    (uint32_t*)MSCP_ACPI_GHES_ERROR_RECORD_ADDRESS_TABLE_BASE, .mscp_ghes_ack_addr_table_base =
    (uint64_t*)MSCP_ACPI_GHES_ACK_ADDRESS_TABLE_BASE, .mscp_ghes_error_record_addr_base =
    (uint32_t*)MSCP_ACPI_GHES_ERROR_RECORD_BASE, .mscp_ghes_base_apcore_offset =
    MSCP_ACPI_GHES_AP_CORE_ADDR_OFFSET, .icc_apcore = NULL, .icc_mscp = NULL, .icc_hsp = NULL, .icc_sdm =
    NULL, .icc_cded = NULL};
    */

    // Temporary initialization will not be functional as local memory is not visible across dies.
    // Temporary initialization until GHES memory block is available on ARSM
    // ToDo - ADO 2347758 : Define RAS related memory block on DIE 0 ARSM
    static acpi_ghes_t ghes_base_temp[ACPI_ERROR_DOMAIN_COUNT] = {0};
    static acpi_ghes_error_record_multi_t ghes_error_record_base_temp[ACPI_ERROR_DOMAIN_COUNT] = {0};
    static uint64_t ghes_error_record_addr_table_base_temp[ACPI_ERROR_DOMAIN_COUNT] = {0};
    static uint64_t ghes_ack_addr_table_base_temp[ACPI_ERROR_DOMAIN_COUNT] = {0};
    static ras_einj_info_t einj_payload_temp = {0};

    static hm_config_t hm_config = {0};

    hm_config.mscp_ghes_base = (acpi_ghes_t*)ghes_base_temp;
    hm_config.mscp_ghes_error_record_addr_table_base = (uint32_t*)ghes_error_record_addr_table_base_temp;
    hm_config.mscp_ghes_ack_addr_table_base = (uint64_t*)ghes_ack_addr_table_base_temp;
    hm_config.mscp_ghes_error_record_addr_base = (uint32_t*)ghes_error_record_base_temp;
    hm_config.mscp_ghes_base_apcore_offset = 0;
    hm_config.mscp_error_injection_addr_base = &einj_payload_temp;
    // Temporary initialization end

    hm_config.semaphore_id = SEM_ID_MSCP_EXP_1;
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

FPFW_INIT_COMPONENT(hm_icc_scp, FPFW_INIT_DEPENDENCIES("hm_svc", "icc_d2dmbx"))
{
    hm_post_intercore_init(HM_INTERCORE_SCP, fpfw_init_get_handle("icc_d2dmbx"));
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(hm_cli_init, FPFW_INIT_DEPENDENCIES("hm_post_init", "hm_icc_scp"))
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
}

FPFW_INIT_COMPONENT(hm_svc_sdm_init,
                    FPFW_INIT_DEPENDENCIES("hm_svc_post_init", "icc_sdm_mbx"))
{
    hm_config_t* hm_config = (hm_config_t*)fpfw_init_get_handle("hm_svc");
    hm_config->icc_apcore = fpfw_init_get_handle("icc_sdm_mbx");

    hm_post_intercore_init(HM_INTERCORE_SDM);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(hm_svc_cded_init,
                    FPFW_INIT_DEPENDENCIES("hm_svc_post_init", "icc_cded_mbx"))
{
    hm_config_t* hm_config = (hm_config_t*)fpfw_init_get_handle("hm_svc");
    hm_config->icc_apcore = fpfw_init_get_handle("icc_cded_mbx");

    hm_post_intercore_init(HM_INTERCORE_CDED);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
*/
