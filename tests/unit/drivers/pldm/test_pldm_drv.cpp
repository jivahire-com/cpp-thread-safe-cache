//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_pldm_drv.cpp
 *
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...
#include <map>
#include <queue>

extern "C" {
#include <DfwkCommon.h>   // for DfwkAsyncRequestInitialize, DfwkAsyncRe...
#include <DfwkSchedule.h> // for DFWK_SCHEDULE
#include <FpFwUtils.h>    // for FPFW_UNUSED
#include <pldm_drv.h>     // for pldm_request_t, pldm_drv_register_pl...
#include <stdint.h>       // for uint32_t
#include <string.h>       // for NULL, memset, strcmp

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static pldm_device_t* p_pldm_device = NULL;
static pldm_interface_t* p_pldm_interface = NULL;

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_DfwkDeviceInitialize(PDFWK_DEVICE_HEADER Device, PDFWK_SCHEDULE Schedule)
{
    assert_non_null(Device);
    assert_non_null(Schedule);

    function_called();

    p_pldm_device = (pldm_device_t*)Device;
}

static std::map<PDFWK_QUEUE, std::queue<PDFWK_ASYNC_REQUEST_HEADER>> request_queues;
static PDFWK_QUEUE __wrap_dispatch_queue;
static DFWK_ASYNC_REQUEST_DISPATCH __wrap_dispatch_routine;
static void* __wrap_dispatch_ctx;

void __wrap_DfwkQueueInitialize(PDFWK_QUEUE Queue,
                                PDFWK_DEVICE_HEADER Device,
                                DFWK_ASYNC_REQUEST_DISPATCH DispatchRoutine,
                                void* DispatchContext,
                                DFWK_QUEUE_TYPE QueueType)
{
    assert_non_null(Queue);
    assert_non_null(Device);
    assert_non_null(DispatchRoutine);
    assert_non_null(DispatchContext);
    check_expected(QueueType);

    __wrap_dispatch_routine = DispatchRoutine;
    __wrap_dispatch_ctx = DispatchContext;
    request_queues[Queue] = std::queue<PDFWK_ASYNC_REQUEST_HEADER>();
    Queue->OwningDevice = Device;

    function_called();
}

void __wrap_DfwkInterfaceInitialize(PDFWK_INTERFACE_HEADER Interface,
                                    PDFWK_DEVICE_HEADER Device,
                                    PDFWK_QUEUE DispatchQueue,
                                    DFWK_REQUEST_DISPATCH_SYNC DispatchSync)
{
    assert_non_null(Interface);
    assert_non_null(Device);
    assert_non_null(DispatchQueue);
    assert_null(DispatchSync);

    __wrap_dispatch_queue = DispatchQueue;

    function_called();

    p_pldm_interface = (pldm_interface_t*)Interface;
}

int32_t __wrap_DfwkInterfaceOpen(PDFWK_INTERFACE_HEADER Interface, PDFWK_DEVICE_HEADER ClientDevice)
{
    assert_non_null(Interface);
    assert_non_null(ClientDevice);

    Interface->ClientDevice = ClientDevice;
    Interface->Opened = true;
    assert_null(Interface->InterfaceOpen);

    function_called();

    return DFWK_SUCCESS;
}

bool __wrap_DfwkQueueDequeueRequest(PDFWK_QUEUE Queue, PDFWK_ASYNC_REQUEST_HEADER* Request)
{
    assert_non_null(Queue);
    assert_non_null(Request);

    function_called();

    auto& rq = request_queues[Queue];
    if (!rq.empty())
    {
        *Request = rq.front();
        rq.pop();
        return true;
    }

    return false;
}

void __wrap_DfwkQueueEnqueueRequest(PDFWK_QUEUE Queue, PDFWK_ASYNC_REQUEST_HEADER Request)
{
    assert_non_null(Queue);
    assert_non_null(Request);

    function_called();

    if (Queue == __wrap_dispatch_queue)
    {
        // Immediately dispatch
        __wrap_dispatch_routine(Request, __wrap_dispatch_ctx);
    }
    else
    {
        // Enqueue
        request_queues[Queue].push(Request);
    }
}

void __wrap_DfwkAsyncRequestComplete(PDFWK_ASYNC_REQUEST_HEADER Request)
{
    assert_non_null(Request);

    function_called();

    Request->CompletionRoutine(Request, Request->CompletionContext);
}

void __wrap_DfwkAsyncRequestInitialize(PDFWK_ASYNC_REQUEST_HEADER Request, size_t RequestSize)
{
    assert_non_null(Request);
    check_expected(RequestSize);

    function_called();
}

void __wrap_DfwkInterfaceSendAsync(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request)
{
    assert_non_null(Interface);
    assert_non_null(Request);

    function_called();

    __wrap_dispatch_routine(Request, __wrap_dispatch_ctx);
}

void __wrap_DfwkAsyncRequestSetCompletionRoutine(PDFWK_ASYNC_REQUEST_HEADER Request,
                                                 DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
                                                 void* CompletionContext)
{
    assert_non_null(Request);
    assert_non_null(CompletionRoutine);
    assert_null(CompletionContext);

    Request->CompletionContext = CompletionContext;
    Request->CompletionRoutine = CompletionRoutine;

    function_called();
}

static PlatformEventReadyNotificationCb __wrap_ready_CallBack;
static void* __wrap_ready_CallBack_ctx;
fpfw_status_t __wrap_fpfw_pldm_service_register_platform_event_ready_notification(pldm_platform_event_ready_notification* p_notification)
{
    assert_non_null(p_notification);

    __wrap_ready_CallBack = p_notification->CallBack;
    __wrap_ready_CallBack_ctx = p_notification->context;

    function_called();

    return FPFW_STATUS_SUCCESS;
}

static PlatformEventNotificationCb __wrap_pe_CallBack;
static void* __wrap_pe_CallBack_ctx;
fpfw_status_t __wrap_fpfw_pldm_service_raise_platform_event(pldm_platform_event_config_t* p_pe_config,
                                                            pldm_platform_event_notification* p_notification)
{
    assert_non_null(p_pe_config);
    assert_non_null(p_notification);

    __wrap_pe_CallBack = p_notification->CallBack;
    __wrap_pe_CallBack_ctx = p_notification->context;

    function_called();

    return FPFW_STATUS_SUCCESS;
}
} // extern "C"

