//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_init.c
 * This file contains the config/initialization for the power service
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <DfwkCommon.h>            // for DfwkAsyncRequestInitialize, PDFW...
#include <FpFwAssert.h>            // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>             // for FPFW_UNUSED
#include <boot_status.h>           // for post_led_status
#include <fpfw_init.h>             // for fpfw_init_get_handle, FPFW_INIT_C...
#include <startup_shutdown.h>      // for sos_start_phase, sos_register_ssi
#include <startup_shutdown_init.h> // for sos_interface_init, sos_init, sos...
#include <startup_shutdown_ssi.h>  // for COLD_BOOT, MSCP_SUBSYS_RESET, STA...
#include <stdbool.h>               // for false, true
#include <stdint.h>                // for uintptr_t
#include <stdio.h>                 // for NULL, printf
#include <system_info.h>           // for system_info_is_warm_start

/*-- Symbolic Constant Macros (defines) --*/

/*-- Declarations (Statics and globals) --*/
static boot_status_req_t boot_status_req = {0};
/*-------------- Functions ---------------*/

FPFW_INIT_COMPONENT(sos_svc, FPFW_INIT_DEPENDENCIES("dfwk"))
{
    static sos_device_t sos_device;
    sos_init(&sos_device, fpfw_init_get_handle("dfwk"));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &sos_device};
}

void boot_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context)
{
    FPFW_UNUSED(p_completion_context);
    FPFW_DBGPRINT_INFO("SOS boot completed (%x)\n", (uintptr_t)request);
    //! Send boot notify message to HSP now
    post_led_status(&boot_status_req, LED_STATUS_CODE_SCP_BOOT_COMPLETE);
}

FPFW_INIT_COMPONENT(sos_int, FPFW_INIT_DEPENDENCIES("sos_svc", "icc_hspmbx", "icc_die2die", "sysinfo"))
{
    static sos_interface_t sos_interface;
    sos_interface_init(fpfw_init_get_handle("sos_svc"), &sos_interface, true);

    // create an interface specifically for SSI and register it
    static sos_interface_t ssi_interface;
    sos_interface_init(fpfw_init_get_handle("sos_svc"), &ssi_interface, false);
    // static data for SSI registration
    static startup_ssi_registration_t ssi_registration;
    int32_t status = sos_register_ssi((void*)&sos_interface, &ssi_registration, &ssi_interface.header);
    FPFW_RUNTIME_ASSERT(status == FPFW_INIT_STATUS_SUCCESS);

    // initialize the intercore communication
    sos_icc_init(fpfw_init_get_handle("icc_hspmbx"), fpfw_init_get_handle("icc_die2die"));

    // send synchronous and asynchronous startup stage start requests
    static startup_start_phase_request_t startup_phase_request;
    DfwkAsyncRequestInitialize((void*)&startup_phase_request.header.async, sizeof(startup_phase_request));

    // Determine the boot type based on the system_info_is_warm_start() API
    if (!system_info_is_warm_start())
    {
        // queue the boot phases for this reset type
        sos_start_phase((void*)&sos_interface, NULL, COLD_BOOT, STARTUP_PHASE_MSCP_ASYNC, NULL, NULL); // synchronous
        sos_start_phase((void*)&sos_interface, &startup_phase_request, COLD_BOOT, STARTUP_PHASE_AP_ASYNC, boot_completion, NULL); // asynchronous
    }
    else
    {
        // queue the boot phases for this reset type
        sos_start_phase((void*)&sos_interface, NULL, WARM_BOOT_POST_AP, STARTUP_PHASE_MSCP_ASYNC, NULL, NULL); // synchronous
        sos_start_phase((void*)&sos_interface, &startup_phase_request, WARM_BOOT_POST_AP, STARTUP_PHASE_AP_ASYNC, boot_completion, NULL); // asynchronous
    }
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &sos_interface};
}