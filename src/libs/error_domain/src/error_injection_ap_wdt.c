//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_injection_ap_wdt.c
 * This file contains ap_wdt ras related functionality.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FPFwInterrupts.h> // for FPFwCoreInterruptRegisterCallback
#include <FpFwAssert.h>     // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>
#include <atu_api.h>
#include <bug_check.h>
#include <cper.h>
#include <crash_dump.h>
#include <error_domain_ap_wdt.h>
#include <error_domain_i.h>
#include <fpfw_init.h>
#include <health_monitor.h>
#include <utils.h>
#include <warm_start.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <ap_top_regs.h>
#include <ap_watchdog_timer_control_registers_regs.h>
#include <scp_top_regs.h>
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------- Function Prototypes -----------*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Functions ---------------*/

PLACED_CODE acpi_einj_cmd_status_t hm_ap_wdt_error_injection_handler(ras_einj_info_t* einj_payload, void* ctx)
{
    FPFW_UNUSED(ctx);

    // Check if einj_payload is NULL
    if (einj_payload == NULL)
    {
        FPFW_DBGPRINT_ERROR("einj_payload is NULL\n");
        return ACPI_EINJ_INVALID_ACCESS;
    }

    // Check input parameters and get the selected injection register
    if (einj_payload->component_group != ACPI_ERROR_DOMAIN_AP_WDT)
    {
        FPFW_DBGPRINT_ERROR("Invalid AP error domain(%d)\n", einj_payload->component_group);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    if (einj_payload->component_group == ACPI_ERROR_DOMAIN_AP_WDT)
    {
        hm_ap_wdt_isr();
    }
    else
    {
        FPFW_DBGPRINT_ERROR("Invalid/Unsupported Error Domain (%d)\n", einj_payload->component_group);
        return ACPI_EINJ_UNKNOWN_FAILURE;
    }

    return ACPI_EINJ_SUCCESS;
}