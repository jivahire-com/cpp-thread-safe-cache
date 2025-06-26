//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_pldm.c
 * API implementations for crash dump transfers over PLDM
 */

/*------------- Includes -----------------*/
#include "crash_dump_gpio.h" // for cd_gpio_assert_cd_in_progress
#include "crash_dump_icc.h"
#include "crash_dump_status.h" // for crash_dump_update_core_state

#include <CrashDump.h>
#include <FpFwUtils.h>         // for FPFW_UNUSED
#include <cmsis_m7.h>          // for __WFI
#include <crash_dump.h>        // for crash_dump_handler
#include <crash_dump_events.h> // for CRASH_DUMP_ET
#include <crash_dump_memory.h> //for crashdump memory
#include <fpfw_cfg_mgr.h>      // for knobs
#include <fpfw_pldm_service.h> // pldm service
#include <idsw_kng.h>          // for IS_PLATFORM_SVP
#include <modules/CdDumpManager.h>
#include <modules/CdMemory.h>
#include <nvic.h> // for nvic_get_current_irq
#include <platform_management_component/pldm_oem_event_types.h>
#include <stdint.h> // for uint32_t
/*-- Symbolic Constant Macros (defines) --*/
bool transfer_completed = false;

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void crash_dump_transfer_dump_platform_event_cb(void* pCtx, void* dest, size_t offset, size_t numBytes);
void crash_dump_pldm_on_ppe_complete(fpfw_pldm_cc_t completionCode, void* context);

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void crash_dump_pldm_transfer_dump()
{
    crash_dump_core_t core_id = CRASH_DUMP_CORE_MCP;
    crash_dump_context_t* ctx = crash_dump_context();
    crash_dump_type_context_t* type_context = ctx->type_ctx[CRASH_DUMP_TYPE_FULL];
    if (!crash_dump_is_dump_complete(0, core_id))
    {
        return;
    }
    size_t dump_size = crash_dump_get_dump_size(0, core_id);
    if (dump_size == 0)
    {
        FPFwCDPrintf("No crash dump data to transfer.\n");
        return;
    }
    fpfw_pmc_platform_event_descriptor_t descriptor = {0};
    pldm_platform_event_config_t event = {.p_descriptor = &descriptor};

    descriptor.event_class = FPFW_PLDM_OEM_EVENT_TYPE_CRASH_DUMP;
    descriptor.event_payload = NULL;
    descriptor.event_payload_size = dump_size;
    descriptor.get_event_payload_cb = crash_dump_transfer_dump_platform_event_cb;
    pldm_platform_event_notification notification = {};
    notification.CallBack = crash_dump_pldm_on_ppe_complete;
    fpfw_pldm_service_raise_platform_event(&event, &notification);

    cd_gpio_assert_cd_available(false);
    crash_dump_update_state(type_context, CRASH_DUMP_IN_TRANSFER);
    transfer_completed = false;
}

bool crash_dump_pldm_transfer_completed()
{
    return transfer_completed;
}

void crash_dump_transfer_dump_platform_event_cb(void* pCtx, void* dest, size_t offset, size_t numBytes)
{
    if (!pCtx || !dest)
    {
        FPFwCDPrintf("pCtx or dest is null!\n");
        return;
    }
    crash_dump_context_t* ctx = (crash_dump_context_t*)pCtx;
    crash_dump_type_context_t* type_context = ctx->type_ctx[CRASH_DUMP_TYPE_FULL];
    uintptr_t addr = (uintptr_t)CRASH_DUMP_FULL_MCP_ADDR + (uintptr_t)offset;
    FPFwCDMemcpyGlobalToLocal(&type_context->crash_dump_ctx.memAPIs, dest, (uint64_t)addr, (size_t)numBytes);
}

void crash_dump_pldm_on_ppe_complete(fpfw_pldm_cc_t completionCode, void* context)
{
    crash_dump_context_t* ctx = (crash_dump_context_t*)context;
    crash_dump_type_context_t* type_context = ctx->type_ctx[CRASH_DUMP_TYPE_FULL];
    if (completionCode != FPFW_PLDM_CC_SUCCESS)
    {
        FPFwCDPrintf("Failed to transfer dump to BMC\n");
    }
    transfer_completed = true;
    crash_dump_update_state(type_context, CRASH_DUMP_IN_USE);
    FPFwCDPrintf("Transfer crashdump to BMC sucessfully.\n");
}