//
// Tests
//
int test_setup(void** state)
{
    FPFW_UNUSED(state);

    PDFWK_SCHEDULE schedule = (PDFWK_SCHEDULE)0x12345678;

    expect_function_call(__wrap_DfwkDeviceInitialize);
    expect_value(__wrap_DfwkQueueInitialize, QueueType, DfwkQueueType_SerializedDispatch);
    expect_function_call(__wrap_DfwkQueueInitialize);
    expect_value(__wrap_DfwkQueueInitialize, QueueType, DfwkQueueType_ManualDispatch);
    expect_function_call(__wrap_DfwkQueueInitialize);
    expect_value(__wrap_DfwkQueueInitialize, QueueType, DfwkQueueType_ManualDispatch);
    expect_function_call(__wrap_DfwkQueueInitialize);
    pldm_device_initialize(schedule);

    expect_function_call(__wrap_DfwkInterfaceInitialize);
    expect_function_call(__wrap_DfwkInterfaceOpen);
    pldm_interface_initialize();

    expect_function_call(__wrap_fpfw_pldm_service_register_platform_event_ready_notification);
    pldm_driver_initialize();

    // Must have registered ready callback after driver init.
    assert_non_null(__wrap_ready_CallBack);

    return 0;
}

int test_teardown(void** state)
{
    FPFW_UNUSED(state);

    request_queues.clear();

    __wrap_dispatch_queue = NULL;
    __wrap_dispatch_routine = NULL;
    __wrap_dispatch_ctx = NULL;

    __wrap_ready_CallBack = NULL;
    __wrap_ready_CallBack_ctx = NULL;

    __wrap_pe_CallBack = NULL;
    __wrap_pe_CallBack_ctx = NULL;

    memset(p_pldm_device, 0, sizeof(pldm_device_t));
    memset(p_pldm_interface, 0, sizeof(pldm_interface_t));

    return 0;
}

static void pldm_on_ppe_complete(fpfw_pldm_cc_t completionCode, void* context)
{
    FPFW_UNUSED(completionCode);
    FPFW_UNUSED(context);

    function_called();
}

void test_ready_completion_routine(PDFWK_ASYNC_REQUEST_HEADER request, void* context)
{
    FPFW_UNUSED(request);
    FPFW_UNUSED(context);

    function_called();
}

