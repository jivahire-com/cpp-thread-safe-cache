//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_main.c
 * Implements the primary driver interface.
 */

/*------------- Includes -----------------*/
#include "crash_dump.h"
#include "power_hw_int_i.h" // for power_init_core, power_init_soc
#include "power_i.h"        // for POWER_LOG_INFO, power_ap_soc_init
#include "power_log.h"      //power log
#include "power_loops_i.h"
#include "power_runconfig.h"   // for power_service_config_t
#include "power_runconfig_i.h" // for power_runconfig_get_element, power...
#include "power_warmstart_i.h" // for power_ws_save_fuse_init

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
static void crash_dump_predump_cb(void* ctx);

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
        case STARTUP_AP_SOC_POWER_INIT_POST_SYNC:
            power_ap_soc_init_post_remote_sync();
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

        pssi_shutdown_notification_request_t ssi_request = (pssi_shutdown_notification_request_t)p_request;

        // TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2002015

        if (ssi_request->shutdown_type != AP_WARM_RESET)
        {
            // Force pmin
            power_hw_force_pmin(PM_FW_PMIN_CONTROL);
        }

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

    const bool full_init = power_hw_full_init_allowed();

    // intialize power service runtime configuration (knobs, fuses, static config, etc)
    power_runconfig_init(p_config);

    // initialize power cap
    power_cap_init();

    if (!full_init)
    {
        power_runconfig_t* p_runconfig = power_runconfig_get();

        // if not a full init, then we need to restore the warm start data
        power_ws_recover_fuse_init(p_runconfig);
        power_knobs_ws_update(&p_runconfig->knobs);
    }

    // setup control and telemetry loops
    power_loops_init();
    // initialize individual power loops
    power_loops_control_init();
    power_loops_telemetry_init();
    power_log_init();
    crash_dump_register_pre_dump_callback(crash_dump_predump_cb, NULL, CRASH_DUMP_TYPE_ALL);
}

void power_ap_soc_init()
{
    const bool full_init = power_hw_full_init_allowed();
    power_telcfg_t telemetry_config;
    power_telemetry_init_config(&telemetry_config);

    const power_runconfig_t* p_runconfig = power_runconfig_get();

    if (full_init)
    {
        // SOC portion of init (TOP PVT)
        power_init_soc(p_runconfig);

        // core portion of init
        power_init_core(p_runconfig, &telemetry_config);

        // once core init is done, store warm start data for future warm starts.
        power_ws_save_fuse_init(p_runconfig);
    }
    else
    {
        power_init_ws_core(p_runconfig, &telemetry_config);
    }

    // secondary init of control loop after core init
    power_loops_control_post_core_init();
}

void power_ap_soc_init_post_remote_sync()
{
    // enable telemetry before we start loops
    power_telemetry_enable();

    // TODO: add warm start state entry code here
    // https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2037273

    // start loop timers
    power_timer_start_loop_timers();
    power_hw_clear_force_pmin(PM_PMIN_ALL);
}

static void crash_dump_predump_cb(void* ctx)
{
    FPFW_UNUSED(ctx);

    // Force Pmin when crash dump begins
    power_hw_force_pmin(PM_FW_PMIN_CONTROL);
}