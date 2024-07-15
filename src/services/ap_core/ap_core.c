//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ap_core.c
 * Implements the APcore driver interface.
 */

/*------------- Includes -----------------*/
#include "ap_core.h"

#include "ap_core_i.h"
#include "ap_core_init.h"

#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <corebits.h>
#include <fpfw_icc_base.h>        // for fpfw_icc_base_send, fpfw_icc_base...
#include <hsp_firmware_headers.h> // for HSP_FIRMWARE_ID
#include <inttypes.h>
#include <startup_shutdown_ssi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static ap_core_service_context_t s_ap_core_ctx = {0};
static fpfw_icc_base_ctx_t* s_icc_base_ctx = NULL;

/*------------- Functions ----------------*/

// dispatcher function to handle set of rvbaraddr
static void ap_core_dispatch_set_rvbaraddr(pap_core_asynchronous_request_t p_request)
{
    APCORE_LOG_INFO("Set RVBARADDR, address %" PRIx32 "%08" PRIx32,
                    (uint32_t)((p_request->data.rvbaraddr) >> 32),
                    (uint32_t)p_request->data.rvbaraddr);

    // call utility function to set all rvbar addresses
    ap_core_util_set_all_rvbaraddr(&s_ap_core_ctx, p_request->data.rvbaraddr);
    // complete immediately, since handling is done
    // TODO: revisit for multi-die
    //       https://azurecsi.visualstudio.com/Dev/_workitems/edit/1869647
    DfwkAsyncRequestComplete(&p_request->header);
}

// dispatcher function to handle core power on
static void ap_core_dispatch_core_power_on(pap_core_asynchronous_request_t p_request)
{
    APCORE_LOG_INFO("Core power on, core ID %u", (unsigned)p_request->data.core_id);

    // power on core
    ap_core_ppu_core_set_power_state(&s_ap_core_ctx, p_request->data.core_id, true);
    // complete p_request; for local cores, there's nothing to wait on
    // TODO: revisit for multi-die
    //       https://azurecsi.visualstudio.com/Dev/_workitems/edit/1869647
    DfwkAsyncRequestComplete(&p_request->header);
}

// dispatcher function to handle core power off
static void ap_core_dispatch_core_power_off(pap_core_asynchronous_request_t p_request)
{
    APCORE_LOG_INFO("Core power off, core ID %u", (unsigned)p_request->data.core_id);

    // power on core
    ap_core_ppu_core_set_power_state(&s_ap_core_ctx, p_request->data.core_id, false);
    // complete p_request; for local cores, there's nothing to wait on
    // TODO: revisit for multi-die
    //       https://azurecsi.visualstudio.com/Dev/_workitems/edit/1869647
    DfwkAsyncRequestComplete(&p_request->header);
}

// dispatcher function to handle boot stage for cluster init
static void ap_core_ssi_start_cluster_init(pssi_startup_notification_request_t p_request)
{
    // turn on cluster PPUs
    APCORE_LOG_INFO("Cluster power on");
    ap_core_ppu_clusters_on(&s_ap_core_ctx);
    // for now, PPU is synchronous
    DfwkAsyncRequestComplete(&p_request->header);
}

// dispatcher function to handle boot stage for primary AP core boot
static void ap_core_ssi_start_primary_ap_core_boot(pssi_startup_notification_request_t p_request)
{
    unsigned int boot_core = ap_core_util_boot_core(&s_ap_core_ctx);
    APCORE_LOG_INFO("Primary AP core power on (%d)", boot_core);
    ap_core_util_set_rvbaraddr(&s_ap_core_ctx, boot_core, s_ap_core_ctx.p_config->boot_core_rvbaraddr);
    ap_core_ppu_core_set_power_state(&s_ap_core_ctx, boot_core, true);
    // for now, PPU is synchronous
    DfwkAsyncRequestComplete(&p_request->header);
}

