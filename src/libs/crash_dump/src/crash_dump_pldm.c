//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_pldm.c
 * API implementations for crash dump transfers over PLDM
 */

/*------------- Includes -----------------*/
#include "crash_dump_pldm.h"

#include "crash_dump_context.h"
#include "crash_dump_gpio.h" // for cd_gpio_assert_cd_in_progress
#include "crash_dump_icc.h"
#include "crash_dump_status.h" // for crash_dump_update_core_state
#include "crash_dump_stream.h" // for init_crash_dump_stream

#include <CrashDump.h>
#include <FpFwUtils.h>         // for FPFW_UNUSED
#include <cmsis_m7.h>          // for __WFI
#include <crash_dump.h>        // for crash_dump_handler
#include <crash_dump_events.h> // for CRASH_DUMP_ET
#include <crash_dump_memory.h> //for crashdump memory
#include <fpfw_cfg_mgr.h>      // for knobs
#ifndef PLDM_DRV_WORKAROUND
    #include <fpfw_pldm_service.h>
#endif
#include <idsw_kng.h> // for IS_PLATFORM_SVP
#include <modules/CdDumpManager.h>
#include <modules/CdMemory.h>
#include <nvic.h> // for nvic_get_current_irq
#include <platform_management_component/pldm_oem_event_types.h>
#ifdef PLDM_DRV_WORKAROUND
    #include <pldm_drv.h>
#endif
#include <stdint.h> // for uint32_t

/*-- Symbolic Constant Macros (defines) --*/
bool transfer_completed = true; // Flag to indicate if the transfer is completed (not in progress)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

uint32_t crash_dump_pldm_transfer_dump()
{
    static crash_dump_stream_t crash_dump_stream = {0};

    if (transfer_completed == false)
    {
        // Crashdump transfer already in progress
        return KNG_E_BUSY;
    }

    if (!crash_dump_stream_open(&crash_dump_stream))
    {
        transfer_completed = true;
        return KNG_E_NOT_READY;
    }

    if (crash_dump_stream.header_aggregate.FileSize == 0)
    {
        CRASH_DUMP_ET_INFO(CRASH_DUMP_ET_TYPE_PLDM_EMPTY_DUMP);
        crash_dump_stream_close(&crash_dump_stream, false);
        transfer_completed = true;
        return KNG_E_NOT_READY;
    }

    // Initialize the PLDM PE parameters
    static fpfw_pmc_platform_event_descriptor_t descriptor = {
        .event_class = FPFW_PLDM_OEM_EVENT_TYPE_CRASH_DUMP,
        .event_payload = NULL,
        .get_event_payload_context = &crash_dump_stream,
        .get_event_payload_cb = crash_dump_transfer_dump_platform_event_cb,
    };
    descriptor.event_payload_size = crash_dump_stream.header_aggregate.FileSize;

    static pldm_platform_event_config_t event = {.p_descriptor = &descriptor};

    static pldm_platform_event_notification notification = {.CallBack = crash_dump_pldm_on_ppe_complete,
                                                            .context = &crash_dump_stream};

#ifdef PLDM_DRV_WORKAROUND
    static pldm_request_t request = {0};
    fpfw_status_t status = pldm_drv_raise_platform_event(&request, &event, &notification);
#else
    fpfw_status_t status = fpfw_pldm_service_raise_platform_event(&event, &notification);
#endif
    if (FPFW_STATUS_FAILED(status))
    {
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_PLDM_START_TRANSFER_ERROR, status);
        crash_dump_stream_close(&crash_dump_stream, false);
        transfer_completed = true;

        return status;
    }

    // De-assert CD available GPIO
    cd_gpio_assert_cd_available(false);

    // Update crash dump header state to indicate transfer in progress
    crash_dump_context_t* ctx = crash_dump_context();
    crash_dump_type_context_t* type_context = ctx->type_ctx[CRASH_DUMP_TYPE_FULL];
    crash_dump_update_state(type_context, CRASH_DUMP_IN_TRANSFER);

    transfer_completed = false;
    CRASH_DUMP_ET_INFO_PARAM(CRASH_DUMP_ET_TYPE_PLDM_START_TRANSFER,
                             (uint32_t)crash_dump_stream.header_aggregate.FileSize);

    return KNG_SUCCESS;
}

bool crash_dump_pldm_transfer_completed()
{
    return transfer_completed;
}

void crash_dump_transfer_dump_platform_event_cb(void* ctx, void* dest, size_t offset, size_t numBytes)
{
    if (!ctx || !dest)
    {
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_PLDM_TRANSFER_ERROR, KNG_E_INVALIDARG);
        return;
    }

    crash_dump_stream_t* stream = (crash_dump_stream_t*)ctx;

    uint32_t bytes_read = crash_dump_stream_read(stream, (uint8_t*)dest, offset, numBytes);
    uint32_t transfer_percent = ((offset + bytes_read) * 100) / stream->header_aggregate.FileSize;

    if (transfer_percent != stream->transfer_percent)
    {
        CRASH_DUMP_ET_INFO_PARAM(CRASH_DUMP_ET_TYPE_PLDM_TRANSFER, transfer_percent);
    }
    stream->transfer_percent = transfer_percent;
}

void crash_dump_pldm_on_ppe_complete(fpfw_pldm_cc_t completionCode, void* ctx)
{
    crash_dump_stream_t* stream = (crash_dump_stream_t*)ctx;

    // Close the crash dump stream
    crash_dump_stream_close(stream, completionCode == FPFW_PLDM_CC_SUCCESS);

    // Update crash dump header state.
    crash_dump_context_t* cd_ctx = crash_dump_context();
    crash_dump_type_context_t* type_context = cd_ctx->type_ctx[CRASH_DUMP_TYPE_FULL];

    // Reset Crash dump state to In Use, so that next crash dump can be captured.
    crash_dump_update_state(type_context, CRASH_DUMP_IN_USE);

    if (completionCode == FPFW_PLDM_CC_SUCCESS)
    {
        CRASH_DUMP_ET_INFO(CRASH_DUMP_ET_TYPE_PLDM_TRANSFER_COMPLETE);
    }
    else
    {
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_PLDM_TRANSFER_COMPLETE, completionCode);
    }

    transfer_completed = true;
}
