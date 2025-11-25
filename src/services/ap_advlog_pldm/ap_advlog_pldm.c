//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ap_advlog_pldm.c
 * AP adv logger pldm request processing
 */

/*------------- Includes -----------------*/
#include "ap_advlog_parse.h"

#include <DbgPrint.h>
#include <FpFwAssert.h>
#include <ap_advlog_pldm.h>
#include <bug_check.h>
#include <fpfw_pldm_service.h>
#include <idsw_kng.h>
#include <platform_management_component/pldm_oem_event_types.h>
#include <pldm_common_power.h>
#include <pldm_pdr.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
//! Structure for power cap requests
typedef struct
{
    pldm_state_effecter_context_t effecter_ctx;
} ap_advlog_pldm_request_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static bool logdump_in_progress = false;
static bool effecter_complete_pending = false;
static ap_advlog_pldm_request_t pldm_ctx = {0};

/*-------- PLDM Callbacks -----------*/
static void ap_advlog_pldm_on_ppe_complete(fpfw_pldm_cc_t completionCode, void* ctx)
{
    UNUSED(ctx);

    fpfw_status_t pldm_status;

    if (completionCode == FPFW_PLDM_CC_SUCCESS)
    {
        FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] Done transferring AP advlog dump to BMC!\n");
    }
    else
    {
        FPFW_DBGPRINT_ERROR("[AP_ADVLOG_PLDM] Failed to transfer AP advlog dump to BMC - PLDM CC 0x%x!\n", completionCode);
    }

    if (effecter_complete_pending == true)
    {
        pldm_status = fpfw_pldm_service_state_effecter_set_complete(&pldm_ctx.effecter_ctx);
        if (pldm_status != FPFW_STATUS_SUCCESS)
        {
            FPFW_DBGPRINT_ERROR("[AP_ADVLOG_PLDM] Failed to complete effecter set - PLDM Status 0x%x\n", pldm_status);
        }
        else
        {
            FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] Effecter Set Complete\n");
        }
    }

    logdump_in_progress = false;
}

static void ap_advlog_pldm_event_cb(void* p_ctx, void* dest, size_t offset, size_t num_bytes)
{
    UNUSED(p_ctx);

    uint64_t chunk_src = get_advanced_logger_base() + offset;

    FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] Event callback - num_bytes: 0x%zx\n", num_bytes);

    for (size_t idx = 0; idx < num_bytes; idx++)
    {
        uint8_t byte = MMIO_READ8(chunk_src + idx);
        *((uint8_t*)dest + idx) = byte;
    }

    FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] Event callback Complete\n");
}

void ap_advlog_pldm_transfer_dump()
{
    fpfw_status_t status;

    if (populate_advanced_logger_info() == false)
    {
        FPFW_DBGPRINT_ERROR("[AP_ADVLOG_PLDM] Failed to retrieve log info! Log will not be sent!\n");
        return;
    }

    if (logdump_in_progress == true)
    {
        FPFW_DBGPRINT_ERROR(
            "[AP_ADVLOG_PLDM] Prior AP logdump is in progress, cannot start a new transfer!\n");
        return;
    }

    static fpfw_pmc_platform_event_descriptor_t descriptor = {.event_class = FPFW_PLDM_OEM_EVENT_TYPE_ADV_LOG_DUMP,
                                                              .event_payload = NULL,
                                                              .get_event_payload_cb = ap_advlog_pldm_event_cb,
                                                              .get_event_payload_context = NULL};

    // Update the event payload size dynamically
    descriptor.event_payload_size = get_advanced_logger_size();

    static pldm_platform_event_config_t event = {.p_descriptor = &descriptor};
    static pldm_platform_event_notification notification = {.CallBack = ap_advlog_pldm_on_ppe_complete};

#ifdef PLDM_DRV_WORKAROUND
    static pldm_request_t pldm_request = {0};
    status = pldm_drv_raise_platform_event(&pldm_request, &event, &notification);
#else
    status = fpfw_pldm_service_raise_platform_event(&event, &notification);
#endif

    if (status == FPFW_STATUS_SUCCESS)
    {
        logdump_in_progress = true;
    }
    else
    {
        FPFW_DBGPRINT_ERROR("[AP_ADVLOG_PLDM] Failed to raise PPE! - PLDM Status 0x%x\n", status);
        logdump_in_progress = false;
    }
}

static void ap_advlog_pldm_effecter_set(pldm_state_effecter_context_t* effecter,
                                        fpfw_pldm_composite_value_t value,
                                        uint8_t set_mask,
                                        void* p_context)
{
    UNUSED(effecter);
    UNUSED(value);
    UNUSED(set_mask);
    UNUSED(p_context);

    FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] Effecter set called\n");

    ap_advlog_pldm_transfer_dump();
}

/*-------- Public Functions -----------*/
void ap_advlog_pldm_init()
{
    if ((idsw_get_cpu_type() == CPU_MCP) && (idsw_get_die_id() == DIE_0))
    {
        FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] Initializing AP advlog PLDM service...\n");

        pldm_state_effecter_config_t advlog_effecter_cfg = {.effecter_id = PLDM_EFFECTER_ID_AP_FW_LOGS_STATE_EFFECTER,
                                                            .notifications.on_effecter_set = ap_advlog_pldm_effecter_set,
                                                            .notifications.context = NULL};

        fpfw_status_t status = fpfw_pldm_service_register_state_effecter(&pldm_ctx.effecter_ctx, &advlog_effecter_cfg);

        FPFW_RUNTIME_ASSERT_EXT(FPFW_STATUS_SUCCEEDED(status), status, 0, 0, 0);

        effecter_complete_pending = true;
    }
}