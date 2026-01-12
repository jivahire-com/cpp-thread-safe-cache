/**
 * @file event_trace_relay_quiesce.c
 * Implement the quiesce interfaces for event trace relay
 */

/*-------------------------------- Includes ---------------------------------*/

#include <DfwkClient.h>
#include <DfwkHost.h>
#include <DfwkThreadXHost.h> // for DFWK_THREADX_HOST
#include <accelerator_ip.h>
#include <bug_check.h>
#include <crash_dump_dfwk.h>
#include <et_mts_client.h>
#include <event_trace_relay_events.h>
#include <event_trace_relay_quiesce_i.h>
#include <fpfw_init.h> // for fpfw_init_get_handle
#include <idsw.h>
#include <idsw_kng.h>
#include <startup_shutdown.h> // for sos_register_ssi
#include <stdint.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

#define LOCAL_SCP_QUIESCE_FLAG      0x1
#define LOCAL_SDM_QUIESCE_FLAG      0x2
#define LOCAL_SDM_CDED_QUIESCE_FLAG 0x4
#define REMOTE_MCP_QUIESCE_FLAG     0x8

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

// static data for SSI registration including driver framework objects
static etr_device_t s_etr_device;
static etr_interface_t s_etr_interface;
static startup_ssi_registration_t s_etr_ssi_registration;
static uint8_t s_expected_core_quiesce_flags = 0;
static uint8_t s_current_core_quiesce_flags = 0;
static PDFWK_ASYNC_REQUEST_HEADER p_dfwk_req;

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/

/**
 * @brief Driver framework async dispatch function that is invoked when a request is queued
 *
 * @param request Request from the caller
 * @param context Context passed during initialization
 */
static void event_trace_relay_dfwk_dispatch(PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_context)
{
    FPFW_UNUSED(p_context);

    switch (p_request->RequestType)
    {
    case SSI_QUIESCE_ASYNC:
        if (s_current_core_quiesce_flags == s_expected_core_quiesce_flags)
        {
            DfwkAsyncRequestComplete(p_request);
        }
        else
        {
            p_dfwk_req = p_request;
        }
        break;
    default:
        DfwkAsyncRequestComplete(p_request);
        break;
    }
}

void event_trace_relay_external_core_quiesce_update(mts_platform_core_id_t core_id)
{
    switch (core_id)
    {
    case MTS_PLATFORM_CORE_SDM:
        s_current_core_quiesce_flags |= LOCAL_SDM_QUIESCE_FLAG;
        break;

    case MTS_PLATFORM_CORE_CDED_SDM:
        s_current_core_quiesce_flags |= LOCAL_SDM_CDED_QUIESCE_FLAG;
        break;
    case MTS_PLATFORM_CORE_SCP:
        s_current_core_quiesce_flags |= LOCAL_SCP_QUIESCE_FLAG;
        break;
    case MTS_PLATFORM_CORE_MCP:
        // TODO ADO: 3269256 Need to update based on handling of die-1 and die-0 MCP quiesce
    default:
        break;
    }

    if (s_current_core_quiesce_flags == s_expected_core_quiesce_flags)
    {
        DfwkAsyncRequestComplete(p_dfwk_req);
    }
}

void event_trace_relay_dfwk_init()
{
    DFWK_THREADX_HOST* p_dfwk_host = (DFWK_THREADX_HOST*)fpfw_init_get_handle("dfwk");

    FPFW_ET_LOG_ETR_ASCII_INFO("Creating dfwk ETR Device");
    DfwkDeviceInitialize(&s_etr_device.Header, &p_dfwk_host->Schedule);

    DfwkQueueInitialize(&s_etr_device.Queue, &s_etr_device.Header, event_trace_relay_dfwk_dispatch, &s_etr_device, DfwkQueueType_SerializedDispatch);

    // Initialize interface with device.
    FPFW_ET_LOG_ETR_ASCII_INFO("Creating dfwk ETR Interface");
    s_etr_interface.Device = &s_etr_device;
    DfwkInterfaceInitialize(&s_etr_interface.Header, &s_etr_interface.Device->Header, &s_etr_interface.Device->Queue, NULL); // Synchonous request is not supported.

    FPFW_ET_LOG_ETR_ASCII_INFO("Registering ETR with SSI");
    int32_t status =
        sos_register_ssi(fpfw_init_get_handle("sos_int"), &s_etr_ssi_registration, &s_etr_interface.Header);
    BUG_ASSERT_PARAM(status == FPFW_INIT_STATUS_SUCCESS, status, 0);

    // Check and register accel clients only if not isolated
    if (!accel_is_isolation_enabled(ACCEL_ID_SDM))
    {
        s_expected_core_quiesce_flags |= (LOCAL_SDM_QUIESCE_FLAG);
    }
    if (!accel_is_isolation_enabled(ACCEL_ID_CDED))
    {
        s_expected_core_quiesce_flags |= (LOCAL_SDM_CDED_QUIESCE_FLAG);
    }

    // Setup the list of cores we need to wait for quiesce intimation from
    s_expected_core_quiesce_flags |= LOCAL_SCP_QUIESCE_FLAG;
}
