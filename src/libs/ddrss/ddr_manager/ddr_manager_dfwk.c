//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file ddr_manager_dfwk.c
 * Implement driver framework interface for DDR
 */

/*------------- Includes -----------------*/

#include <DfwkClient.h>
#include <DfwkHost.h>
#include <DfwkThreadXHost.h> // for DFWK_THREADX_HOST
#include <bug_check.h>
#include <crash_dump_dfwk.h>
#include <ddr_manager.h>
#include <ddr_manager_dfwk.h>
#include <ddr_manager_events.h>
#include <ddrss_sdl.h>
#include <fpfw_cfg_mgr.h>
#include <fpfw_init.h>        // for fpfw_init_get_handle
#include <startup_shutdown.h> // for sos_register_ssi
#include <stdint.h>
#include <tx_api.h>

static ddr_device_t ddr_device;
static ddr_interface_t ddr_interface;
// static data for SSI registration
static startup_ssi_registration_t ddr_ssi_registration;

/*------------- Functions ----------------*/

void ddr_manager_dfwk_dispatch(PDFWK_ASYNC_REQUEST_HEADER request, void* context)
{
    (void)(context);
    DDR_MANAGER_ET_STATUS_PARAM(DDR_MANAGER_ET_TYPE_DFWK_DISPATCH_REQUEST, (int)request->RequestType);
    ddr_service_context_t* ddr_service_ctx = ddr_get_service_context();

    switch (request->RequestType)
    {
    case SSI_QUIESCE_ASYNC:
        pssi_shutdown_notification_request_t ssi_request = (pssi_shutdown_notification_request_t)request;

        if (ssi_request->shutdown_type != AP_WARM_RESET)
        {
            // Quiesce DDR timers so no new polling is enqueued
            tx_timer_deactivate(&ddr_service_ctx->ddr_i3c_polling_timer);
            tx_timer_deactivate(&ddr_service_ctx->ecc_ce_polling_timer);
            tx_timer_deactivate(&ddr_service_ctx->rh_tlm_polling_timer);

            // Ask the DDR worker thread to drain pending polling and acknowledge quiesce
            uint32_t msg = DDR_QUIESCE_EVENT;
            if (tx_queue_send(&ddr_service_ctx->work_queue, &msg, TX_NO_WAIT) != TX_SUCCESS)
            {
                DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_QUEUE_SEND_ERROR_START_POLLING_TIMER, 0);
            }
            else
            {
                // Wait briefly for the worker to finish any in-flight I3C and drain its queue
                ULONG wait_ticks = TX_TIMER_TICKS_PER_SECOND / 10; // ~100ms assuming 1kHz tick
                UINT sem_status = tx_semaphore_get(&ddr_service_ctx->quiesce_sem, wait_ticks);
                if (sem_status != TX_SUCCESS)
                {
                    DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_TIMER_CREATE_ERROR, sem_status);
                }
            }
        }

        // Only store SDL on cold shutdown from Die 0 when SDL config knob is enabled
        bool ddr_contents_will_be_lost =
            (ssi_request->shutdown_type != AP_WARM_RESET) && (ssi_request->shutdown_type != MSCP_SUBSYS_RESET);
        bool should_store_sdl =
            ddr_contents_will_be_lost && (idsw_get_die_id() == DIE_0) && config_get_ddrmanager_sdl_en();

        if (should_store_sdl)
        {
            // Copy SDL from memory to flash.  Pass request so it may be completed after sdl var store completes
            store_sdl_var_async(request);
        }
        else
        {
            DfwkAsyncRequestComplete(request);
        }
        break;

    default:
        DfwkAsyncRequestComplete(request);
        break;
    }
}

void ddr_manager_dfwk_init()
{
    DFWK_THREADX_HOST* dfwk_host = (DFWK_THREADX_HOST*)fpfw_init_get_handle("dfwk");

    // Initialize device.
    DfwkDeviceInitialize(&ddr_device.Header, &dfwk_host->Schedule);
    DfwkQueueInitialize(&ddr_device.Queue, &ddr_device.Header, ddr_manager_dfwk_dispatch, &ddr_device, DfwkQueueType_SerializedDispatch);

    // Initialize interface with device.
    ddr_interface.Device = &ddr_device;
    DfwkInterfaceInitialize(&ddr_interface.Header, &ddr_interface.Device->Header, &ddr_interface.Device->Queue, NULL); // Synchonous request is not supported.

    int32_t status = sos_register_ssi(fpfw_init_get_handle("sos_int"), &ddr_ssi_registration, &ddr_interface.Header);
    BUG_ASSERT_PARAM(status == FPFW_INIT_STATUS_SUCCESS, status, 0);
}
