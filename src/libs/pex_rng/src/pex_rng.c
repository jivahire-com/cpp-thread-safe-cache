//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file pex_rng.c
 *  This modules initializes and manages the RNG IP
 */

/*------------- Includes -----------------*/
// Standard library headers
#include <DbgPrint.h>
#include <DfwkClient.h>      // for DfwkClientInterfaceOpen
#include <DfwkDriver.h>      // for DfwkInterfaceInitialize, DfwkQueueInitialize
#include <DfwkHost.h>        // for DfwkDeviceInitialize
#include <DfwkThreadXHost.h> // for PDFWK_THREADX_HOST
#include <FpFwAssert.h>      // for FPFW_RUNTIME_ASSERT
#include <core_cluster_top_regs.h>
#include <corebits.h>
#include <dvfs.h>              // for dvfs_get_pex_scp_irq, dvfs_clr_pex_scp_irq
#include <error_domain_pex.h>  // for RECORD_ID_PEX
#include <fpfw_init.h>         // for fpfw_init_get_handle
#include <health_monitor.h>    // for hm_submit_cper
#include <idsw.h>              // for idsw_get_platform_sdv,
#include <idsw_kng.h>          // for PLATFORM_FPGA_LARGE
#include <kng_soc_constants.h> // for NUM_DIE
#include <pex_regs.h>
#include <pex_rng.h>
#include <rng.h>
#define __NO_CSR_TYPEDEFS__
#include <scp_top_regs.h>
#undef __NO_CSR_TYPEDEFS__
#include <stdio.h>
#include <stdlib.h> // for malloc
#include <string.h> // for memset
#include <utils.h>

/*------------- Global Variables -----------------*/
pex_rng_device_t g_pex_device;
pex_rng_interface_t g_pex_interface;
pex_rng_request_t g_pex_request;
/*------------- Functions ----------------*/

// Completion routine for PEX polling async operation
static void pex_polling_completion_routine(PDFWK_ASYNC_REQUEST_HEADER request, void* completion_context)
{
    FPFW_UNUSED(request);
    FPFW_UNUSED(completion_context);

    // Handle completion of PEX polling operation
    FPFW_DBGPRINT_INFO("PEX polling operation completed\n");
}

void pex_async_dispatch(PDFWK_ASYNC_REQUEST_HEADER request, void* context)
{
    pex_rng_config_t* rng_config = (pex_rng_config_t*)context;
    uint32_t pex_base = rng_config->cluster_pex_base;
    switch (request->RequestType)
    {
    case PEX_SEND_CPER:
        // Submit CPER
        acpi_err_sec_firmware_t sec_fw_cper_section = {.severity = ACPI_ERROR_SEVERITY_CORRECTED,
                                                       .record_id = RECORD_ID_PEX,
                                                       .param = {pex_base, KNG_PEX_RNG_ERR, 0}};

        acpi_cper_section_t cper_section;
        cper_section.sec_fw = sec_fw_cper_section;

        hm_submit_cper(ACPI_ERROR_DOMAIN_PEX, ACPI_ERROR_SEVERITY_CORRECTED, &cper_section, sizeof(cper_section));
        DfwkAsyncRequestComplete(request);
        break;
    default:
        break;
    }
}

void init_pex_rng(pex_rng_config_t* rng_config)
{
    FPFW_RUNTIME_ASSERT(rng_config != NULL);

    memset(&g_pex_device, 0, sizeof(pex_rng_device_t));
    memset(&g_pex_interface, 0, sizeof(pex_rng_interface_t));
    memset(&g_pex_request, 0, sizeof(pex_rng_request_t));

    for (unsigned int core = 0; core < rng_config->core_count; ++core)
    {
        const corebits_t* enabled_cores = rng_config->platform_cores_in_die;
        if (!corebits_is_bit_set(enabled_cores, core))
        {
            continue;
        }
        FPFW_DBGPRINT_INFO("Enabling RNG for core %d\n", core);

        const uintptr_t cluster_pex_base_addr = (rng_config->cluster_pex_base + (rng_config->cluster_stride * core));
        uint32_t ap_rng_base = cluster_pex_base_addr + PEX_RNG_ADDRESS;
        // Clk divider initially set to 1 with CPU at 50MHz was too fast for Si TODO: div value still needs to be optimized based on characterization
        rng_enable_r(ap_rng_base, 0xC0);
        FPFW_RUNTIME_ASSERT(rng_wait_for_rng_complete_r(ap_rng_base) == 0x0);
    }
    // Initialize DFWK structures
    DfwkDeviceInitialize(&g_pex_device.header, &((PDFWK_THREADX_HOST)fpfw_init_get_handle("dfwk"))->Schedule);
    DfwkQueueInitialize(&g_pex_device.default_queue, &g_pex_device.header, pex_async_dispatch, &g_pex_device, DfwkQueueType_SerializedDispatch);
    DfwkInterfaceInitialize(&g_pex_interface.header, &g_pex_device.header, &g_pex_device.default_queue, NULL);
    g_pex_interface.pex_device = &g_pex_device;
    DfwkClientInterfaceOpen(&g_pex_interface.header);

    // Initialize the request
    g_pex_request.Header.RequestType = PEX_SEND_CPER; // Example request type
    DfwkAsyncRequestInitialize(&g_pex_request.Header, sizeof(pex_rng_request_t));
}

void schedule_pex_error_handling_dfwk(void* context)
{
    pex_rng_config_t* g_rng_cfg = (pex_rng_config_t*)context;

    // Set up the completion routine for the async operation
    DfwkAsyncRequestSetCompletionRoutine(&g_pex_request.Header,
                                         pex_polling_completion_routine,
                                         g_rng_cfg); // completion context

    FPFW_DBGPRINT_INFO("Scheduling PEX error handling on DFWK thread\n");

    // Send the async request
    DfwkInterfaceSendAsync(&g_pex_interface.header, &g_pex_request.Header);
}

void reset_pex_rng(uintptr_t ap_rng_base)
{
    rng_disable_r(ap_rng_base);
    rng_enable_r(ap_rng_base, 0xC0);
    FPFW_RUNTIME_ASSERT(rng_wait_for_rng_complete_r(ap_rng_base) == 0x0);
}

// Adding dummy implementations for static declarations in rng.h
// TODO: Change the static declarations in rng.h
// ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1917157/?workitem=2063037

static void rng_write_ctrl(uint32_t val)
{
    /* dummy function, do nothing */
    FPFW_UNUSED(val);
}

static uint32_t rng_read_ctrl()
{
    /* dummy function, do nothing */
    return 0;
}

void unused_functions()
{
    FPFW_UNUSED(rng_write_ctrl);
    FPFW_UNUSED(rng_read_ctrl);
}