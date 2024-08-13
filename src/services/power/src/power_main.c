//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_main.c
 * Implements the primary driver interface.
 */

/*------------- Includes -----------------*/
#include "power_hw_int_i.h" // for power_init_core, power_init_soc
#include "power_i.h"        // for POWER_LOG_INFO, power_ap_soc_init
#include "power_loops_i.h"
#include "power_runconfig.h"   // for power_service_config_t
#include "power_runconfig_i.h" // for power_runconfig_get_element, power...

#include <DfwkDriver.h>           // for DfwkAsyncRequestComplete, DfwkInte...
#include <DfwkHost.h>             // for DfwkDeviceInitialize
#include <FpFwAssert.h>           // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>            // for FPFW_UNUSED
#include <power_dfwk.h>           // for (anonymous), ppower_service_cli_re...
#include <power_init.h>           // for power_init, power_interface_init
#include <startup_shutdown_ssi.h> // for pssi_startup_notification_request_t
#include <stdbool.h>              // for false
#include <stdint.h>               // for int32_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

static void power_service_dispatch_async(PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_context)
{

    FPFW_UNUSED(p_context);

    switch (p_request->RequestType)
    {
    /* boot and shutdown requests */
    case SSI_STARTUP_STAGE_START_ASYNC: {
        pssi_startup_notification_request_t ssi_request = (pssi_startup_notification_request_t)p_request;
        switch (ssi_request->stage)
        {
        case STARTUP_AP_SOC_POWER_INIT:
            power_ap_soc_init();
            break;
        default:
            // nothing to do for other types.
            break;
        }
        // complete request
        DfwkAsyncRequestComplete(p_request);
    }
    break;
    case SSI_STARTUP_STAGE_COMPLETE_ASYNC: {
        // complete immediately, since nothing to do
        DfwkAsyncRequestComplete(p_request);
    }
    break;
    case SSI_SHUTDOWN_QUIESCE_ASYNC: {
        // complete immediately, since nothing to do
        DfwkAsyncRequestComplete(p_request);
    }
    break;
    case CLI_COMMANDS_POWER_CONFIG:
    case CLI_COMMANDS_POWER_SET:
    case CLI_COMMANDS_POWER_STATUS:
    case CLI_COMMANDS_POWER_LOG:
        power_cli_requests_async_handler(p_request, p_context);
        break;
    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
}

static int32_t power_service_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER p_request)
{
    switch (p_request->RequestType)
    {
    case CLI_COMMANDS_POWER_CONFIG:
    // Placeholder to service `pwr cfg` sync command
    case CLI_COMMANDS_POWER_SET:
    // Placeholder to service `pwr set` sync command
    case CLI_COMMANDS_POWER_STATUS:
    // Placeholder to service `pwr status` sync command
    case CLI_COMMANDS_POWER_LOG:
    // Placeholder to service `pwr log` sync command
    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
    return DFWK_SUCCESS;
}

void power_interface_init(ppower_service_t p_device, ppower_service_interface_t p_interface)
{
    DfwkInterfaceInitialize(&p_interface->header, &p_device->header, &p_device->default_queue, power_service_dispatch_sync);
    p_interface->p_device = p_device;
}

void power_init(ppower_service_t p_device, PDFWK_SCHEDULE p_schedule, const power_service_config_t* p_config)
{
    POWER_LOG_INFO("power_init");
    DfwkDeviceInitialize(&p_device->header, p_schedule);
    DfwkQueueInitialize(&p_device->default_queue, &p_device->header, power_service_dispatch_async, &p_device->header, DfwkQueueType_SerializedDispatch);

    // intialize power service runtime configuration (knobs, fuses, static config, etc)
    power_runconfig_init(p_config);

    // initialize power cap
    power_cap_init();

    // setup control and telemetry loops
    power_loops_init();
}

void power_ap_soc_init()
{
    power_telcfg_t telemetry_config;
    power_telemetry_init_config(&telemetry_config);

    const power_runconfig_t* p_runconfig = power_runconfig_get();
    // SOC portion of init (TOP PVT)
    power_init_soc(p_runconfig);

    // core portion of init
    power_init_core(p_runconfig, &telemetry_config);

    // start loop timers
    power_timer_start_loop_timers();
}