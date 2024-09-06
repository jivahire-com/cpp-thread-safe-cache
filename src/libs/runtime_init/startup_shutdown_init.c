//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_init.c
 * This file contains the config/initialization for the power service
 */

/*------------- Includes -----------------*/
#include <DfwkCommon.h>            // for DfwkAsyncRequestInitialize, PDFW...
#include <FpFwAssert.h>            // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>             // for FPFW_UNUSED
#include <fpfw_init.h>             // for fpfw_init_get_handle, FPFW_INIT_C...
#include <startup_shutdown.h>      // for sos_start_phase, sos_register_ssi
#include <startup_shutdown_init.h> // for sos_interface_init, sos_init, sos...
#include <startup_shutdown_ssi.h>  // for COLD_BOOT, MSCP_SUBSYS_RESET, STA...
#include <stdbool.h>               // for false, true
#include <stdint.h>                // for uintptr_t
#include <stdio.h>                 // for NULL, printf

/*-- Symbolic Constant Macros (defines) --*/

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
    printf("SOS boot completed (%x)\n", (uintptr_t)request);
    // could send HSP SCP booted postcode here
}

FPFW_INIT_COMPONENT(sos_int, FPFW_INIT_DEPENDENCIES("sos_svc"))
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

    // send synchronous and asynchronous startup stage start requests
    static startup_start_phase_request_t startup_phase_request;
    DfwkAsyncRequestInitialize((void*)&startup_phase_request.header.async, sizeof(startup_phase_request));
    // queue the boot phases for this reset type - cold boot for now
    sos_start_phase((void*)&sos_interface, NULL, COLD_BOOT, STARTUP_PHASE_MSCP_ASYNC, NULL, NULL); // synchronous
    sos_start_phase((void*)&sos_interface, &startup_phase_request, COLD_BOOT, STARTUP_PHASE_AP_ASYNC, boot_completion, NULL); // asynchronous

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &sos_interface};
}