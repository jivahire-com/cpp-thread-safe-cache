//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_init.c
 * This file contains the config/initialization for the power service
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <DfwkCommon.h>  // for DfwkAsyncRequestInitialize, PDFW...
#include <FpFwUtils.h>   // for FPFW_UNUSED
#include <boot_status.h> // for post_led_status
#include <bug_check.h>
#include <fpfw_init.h> // for fpfw_init_get_handle, FPFW_INIT_C...
#ifdef SCP_RUNTIME_INIT
    #include <ift_fw.h> // for ift_run_tests
#endif
#include <startup_shutdown.h>      // for sos_start_phase, sos_register_ssi
#include <startup_shutdown_init.h> // for sos_interface_init, sos_init, sos...
#include <startup_shutdown_ssi.h>  // for COLD_BOOT, MSCP_SUBSYS_RESET, STA...
#include <stdbool.h>               // for false, true
#include <stdint.h>                // for uintptr_t
#include <stdio.h>                 // for NULL, printf
#include <system_info.h>           // for system_info_is_warm_start
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-- Declarations (Statics and globals) --*/
static boot_status_req_t boot_status_req = {0};
/*-------------- Functions ---------------*/

PLACED_CODE FPFW_INIT_COMPONENT(sos_svc, FPFW_INIT_DEPENDENCIES("dfwk"))
{
    static sos_device_t sos_device;
    sos_init(&sos_device, fpfw_init_get_handle("dfwk"));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &sos_device};
}

PLACED_CODE void boot_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context)
{
    FPFW_UNUSED(p_completion_context);
    FPFW_DBGPRINT_INFO("SOS boot completed (%x)\n", (uintptr_t)request);
    //! Send boot notify message to HSP now
    post_led_status(&boot_status_req, LED_STATUS_CODE_SCP_BOOT_COMPLETE);
}

#ifdef SCP_RUNTIME_INIT

PLACED_CODE void ift_sos_completion_cb(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context)
{
    FPFW_UNUSED(request);
    FPFW_UNUSED(p_completion_context);

    ift_start_tests();
}

PLACED_CODE FPFW_INIT_COMPONENT(
    sos_int,
    FPFW_INIT_DEPENDENCIES("sos_svc", "icc_hspmbx", "icc_die2die", "icc_sdm_mbx", "icc_cded_mbx", "sysinfo", "ift_drv", "boot_stat"))
{
    static sos_interface_t sos_interface;
    sos_interface_init(fpfw_init_get_handle("sos_svc"), &sos_interface, true);

    // create an interface specifically for SSI and register it
    static sos_interface_t ssi_interface;
    sos_interface_init(fpfw_init_get_handle("sos_svc"), &ssi_interface, false);
    // static data for SSI registration
    static startup_ssi_registration_t ssi_registration;
    int32_t status = sos_register_ssi((void*)&sos_interface, &ssi_registration, &ssi_interface.header);
    BUG_ASSERT_PARAM(status == FPFW_INIT_STATUS_SUCCESS, status, 0);

    // initialize the intercore communication
    sos_icc_init(fpfw_init_get_handle("icc_hspmbx"),
                 fpfw_init_get_handle("icc_die2die"),
                 fpfw_init_get_handle("icc_sdm_mbx"),
                 fpfw_init_get_handle("icc_cded_mbx"));

    // send synchronous and asynchronous startup stage start requests
    static startup_start_phase_request_t startup_phase_request;
    DfwkAsyncRequestInitialize((void*)&startup_phase_request.header.async, sizeof(startup_phase_request));

    // Skip the normal boot process if IFT is enabled
    if (!ift_is_enabled())
    {
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
    }
    else
    {
        sos_start_phase((void*)&sos_interface, &startup_phase_request, IFT_BOOT, STARTUP_PHASE_IFT_ASYNC, ift_sos_completion_cb, NULL); // asynchronous
    }

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(&boot_status_req,
                            MSCP_BOOT_STATUS_CODE_SCP_SOS_INIT_END,
                            GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP,
                                                            MSCP_GENERIC,
                                                            ((idsw_get_die_id() == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &sos_interface};
}

#elif MCP_RUNTIME_INIT

FPFW_INIT_COMPONENT(sos_int, FPFW_INIT_DEPENDENCIES("sos_svc", "icc_hspmbx", "boot_stat"))
{
    // Initialize the public SOS Interface
    static sos_interface_t sos_interface;
    sos_interface_init(fpfw_init_get_handle("sos_svc"), &sos_interface, true);

    // Create a private interface specifically for SSI and register it
    static sos_interface_t ssi_interface;
    sos_interface_init(fpfw_init_get_handle("sos_svc"), &ssi_interface, false);

    // initialize the intercore communication
    mcp_sos_icc_init(fpfw_init_get_handle("icc_hspmbx"));

    // Register the against the private interface
    static startup_ssi_registration_t ssi_registration;
    int32_t status = sos_register_ssi((void*)&sos_interface, &ssi_registration, &ssi_interface.header);
    BUG_ASSERT_PARAM(status == FPFW_INIT_STATUS_SUCCESS, status, 0);

    // Build and queue the request for the MCP boot phase, to be handled by the SOS service
    static startup_start_phase_request_t startup_phase_request;
    DfwkAsyncRequestInitialize((void*)&startup_phase_request.header.async, sizeof(startup_phase_request));

    // queue the boot phases for this reset type, handled by the SOS service thread
    sos_start_phase((void*)&sos_interface, NULL, COLD_BOOT, STARTUP_PHASE_MSCP_ASYNC, NULL, NULL);

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(&boot_status_req,
                            MSCP_BOOT_STATUS_CODE_MCP_SOS_INIT_END,
                            GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_MCP,
                                                            MSCP_GENERIC,
                                                            ((idsw_get_die_id() == DIE_0) ? MCP_PRIMARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &sos_interface};
}

#endif