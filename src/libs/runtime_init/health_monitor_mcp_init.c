//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file health_monitor_mcp_init.c
 * This file contains the config/initialization for Health Monitor, MCP
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FpFwUtils.h>
#include <atu_api.h>
#include <bug_check.h>
#include <einj.h>
#include <fpfw_init.h>
#ifndef PLDM_DRV_WORKAROUND
    #include <fpfw_pldm_service.h>
#endif
#include <health_monitor.h>
#include <hm_cli.h>
#include <idhw.h>
#include <idsw.h>
#include <mscp_exp_rmss_memory_map.h>
#ifdef PLDM_DRV_WORKAROUND
    #include <pldm_drv.h>
#endif
#include <semaphore_lib.h>

/*-- Symbolic Constant Macros (defines) --*/
// Ascii of 'H''M'
#define HM_SEMAPHORE_KEY(die_id) (((die_id) << 16) | (0x686d & 0xFFFF))

/*-------------- Functions ---------------*/

FPFW_INIT_COMPONENT(hm_svc, FPFW_INIT_DEPENDENCIES("hw_ver", "icc_mscp2mscp", "icc_die2die", "atu_svc", "cd_drv"))
{
    uintptr_t mscp_ghes_base = MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(RAS_GHES_TABLE_BLOCK_BASE);
    uintptr_t mscp_ghes_error_record_addr_table_base =
        MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(RAS_ERROR_RECORD_ADDRESS_TABLE_BASE);
    uintptr_t mscp_ghes_ack_addr_table_base =
        MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(RAS_ACK_ADDRESS_TABLE_BASE);
    uintptr_t mscp_ghes_error_record_addr_base =
        MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(RAS_GHES_ERROR_RECORD_BASE);
    uintptr_t mscp_full_cper_record_base =
        MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(RAS_LAST_CPER_BASE);

    static hm_config_t hm_config = {0};
    hm_config.mscp_ghes_base = (acpi_ghes_t*)mscp_ghes_base;
    hm_config.mscp_ghes_error_record_addr_table_base = (uint64_t*)mscp_ghes_error_record_addr_table_base;
    hm_config.mscp_ghes_ack_addr_table_base = (uint64_t*)mscp_ghes_ack_addr_table_base;
    hm_config.mscp_ghes_error_record_addr_base = (uint64_t*)mscp_ghes_error_record_addr_base;
    hm_config.mscp_error_injection_addr_base = 0;
    hm_config.mscp_hsp_ras_payload_base = (uint8_t*)SCP_EXP_HSP_RAS_PAYLOAD_BASE;
    hm_config.mscp_full_cper_record_base = (uint8_t*)mscp_full_cper_record_base;

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

#ifdef PLDM_DRV_WORKAROUND
static void pldm_platform_event_ready_callback(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext)
{
    pldm_request_t* pldm_request = (pldm_request_t*)Request;
    FPFW_UNUSED(CompletionContext);

    BUG_ASSERT_PARAM(Request->RequestType == PLDM_GET_READY_ASYNC, Request->RequestType, 0);
    BUG_ASSERT_PARAM(pldm_request->status == FPFW_STATUS_SUCCESS, pldm_request->status, 0);

    hm_set_pldm_ready_status();
}

FPFW_INIT_COMPONENT(hm_pldm, FPFW_INIT_DEPENDENCIES("hm_svc", "pldm_drv"))
{
    if (idsw_get_die_id() == DIE_0)
    {
        static pldm_request_t ready_request = {0};

        DfwkAsyncRequestSetCompletionRoutine(&ready_request.header, pldm_platform_event_ready_callback, NULL);
        fpfw_status_t status = pldm_drv_register_platform_event_ready_notification(&ready_request);
        BUG_ASSERT_PARAM(status == FPFW_STATUS_SUCCESS, status, 0);
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
#else
static void pldm_platform_event_ready_callback(uint16_t event_id, void* context)
{
    FPFW_UNUSED(event_id);
    FPFW_UNUSED(context);

    hm_set_pldm_ready_status();
}

FPFW_INIT_COMPONENT(hm_pldm, FPFW_INIT_DEPENDENCIES("hm_svc", "pldm"))
{
    if (idsw_get_die_id() == DIE_0)
    {
        pldm_platform_event_ready_notification ready_notification = {
            .CallBack = pldm_platform_event_ready_callback,
            .context = NULL,
        };

        fpfw_status_t status = fpfw_pldm_service_register_platform_event_ready_notification(&ready_notification);
        BUG_ASSERT_PARAM(status == FPFW_STATUS_SUCCESS, status, 0);
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
#endif
