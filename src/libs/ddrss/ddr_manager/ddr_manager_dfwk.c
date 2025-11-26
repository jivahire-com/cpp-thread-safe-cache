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
#include <ddr_manager_dfwk.h>
#include <ddr_manager_events.h>
#include <fpfw_init.h>        // for fpfw_init_get_handle
#include <startup_shutdown.h> // for sos_register_ssi
#include <stdint.h>

static ddr_device_t ddr_device;
static ddr_interface_t ddr_interface;
// static data for SSI registration
static startup_ssi_registration_t ddr_ssi_registration;

/*------------- Functions ----------------*/

void ddr_manager_dfwk_dispatch(PDFWK_ASYNC_REQUEST_HEADER request, void* context)
{
    (void)(context);

    DDR_MANAGER_ET_STATUS_PARAM(DDR_MANAGER_ET_TYPE_DFWK_DISPATCH_REQUEST, (int)request->RequestType);

    switch (request->RequestType)
    {
    case SSI_QUIESCE_ASYNC:
        pssi_shutdown_notification_request_t ssi_request = (pssi_shutdown_notification_request_t)request;
        if (ssi_request->shutdown_type != AP_WARM_RESET)
        {
            printf("Goodbye world, DDR Manager received SSI_QUIESCE_ASYNC request\n");
        }
        DfwkAsyncRequestComplete(request);
        break;
    // TODO: Address other request types here as needed
    // https://dev.azure.com/AzureCSI/Dev/_workitems/edit/3160323
    default:
        DfwkAsyncRequestComplete(request);
        break;
    }
}
void ddr_manager_dfwk_init()
{
    DFWK_THREADX_HOST* dfwk_host = (DFWK_THREADX_HOST*)fpfw_init_get_handle("dfwk");

    printf("Creating dfwk DDR Device\r\n");
    DfwkDeviceInitialize(&ddr_device.Header, &dfwk_host->Schedule);

    DfwkQueueInitialize(&ddr_device.Queue, &ddr_device.Header, ddr_manager_dfwk_dispatch, &ddr_device, DfwkQueueType_SerializedDispatch);

    // Initialize interface with device.
    printf("Creating dfwk DDR Interface\r\n");
    ddr_interface.Device = &ddr_device;
    DfwkInterfaceInitialize(&ddr_interface.Header, &ddr_interface.Device->Header, &ddr_interface.Device->Queue, NULL); // Synchonous request is not supported.

    printf("Registering DDR Manager SSI\r\n");
    int32_t status = sos_register_ssi(fpfw_init_get_handle("sos_int"), &ddr_ssi_registration, &ddr_interface.Header);
    BUG_ASSERT_PARAM(status == FPFW_INIT_STATUS_SUCCESS, status, 0);
}