TEST_FUNCTION(test_pldm_drv_init, test_setup, test_teardown)
{
    // Simulate PLDM platform event ready
    expect_function_call(__wrap_DfwkQueueDequeueRequest);
    __wrap_ready_CallBack(0, __wrap_ready_CallBack_ctx);

    pldm_request_t request = {};
    request.header.CompletionRoutine = test_ready_completion_routine;
    request.header.CompletionContext = NULL;

    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(pldm_request_t));
    expect_function_call(__wrap_DfwkAsyncRequestInitialize);
    expect_function_call(__wrap_DfwkInterfaceSendAsync);
    expect_function_call(__wrap_DfwkAsyncRequestComplete);
    expect_function_call(test_ready_completion_routine); // Client callback is expected to be called
    fpfw_status_t status = pldm_drv_register_platform_event_ready_notification(&request);
    assert_true(FPFW_STATUS_SUCCEEDED(status));
}

TEST_FUNCTION(test_pldm_drv_ready_event, test_setup, test_teardown)
{
    pldm_request_t request = {};
    request.header.CompletionRoutine = test_ready_completion_routine;
    request.header.CompletionContext = NULL;

    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(pldm_request_t));
    expect_function_call(__wrap_DfwkAsyncRequestInitialize);
    expect_function_call(__wrap_DfwkInterfaceSendAsync);
    expect_function_call(__wrap_DfwkQueueEnqueueRequest);
    fpfw_status_t status = pldm_drv_register_platform_event_ready_notification(&request);
    assert_true(FPFW_STATUS_SUCCEEDED(status));

    // Platform event ready.
    expect_function_call(__wrap_DfwkQueueDequeueRequest);
    expect_function_call(__wrap_DfwkAsyncRequestComplete);
    expect_function_call(test_ready_completion_routine);  // Client callback is expected to be called
    expect_function_call(__wrap_DfwkQueueDequeueRequest); // Check queue until empty
    __wrap_ready_CallBack(0, __wrap_ready_CallBack_ctx);
}

TEST_FUNCTION(test_pldm_drv_raise_platform_event_before_ready, test_setup, test_teardown)
{
    pldm_request_t request = {};
    uint8_t payload_data[16] = {0};
    fpfw_pmc_platform_event_descriptor_t descriptor = {.event_class = 1,
                                                       .event_payload = payload_data,
                                                       .event_payload_size = sizeof(payload_data)};

    pldm_platform_event_config_t pe_config = {.p_descriptor = &descriptor};
    pldm_platform_event_notification pe_notification = {.CallBack = pldm_on_ppe_complete, .context = NULL};

    fpfw_status_t status = pldm_drv_raise_platform_event(&request, &pe_config, &pe_notification);
    assert_true(status == FPFW_PLDM_SERVICE_E_NOT_READY);
}

TEST_FUNCTION(test_pldm_drv_raise_platform_event, test_setup, test_teardown)
{
    // Platform event ready.
    expect_function_call(__wrap_DfwkQueueDequeueRequest);
    __wrap_ready_CallBack(0, __wrap_ready_CallBack_ctx);

    pldm_request_t request = {};
    uint8_t payload_data[16] = {0};
    fpfw_pmc_platform_event_descriptor_t descriptor = {.event_class = 1,
                                                       .event_payload = payload_data,
                                                       .event_payload_size = sizeof(payload_data)};

    pldm_platform_event_config_t pe_config = {.p_descriptor = &descriptor};
    pldm_platform_event_notification pe_notification = {.CallBack = pldm_on_ppe_complete, .context = NULL};

    expect_value(__wrap_DfwkAsyncRequestInitialize, RequestSize, sizeof(pldm_request_t));
    expect_function_call(__wrap_DfwkAsyncRequestInitialize);
    expect_function_call(__wrap_DfwkAsyncRequestSetCompletionRoutine);
    expect_function_call(__wrap_DfwkInterfaceSendAsync);

    expect_function_call(__wrap_fpfw_pldm_service_raise_platform_event);
    fpfw_status_t status = pldm_drv_raise_platform_event(&request, &pe_config, &pe_notification);
    assert_true(FPFW_STATUS_SUCCEEDED(status));

    // Simulate PLDM platform event completion
    expect_function_call(__wrap_DfwkAsyncRequestComplete);
    expect_function_call(pldm_on_ppe_complete); // Client callback is expected to be called
    expect_function_call(__wrap_DfwkQueueDequeueRequest);
    __wrap_pe_CallBack(FPFW_PLDM_CC_SUCCESS, __wrap_pe_CallBack_ctx);
}