// asynchronous dispatch function
void ap_core_dispatch(PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_context)
{
    FPFW_UNUSED(p_context);
    s_ap_core_ctx.outstanding_request = (pap_core_asynchronous_request_t)p_request;

    switch (p_request->RequestType)
    {

        /* these are service-specific requests */

    case APCORE_SET_RVBARADDR_ASYNC:
        ap_core_dispatch_set_rvbaraddr((pap_core_asynchronous_request_t)p_request);
        break;
    case APCORE_CORE_POWER_ON_ASYNC:
        ap_core_dispatch_core_power_on((pap_core_asynchronous_request_t)p_request);
        break;
    case APCORE_CORE_POWER_OFF_ASYNC:
        ap_core_dispatch_core_power_off((pap_core_asynchronous_request_t)p_request);
        break;

        /* these are requests used to initiate start phases or a shutdown */

    case SSI_STARTUP_STAGE_START_ASYNC: {
        pssi_startup_notification_request_t ssi_request = (pssi_startup_notification_request_t)p_request;
        switch (ssi_request->stage)
        {
        case STARTUP_CLUSTER_CORE_INIT:
            ap_core_ssi_start_cluster_init(ssi_request);
            break;
        case STARTUP_PRIMARY_AP_CORE_BOOT:
            // turn on primary AP core
            ap_core_ssi_start_primary_ap_core_boot(ssi_request);
            break;
        case STARTUP_BL31_LOAD:
            if (!system_info_is_hsp_present())
            {
                // if no HSP is present, just complete p_request
                DfwkAsyncRequestComplete(p_request);
                break;
            }

            // request TFA firmware load (BL31)
            ap_core_request_load_tfa(s_icc_base_ctx);
            break;
        default:
            // nothing to do for other types.
            // complete p_request
            DfwkAsyncRequestComplete(p_request);
            break;
        }
    }
    break;
    case SSI_STARTUP_STAGE_COMPLETE_ASYNC:
        // complete immediately, since nothing to do
        DfwkAsyncRequestComplete(p_request);
        break;
    case SSI_SHUTDOWN_QUIESCE_ASYNC: {
        pssi_shutdown_notification_request_t ssi_request = (pssi_shutdown_notification_request_t)p_request;
        FPFW_UNUSED(ssi_request)
        // need to implement shutdown cases
        APCORE_LOG_TRACE("SSI shutdown, shutdown type %d", ssi_request->shutdown_type);
        // complete immediately, since nothing to do
        DfwkAsyncRequestComplete(p_request);
    }
    break;
    default:
        FPFW_RUNTIME_ASSERT((false));
        break;
    }
}

// synchronous dispatch function
int32_t ap_core_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER p_request)
{
    switch (p_request->RequestType)
    {

    // service doesn't currently support any synchronous requests
    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
    return DFWK_SUCCESS; // return success
}

void ap_core_interface_init(pap_core_service_t p_device, pap_core_interface_t p_interface)
{
    FPFW_RUNTIME_ASSERT(p_device != NULL);
    FPFW_RUNTIME_ASSERT(p_interface != NULL);

    DfwkInterfaceInitialize(&p_interface->header, &p_device->header, &p_device->default_queue, ap_core_dispatch_sync);
    p_interface->p_device = p_device;
}

void ap_core_init(pap_core_service_t p_device,
                  PDFWK_SCHEDULE p_schedule,
                  fpfw_icc_base_ctx_t* icc_base_ctx,
                  const ap_core_service_config_t* p_config)
{
    FPFW_RUNTIME_ASSERT(p_device != NULL);
    FPFW_RUNTIME_ASSERT(p_schedule != NULL);
    FPFW_RUNTIME_ASSERT(p_config != NULL);

    APCORE_LOG_INFO("Init");
    DfwkDeviceInitialize(&p_device->header, p_schedule);
    DfwkQueueInitialize(&p_device->default_queue, &p_device->header, ap_core_dispatch, &p_device->header, DfwkQueueType_SerializedDispatch);

    // store off device config
    s_ap_core_ctx.p_config = p_config;
    s_icc_base_ctx = icc_base_ctx;

    // read fuses to get enabled cores
    ap_core_util_get_fuse_enabled_cores(&s_ap_core_ctx.enabled_cores);
    // limit enabled cores to those that are actually present
    corebits_and(&s_ap_core_ctx.enabled_cores, p_config->platform_cores_in_die);

    ap_core_ppu_init(&s_ap_core_ctx);
}

pap_core_asynchronous_request_t ap_core_get_outstanding_request()
{
    return s_ap_core_ctx.outstanding_request;
}
