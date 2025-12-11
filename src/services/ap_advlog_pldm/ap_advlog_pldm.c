//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ap_advlog_pldm.c
 * AP adv logger pldm request processing
 */

/*------------- Includes -----------------*/
#include "ap_advlog_parse.h"
#include "ap_advlog_pldm_events.h"

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
#define AP_ADVLOG_WORKER_THREAD_STACK_SIZE (3 * 1024)
#define AP_ADVLOG_WORKER_THREAD_PRIORITY   (TX_MAX_PRIORITIES - 1) // Lowest priority
#define AP_ADVLOG_EVENT_START_TRANSFER     0x1

/*------------- Typedefs -----------------*/
//! Structure for power cap requests
typedef struct
{
    pldm_state_effecter_context_t effecter_ctx;
} ap_advlog_pldm_request_t;

/*-------- Function Prototypes -----------*/
static void ap_advlog_worker_thread_entry(ULONG thread_input);

/*-- Declarations (Statics and globals) --*/
static bool compress_logs = true;
static bool logdump_in_progress = false;
static bool effecter_complete_pending = false;
static ap_advlog_pldm_request_t pldm_ctx = {0};

// Worker thread for low-priority compression
static TX_THREAD ap_advlog_worker_thread;
static uint8_t ap_advlog_worker_thread_stack[AP_ADVLOG_WORKER_THREAD_STACK_SIZE];
static TX_EVENT_FLAGS_GROUP ap_advlog_event_flags;

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
        AP_ADVLOG_PLDM_ET_ERROR_PARAM(AP_ADVLOG_PLDM_ET_TYPE_DUMP_TRANSFER_FAIL, completionCode);
        FPFW_DBGPRINT_ERROR("[AP_ADVLOG_PLDM] Failed to transfer AP advlog dump to BMC - PLDM CC 0x%x!\n", completionCode);
    }

    if (effecter_complete_pending == true)
    {
        pldm_status = fpfw_pldm_service_state_effecter_set_complete(&pldm_ctx.effecter_ctx);
        if (pldm_status != FPFW_STATUS_SUCCESS)
        {
            AP_ADVLOG_PLDM_ET_ERROR_PARAM(AP_ADVLOG_PLDM_ET_TYPE_EFFECTER_COMPLETE_FAIL, pldm_status);
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
    uint64_t chunk_src;

    if (compress_logs)
    {
        chunk_src = get_advanced_logger_compressed_base() + offset;
    }
    else
    {
        chunk_src = get_advanced_logger_base() + offset;
    }

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
    size_t payload_size;

    if (populate_advanced_logger_info() == false)
    {
        AP_ADVLOG_PLDM_ET_ERROR(AP_ADVLOG_PLDM_ET_TYPE_INFO_RETRIEVE_FAIL);
        FPFW_DBGPRINT_ERROR("[AP_ADVLOG_PLDM] Failed to retrieve log info! Log will not be sent!\n");
        return;
    }

    if (logdump_in_progress == true)
    {
        AP_ADVLOG_PLDM_ET_ERROR(AP_ADVLOG_PLDM_ET_TYPE_LOGDUMP_IN_PROGRESS);
        FPFW_DBGPRINT_ERROR(
            "[AP_ADVLOG_PLDM] Prior AP logdump is in progress, cannot start a new transfer!\n");
        return;
    }

    static fpfw_pmc_platform_event_descriptor_t descriptor = {.event_class = FPFW_PLDM_OEM_EVENT_TYPE_ADV_LOG_DUMP,
                                                              .event_payload = NULL,
                                                              .get_event_payload_cb = ap_advlog_pldm_event_cb,
                                                              .get_event_payload_context = NULL};

    // Set flag to true to attempt compression for each transfer
    compress_logs = true;

    // Update the event payload size dynamically
    status = adv_logger_compress(&payload_size);
    if (status != FPFW_STATUS_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("[AP_ADVLOG_PLDM] Failed to compress advanced logger data for PLDM event\n");
        FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] Proceeding with uncompressed data transfer\n");
        compress_logs = false;
    }

    if (!compress_logs)
    {
        payload_size = get_advanced_logger_size();
    }

    descriptor.event_payload_size = payload_size;

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
        AP_ADVLOG_PLDM_ET_ERROR_PARAM(AP_ADVLOG_PLDM_ET_TYPE_RAISE_PPE_FAIL, status);
        FPFW_DBGPRINT_ERROR("[AP_ADVLOG_PLDM] Failed to raise PPE! - PLDM Status 0x%x\n", status);
        logdump_in_progress = false;
    }
}

static void ap_advlog_worker_thread_entry(ULONG thread_input)
{
    UNUSED(thread_input);
    ULONG actual_flags;

    while (true)
    {
        // Wait for signal to start transfer
        tx_event_flags_get(&ap_advlog_event_flags, AP_ADVLOG_EVENT_START_TRANSFER, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);

        // Perform the compression and transfer on this low-priority thread
        ap_advlog_pldm_transfer_dump();

/* Break out of the loop for unit tests */
#ifdef _WIN32
        break;
#endif
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

    // Signal the worker thread to start transfer instead of doing it directly
    tx_event_flags_set(&ap_advlog_event_flags, AP_ADVLOG_EVENT_START_TRANSFER, TX_OR);
}

/*-------- Public Functions -----------*/
void ap_advlog_pldm_init()
{
    if ((idsw_get_cpu_type() == CPU_MCP) && (idsw_get_die_id() == DIE_0))
    {
        FPFW_DBGPRINT_INFO("[AP_ADVLOG_PLDM] Initializing AP advlog PLDM service...\n");

        // Create event flags for signaling the worker thread
        UINT tx_status = tx_event_flags_create(&ap_advlog_event_flags, "AP_ADVLOG_FLAGS");
        BUG_ASSERT_PARAM(tx_status == TX_SUCCESS, tx_status, 0);

        // Create low-priority worker thread for compression
        tx_status = tx_thread_create(&ap_advlog_worker_thread,
                                     "AP_ADVLOG_WORKER",
                                     ap_advlog_worker_thread_entry,
                                     0,
                                     ap_advlog_worker_thread_stack,
                                     AP_ADVLOG_WORKER_THREAD_STACK_SIZE,
                                     AP_ADVLOG_WORKER_THREAD_PRIORITY,
                                     AP_ADVLOG_WORKER_THREAD_PRIORITY,
                                     TX_NO_TIME_SLICE,
                                     TX_AUTO_START);
        BUG_ASSERT_PARAM(tx_status == TX_SUCCESS, tx_status, 0);

        pldm_state_effecter_config_t advlog_effecter_cfg = {.effecter_id = PLDM_EFFECTER_ID_AP_FW_LOGS_STATE_EFFECTER,
                                                            .notifications.on_effecter_set = ap_advlog_pldm_effecter_set,
                                                            .notifications.context = NULL};

        fpfw_status_t status = fpfw_pldm_service_register_state_effecter(&pldm_ctx.effecter_ctx, &advlog_effecter_cfg);

        BUG_ASSERT_PARAM(FPFW_STATUS_SUCCEEDED(status), status, 0);

        effecter_complete_pending = true;
    }
}