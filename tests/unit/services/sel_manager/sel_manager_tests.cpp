//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_sds_init.cpp
 * Tests the init of the sds service
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...
#include <map>

extern "C" {
#include <FpFwUtils.h>
#ifndef PLDM_DRV_WORKAROUND
    #include <fpfw_pldm_service.h>
#endif
#include <idsw_kng.h>
#include <kng_icc_shared.h>
#ifdef PLDM_DRV_WORKAROUND
    #include <pldm_drv.h>
#endif
#include <sel_i.h>
#include <sel_init.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MAX_SEL_QUEUE_LENGTH 32 // From sel_main.c

#define ICC_MSCP2MSCP_HANDLE     0x12341234
#define ICC_DIE2DIE_HANDLE       0x56785678
#define ICC_MCP2APS_HANDLE       0x9abcdef0
#define DFWK_THREADX_HOST_HANDLE 0x0fedcba9

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_FpFwLockInitialize(PFPFW_LOCK Lock)
{
    assert_non_null(Lock);

    function_called();
}

FPFW_LOCK_STATE __wrap_FpFwLockAcquire(PFPFW_LOCK Lock)
{
    assert_non_null(Lock);

    function_called();

    return mock_type(FPFW_LOCK_STATE);
}

void __wrap_FpFwLockRelease(PFPFW_LOCK Lock, FPFW_LOCK_STATE OldState)
{
    assert_non_null(Lock);
    check_expected(OldState);

    function_called();
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}

// ICC Mocks
static std::map<uint32_t, icc_base_send_complete_notify> icc_send_cb;
static std::map<uint32_t, void*> icc_send_cb_ctx;
fpfw_status_t __wrap_fpfw_icc_base_send(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_send_req_t* params)
{
    check_expected(icc_ctx);
    assert_non_null(params);

    icc_send_cb[(uint32_t)icc_ctx] = params->cb;
    icc_send_cb_ctx[(uint32_t)icc_ctx] = params->cb_ctx;

    function_called();
    return 0; // FPFW_ICC_BASE_STATUS_SUCCESS
}

static std::map<uint32_t, icc_base_recv_complete_notify> icc_recv_cb;
static std::map<uint32_t, void*> icc_recv_cb_ctx;
fpfw_status_t __wrap_fpfw_icc_base_recv(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_recv_req_t* params)
{
    assert_non_null(icc_ctx);
    assert_non_null(params);

    icc_recv_cb[(uint32_t)icc_ctx] = params->cb;
    icc_recv_cb_ctx[(uint32_t)icc_ctx] = params->cb_ctx;

    function_called();
    return 0; // FPFW_ICC_BASE_STATUS_SUCCESS
}

#ifdef PLDM_DRV_WORKAROUND
// PLDM Driver Mocks
PDFWK_ASYNC_REQUEST_HEADER __wrap_last_pldm_request_sent = NULL;
DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE __wrap_pldm_platform_event_ready_callback = NULL;
void* __wrap_CompletionContext = NULL;
fpfw_status_t __wrap_pldm_drv_register_platform_event_ready_notification(pldm_request_t* request)
{
    assert_non_null(request);
    request->header.RequestType = PLDM_GET_READY_ASYNC;
    __wrap_last_pldm_request_sent = &request->header;
    __wrap_pldm_platform_event_ready_callback = request->header.CompletionRoutine;
    __wrap_CompletionContext = request->header.CompletionContext;

    function_called();

    return FPFW_STATUS_SUCCESS;
}

static PlatformEventNotificationCb pldm_raise_event_cb;
static void* pldm_raise_event_ctx;
fpfw_status_t __wrap_pldm_drv_raise_platform_event(pldm_request_t* request,
                                                   pldm_platform_event_config_t* pe_config,
                                                   pldm_platform_event_notification* pe_notification)
{
    assert_non_null(request);
    assert_non_null(pe_config);
    assert_non_null(pe_notification);
    request->header.RequestType = PLDM_SEND_PLATFORM_EVENT_ASYNC;
    pldm_raise_event_cb = pe_notification->CallBack;
    pldm_raise_event_ctx = pe_notification->context;

    function_called();

    return FPFW_STATUS_SUCCESS;
}
#else
// PLDM Mocks
static PlatformEventReadyNotificationCb pldm_ready_cb;
static void* pldm_ready_cb_ctx;
fpfw_status_t __wrap_fpfw_pldm_service_register_platform_event_ready_notification(pldm_platform_event_ready_notification* p_notification)
{
    assert_non_null(p_notification);
    pldm_ready_cb = p_notification->CallBack;
    pldm_ready_cb_ctx = p_notification->context;

    function_called();

    return 0; // FPFW_STATUS_SUCCESS
}

static PlatformEventNotificationCb pldm_raise_event_cb;
static void* pldm_raise_event_ctx;
fpfw_status_t __wrap_fpfw_pldm_service_raise_platform_event(pldm_platform_event_config_t* p_pe_config,
                                                            pldm_platform_event_notification* p_notification)
{
    assert_non_null(p_pe_config);
    assert_non_null(p_notification);
    pldm_raise_event_cb = p_notification->CallBack;
    pldm_raise_event_ctx = p_notification->context;

    function_called();

    return 0; // FPFW_STATUS_SUCCESS
}
#endif

// DFWK Mocks
void __wrap_DfwkAsyncRequestComplete(PDFWK_ASYNC_REQUEST_HEADER Request)
{
    assert_non_null(Request);
    function_called();

    // Call the completion routine
    Request->State = DfwkRequestState_Available;
    Request->CompletionRoutine(Request, Request->CompletionContext);
}

void __wrap_DfwkDeviceInitialize(PDFWK_DEVICE_HEADER Device, PDFWK_SCHEDULE Schedule)
{
    assert_non_null(Device);
    assert_non_null(Schedule);

    Device->Schedule = Schedule;

    function_called();
}

static DFWK_ASYNC_REQUEST_DISPATCH sel_svc_dispatch;
static void* sel_svc_dispatch_ctx;
void __wrap_DfwkQueueInitialize(PDFWK_QUEUE Queue,
                                PDFWK_DEVICE_HEADER Device,
                                DFWK_ASYNC_REQUEST_DISPATCH DispatchRoutine,
                                void* DispatchContext,
                                DFWK_QUEUE_TYPE QueueType)
{
    assert_non_null(Queue);
    assert_non_null(Device);
    assert_non_null(DispatchRoutine);
    assert_true(QueueType == DfwkQueueType_SerializedDispatch);

    sel_svc_dispatch = DispatchRoutine;
    sel_svc_dispatch_ctx = DispatchContext;

    function_called();
}

void __real_DfwkInterfaceInitialize(PDFWK_INTERFACE_HEADER Interface,
                                    PDFWK_DEVICE_HEADER Device,
                                    PDFWK_QUEUE DispatchQueue,
                                    DFWK_REQUEST_DISPATCH_SYNC DispatchSync);
void __wrap_DfwkInterfaceInitialize(PDFWK_INTERFACE_HEADER Interface,
                                    PDFWK_DEVICE_HEADER Device,
                                    PDFWK_QUEUE DispatchQueue,
                                    DFWK_REQUEST_DISPATCH_SYNC DispatchSync)
{
    __real_DfwkInterfaceInitialize(Interface, Device, DispatchQueue, DispatchSync);
    function_called();
}

int32_t __wrap_DfwkInterfaceOpen(PDFWK_INTERFACE_HEADER Interface, PDFWK_DEVICE_HEADER ClientDevice)
{
    assert_non_null(Interface);
    assert_non_null(ClientDevice);

    assert_false(Interface->Opened);
    Interface->ClientDevice = ClientDevice;
    Interface->Opened = true;
    assert_null(Interface->InterfaceOpen);

    function_called();

    return 0; // DFWK_SUCCESS
}

void __real_DfwkAsyncRequestInitialize(PDFWK_ASYNC_REQUEST_HEADER Request, size_t RequestSize);
void __wrap_DfwkAsyncRequestInitialize(PDFWK_ASYNC_REQUEST_HEADER Request, size_t RequestSize)
{
    __real_DfwkAsyncRequestInitialize(Request, RequestSize);
    function_called();
}

static PDFWK_ASYNC_REQUEST_HEADER last_async_request_sent = NULL;
void __real_DfwkAsyncRequestSetCompletionRoutine(PDFWK_ASYNC_REQUEST_HEADER Request,
                                                 DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
                                                 void* CompletionContext);
void __wrap_DfwkAsyncRequestSetCompletionRoutine(PDFWK_ASYNC_REQUEST_HEADER Request,
                                                 DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
                                                 void* CompletionContext)
{
    __real_DfwkAsyncRequestSetCompletionRoutine(Request, CompletionRoutine, CompletionContext);
    function_called();

    last_async_request_sent = NULL;
}

void __wrap_DfwkInterfaceSendAsync(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request)
{
    assert_non_null(Interface);
    assert_non_null(Request);

    last_async_request_sent = Request;

    function_called();
}

} // extern "C"

static int test_setup_scp(void** ctx)
{
    FPFW_UNUSED(ctx);
    KNG_STATUS status = KNG_SUCCESS;

    // sel_init
    expect_function_call(__wrap_FpFwLockInitialize);
    will_return(__wrap_FpFwLockAcquire, 123);
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_value(__wrap_FpFwLockRelease, OldState, 123);
    expect_function_call(__wrap_FpFwLockRelease);
    sel_init();

    // SCP registers ICC receive for MSCP2MSCP (SCP <-> MCP) and DIE2DIE (SCP0 <-> SCP1)
    expect_function_call(__wrap_fpfw_icc_base_recv);
    status = sel_register_icc(SEL_ICC_MSCP2MSCP, (fpfw_icc_base_ctx_t*)ICC_MSCP2MSCP_HANDLE);
    assert_true(KNG_SUCCEEDED(status));
    expect_function_call(__wrap_fpfw_icc_base_recv);
    status = sel_register_icc(SEL_ICC_DIE2DIE, (fpfw_icc_base_ctx_t*)ICC_DIE2DIE_HANDLE);
    assert_true(KNG_SUCCEEDED(status));

    // SCP initializes DFWK Device and Interface
    expect_function_call(__wrap_DfwkDeviceInitialize);
    expect_function_call(__wrap_DfwkQueueInitialize);
    sel_init_dfwk_device((PDFWK_SCHEDULE)DFWK_THREADX_HOST_HANDLE);

    expect_function_call(__wrap_DfwkInterfaceInitialize);
    expect_function_call(__wrap_DfwkInterfaceOpen);
    sel_init_dfwk_interface();

    return 0;
}

static int test_setup_mcp(KNG_DIE_ID die_id)
{
    KNG_STATUS status = KNG_SUCCESS;

    // sel_init
    expect_function_call(__wrap_FpFwLockInitialize);
    will_return(__wrap_FpFwLockAcquire, 123);
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_value(__wrap_FpFwLockRelease, OldState, 123);
    expect_function_call(__wrap_FpFwLockRelease);
    sel_init();

    // MCP registers ICC receive for MSCP2MSCP (SCP <-> MCP), DIE2DIE (MCP0 <-> MCP1) and MSCP2APS (MCP <-> AP_S)
    expect_function_call(__wrap_fpfw_icc_base_recv);
    status = sel_register_icc(SEL_ICC_MSCP2MSCP, (fpfw_icc_base_ctx_t*)ICC_MSCP2MSCP_HANDLE);
    assert_true(KNG_SUCCEEDED(status));
    expect_function_call(__wrap_fpfw_icc_base_recv);
    status = sel_register_icc(SEL_ICC_DIE2DIE, (fpfw_icc_base_ctx_t*)ICC_DIE2DIE_HANDLE);
    assert_true(KNG_SUCCEEDED(status));
    expect_function_call(__wrap_fpfw_icc_base_recv);
    status = sel_register_icc(SEL_ICC_MSCP2APS, (fpfw_icc_base_ctx_t*)ICC_MCP2APS_HANDLE);
    assert_true(KNG_SUCCEEDED(status));

    will_return(__wrap_idsw_get_die_id, die_id);
    if (die_id == DIE_0)
    {
        // MCP0 registers PLDM ready callback
        will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
#ifdef PLDM_DRV_WORKAROUND
        expect_function_call(__wrap_DfwkAsyncRequestSetCompletionRoutine);
        expect_function_call(__wrap_pldm_drv_register_platform_event_ready_notification);
#else
        expect_function_call(__wrap_fpfw_pldm_service_register_platform_event_ready_notification);
#endif
    }
    status = sel_register_pldm();
    assert_true(KNG_SUCCEEDED(status));

    // MCP initializes DFWK Device and Interface
    expect_function_call(__wrap_DfwkDeviceInitialize);
    expect_function_call(__wrap_DfwkQueueInitialize);
    sel_init_dfwk_device((PDFWK_SCHEDULE)DFWK_THREADX_HOST_HANDLE);

    expect_function_call(__wrap_DfwkInterfaceInitialize);
    expect_function_call(__wrap_DfwkInterfaceOpen);
    sel_init_dfwk_interface();

    if (die_id == DIE_0)
    {
        // Simulate PLDM ready callback
        will_return(__wrap_idsw_get_die_id, die_id);
        will_return(__wrap_idsw_get_cpu_type, CPU_MCP);

        expect_function_call(__wrap_DfwkAsyncRequestInitialize);
        expect_function_call(__wrap_DfwkAsyncRequestSetCompletionRoutine);
        expect_function_call(__wrap_DfwkInterfaceSendAsync);

#ifdef PLDM_DRV_WORKAROUND
        __wrap_pldm_platform_event_ready_callback(__wrap_last_pldm_request_sent, __wrap_CompletionContext);
#else
        pldm_ready_cb(0, pldm_ready_cb_ctx);
#endif

        // Pop from queue
        will_return(__wrap_FpFwLockAcquire, 123);
        expect_function_call(__wrap_FpFwLockAcquire);
        expect_value(__wrap_FpFwLockRelease, OldState, 123);
        expect_function_call(__wrap_FpFwLockRelease);

        // Queue is empty. Request will be completed
        expect_function_call(__wrap_DfwkAsyncRequestComplete);
        assert_non_null(last_async_request_sent);
        sel_svc_dispatch(last_async_request_sent, sel_svc_dispatch_ctx);
    }

    return 0;
}

static int test_setup_mcp0(void** ctx)
{
    FPFW_UNUSED(ctx);

    return test_setup_mcp(DIE_0);
}

static int test_setup_mcp1(void** ctx)
{
    FPFW_UNUSED(ctx);

    return test_setup_mcp(DIE_1);
}

static int test_teardown(void** ctx)
{
    FPFW_UNUSED(ctx);

    // Clear queue
    expect_function_call(__wrap_FpFwLockInitialize);
    will_return(__wrap_FpFwLockAcquire, 123);
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_value(__wrap_FpFwLockRelease, OldState, 123);
    expect_function_call(__wrap_FpFwLockRelease);
    sel_init_queue();

    // Clear request
    if (last_async_request_sent != NULL)
    {
        sel_svc_request_t* request = (sel_svc_request_t*)last_async_request_sent;
        memset(request, 0, sizeof(sel_svc_request_t));

        request->status = KNG_SUCCESS;
        request->is_completed = true;
    }

    return 0;
}

//
// Tests
//
TEST_FUNCTION(test_sel_mgr_init_and_queue, nullptr, test_teardown)
{
    sel_event_record_t record[MAX_SEL_QUEUE_LENGTH + 1] = {};
    sel_event_record_t out_record = {};

    will_return_always(__wrap_FpFwLockAcquire, 123);
    expect_value_count(__wrap_FpFwLockRelease, OldState, 123, -1);

    // sel_init
    expect_function_call(__wrap_FpFwLockInitialize);
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);
    sel_init();

    // Single push & pop test
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    record[0].default_info.record_id = 1234;
    bool ret = sel_push(&record[0]);
    assert_true(ret);
    ret = sel_pop(&out_record);
    assert_true(ret);
    assert_int_equal(out_record.default_info.record_id, record[0].default_info.record_id);

    // Head push into empty queue
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    record[0].default_info.record_id = 5678;
    ret = sel_push_head(&record[0]);
    assert_true(ret);
    ret = sel_pop(&out_record);
    assert_true(ret);
    assert_int_equal(out_record.default_info.record_id, record[0].default_info.record_id);

    // Multi push & pop test
    for (int i = 0; i < 5; i++) // Push
    {
        expect_function_call(__wrap_FpFwLockAcquire);
        expect_function_call(__wrap_FpFwLockRelease);
    }
    for (int i = 0; i < 5; i++) // Pop
    {
        expect_function_call(__wrap_FpFwLockAcquire);
        expect_function_call(__wrap_FpFwLockRelease);
    }

    for (int i = 0; i < 5; i++)
    {
        record[i].default_info.record_id = (uint16_t)(i + 1);
        ret = sel_push(&record[i]);
        assert_true(ret);
    }
    for (int i = 0; i < 5; i++)
    {
        ret = sel_pop(&out_record);
        assert_true(ret);
        assert_int_equal(out_record.default_info.record_id, record[i].default_info.record_id);
    }

    // Overflow test
    for (int i = 0; i < MAX_SEL_QUEUE_LENGTH + 1; i++) // push
    {
        expect_function_call(__wrap_FpFwLockAcquire);
        expect_function_call(__wrap_FpFwLockRelease);
    }

    for (int i = 0; i < MAX_SEL_QUEUE_LENGTH + 1; i++)
    {
        record[i].default_info.record_id = (uint16_t)(i + 1);
        ret = sel_push(&record[i]);

        if (i < MAX_SEL_QUEUE_LENGTH)
        {
            assert_true(ret);
        }
        else
        {
            assert_false(ret); // The last one should return false
        }
    }

    // Pop one to verify the oldest one is dropped
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    ret = sel_pop(&out_record);
    assert_true(ret);
    assert_int_equal(out_record.default_info.record_id, record[1].default_info.record_id); // The oldest one is dropped.

    // Push head into only single slot left
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    ret = sel_push_head(&out_record);
    assert_true(ret);

    // Try to push head into full queue
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);
    ret = sel_push_head(&out_record);
    assert_false(ret);

    // Underflow test
    for (int i = 0; i < MAX_SEL_QUEUE_LENGTH + 1; i++) // pop
    {
        expect_function_call(__wrap_FpFwLockAcquire);
        expect_function_call(__wrap_FpFwLockRelease);
    }

    for (int i = 0; i < MAX_SEL_QUEUE_LENGTH + 1; i++)
    {
        ret = sel_pop(&out_record);

        if (i < MAX_SEL_QUEUE_LENGTH)
        {
            assert_true(ret);
            assert_int_equal(out_record.default_info.record_id, record[i + 1].default_info.record_id);
        }
        else
        {
            assert_false(ret); // The last one should return false (Underflow)
        }
    }
}

TEST_FUNCTION(test_sel_mgr_log_sel_event_scp0, test_setup_scp, test_teardown)
{
    will_return_always(__wrap_FpFwLockAcquire, 123);
    expect_value_count(__wrap_FpFwLockRelease, OldState, 123, -1);

    // Push to queue
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);

    expect_function_call(__wrap_DfwkAsyncRequestInitialize);
    expect_function_call(__wrap_DfwkAsyncRequestSetCompletionRoutine);
    expect_function_call(__wrap_DfwkInterfaceSendAsync);

    sel_event_record_t record = {};
    record.default_info.record_id = 0xABCD;

    log_sel_event(&record);

    // Call DFWK dispatch to simulate sending SEL via ICC
    // Pop from queue
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    // Expect ICC send to MCP0
    expect_value(__wrap_fpfw_icc_base_send, icc_ctx, (fpfw_icc_base_ctx_t*)ICC_MSCP2MSCP_HANDLE);
    expect_function_call(__wrap_fpfw_icc_base_send);

    assert_non_null(last_async_request_sent);
    sel_svc_dispatch(last_async_request_sent, sel_svc_dispatch_ctx);

    // Invoke ICC send complete callback to complete the flow
    expect_function_call(__wrap_DfwkAsyncRequestComplete);

    // Expecting Flush the queue again until it's empty
    expect_function_call(__wrap_DfwkAsyncRequestSetCompletionRoutine);
    expect_function_call(__wrap_DfwkInterfaceSendAsync);
    icc_send_cb[(uint32_t)ICC_MSCP2MSCP_HANDLE](icc_send_cb_ctx[(uint32_t)ICC_MSCP2MSCP_HANDLE], 0); // FPFW_STATUS_SUCCESS

    // Pop from queue
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    // Queue is empty. Request will be completed
    expect_function_call(__wrap_DfwkAsyncRequestComplete);
    assert_non_null(last_async_request_sent);
    sel_svc_dispatch(last_async_request_sent, sel_svc_dispatch_ctx);
}

TEST_FUNCTION(test_sel_mgr_log_sel_event_scp1, test_setup_scp, test_teardown)
{
    will_return_always(__wrap_FpFwLockAcquire, 123);
    expect_value_count(__wrap_FpFwLockRelease, OldState, 123, -1);

    // Push to queue
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    will_return_always(__wrap_idsw_get_die_id, DIE_1);
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);

    expect_function_call(__wrap_DfwkAsyncRequestInitialize);
    expect_function_call(__wrap_DfwkAsyncRequestSetCompletionRoutine);
    expect_function_call(__wrap_DfwkInterfaceSendAsync);

    sel_event_record_t record = {};
    record.default_info.record_id = 0xABCD;

    log_sel_event(&record);

    // Call DFWK dispatch to simulate sending SEL via ICC
    // Pop from queue
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    // Expect ICC send to SCP0
    expect_value(__wrap_fpfw_icc_base_send, icc_ctx, (fpfw_icc_base_ctx_t*)ICC_DIE2DIE_HANDLE);
    expect_function_call(__wrap_fpfw_icc_base_send);

    assert_non_null(last_async_request_sent);
    sel_svc_dispatch(last_async_request_sent, sel_svc_dispatch_ctx);

    // Invoke ICC send complete callback to complete the flow
    expect_function_call(__wrap_DfwkAsyncRequestComplete);

    // Expecting Flush the queue again until it's empty
    expect_function_call(__wrap_DfwkAsyncRequestSetCompletionRoutine);
    expect_function_call(__wrap_DfwkInterfaceSendAsync);
    icc_send_cb[(uint32_t)ICC_DIE2DIE_HANDLE](icc_send_cb_ctx[(uint32_t)ICC_DIE2DIE_HANDLE], 0); // FPFW_STATUS_SUCCESS

    // Pop from queue
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    // Queue is empty. Request will be completed
    expect_function_call(__wrap_DfwkAsyncRequestComplete);
    assert_non_null(last_async_request_sent);
    sel_svc_dispatch(last_async_request_sent, sel_svc_dispatch_ctx);
}

TEST_FUNCTION(test_sel_mgr_log_sel_event_mcp0, test_setup_mcp0, test_teardown)
{
    will_return_always(__wrap_FpFwLockAcquire, 123);
    expect_value_count(__wrap_FpFwLockRelease, OldState, 123, -1);

    // Push to queue
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_idsw_get_cpu_type, CPU_MCP);

    expect_function_call(__wrap_DfwkAsyncRequestSetCompletionRoutine);
    expect_function_call(__wrap_DfwkInterfaceSendAsync);

    sel_event_record_t record = {};
    record.default_info.record_id = 0xABCD;

    log_sel_event(&record);

    // Call DFWK dispatch to simulate sending SEL via ICC
    // Pop from queue
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    // Expect PLDM raise event to BMC
#ifdef PLDM_DRV_WORKAROUND
    expect_function_call(__wrap_pldm_drv_raise_platform_event);
#else
    expect_function_call(__wrap_fpfw_pldm_service_raise_platform_event);
#endif

    assert_non_null(last_async_request_sent);
    sel_svc_dispatch(last_async_request_sent, sel_svc_dispatch_ctx);

    // Invoke ICC send complete callback to complete the flow
    expect_function_call(__wrap_DfwkAsyncRequestComplete);

    // Expecting Flush the queue again until it's empty
    expect_function_call(__wrap_DfwkAsyncRequestSetCompletionRoutine);
    expect_function_call(__wrap_DfwkInterfaceSendAsync);
    pldm_raise_event_cb(FPFW_PLDM_CC_SUCCESS, pldm_raise_event_ctx);

    // Pop from queue
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    // Queue is empty. Request will be completed
    expect_function_call(__wrap_DfwkAsyncRequestComplete);
    assert_non_null(last_async_request_sent);
    sel_svc_dispatch(last_async_request_sent, sel_svc_dispatch_ctx);
}

TEST_FUNCTION(test_sel_mgr_log_sel_event_mcp1, test_setup_mcp1, test_teardown)
{
    will_return_always(__wrap_FpFwLockAcquire, 123);
    expect_value_count(__wrap_FpFwLockRelease, OldState, 123, -1);

    // Push to queue
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    will_return_always(__wrap_idsw_get_die_id, DIE_1);
    will_return_always(__wrap_idsw_get_cpu_type, CPU_MCP);

    expect_function_call(__wrap_DfwkAsyncRequestInitialize);
    expect_function_call(__wrap_DfwkAsyncRequestSetCompletionRoutine);
    expect_function_call(__wrap_DfwkInterfaceSendAsync);

    sel_event_record_t record = {};
    record.default_info.record_id = 0xABCD;

    log_sel_event(&record);

    // Call DFWK dispatch to simulate sending SEL via ICC
    // Pop from queue
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    // Expect ICC send to SCP0
    expect_value(__wrap_fpfw_icc_base_send, icc_ctx, (fpfw_icc_base_ctx_t*)ICC_DIE2DIE_HANDLE);
    expect_function_call(__wrap_fpfw_icc_base_send);

    assert_non_null(last_async_request_sent);
    sel_svc_dispatch(last_async_request_sent, sel_svc_dispatch_ctx);

    // Invoke ICC send complete callback to complete the flow
    expect_function_call(__wrap_DfwkAsyncRequestComplete);

    // Expecting Flush the queue again until it's empty
    expect_function_call(__wrap_DfwkAsyncRequestSetCompletionRoutine);
    expect_function_call(__wrap_DfwkInterfaceSendAsync);
    icc_send_cb[(uint32_t)ICC_DIE2DIE_HANDLE](icc_send_cb_ctx[(uint32_t)ICC_DIE2DIE_HANDLE], 0); // FPFW_STATUS_SUCCESS

    // Pop from queue
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    // Queue is empty. Request will be completed
    expect_function_call(__wrap_DfwkAsyncRequestComplete);
    assert_non_null(last_async_request_sent);
    sel_svc_dispatch(last_async_request_sent, sel_svc_dispatch_ctx);
}

TEST_FUNCTION(test_sel_mgr_icc_receive_event_scp0, test_setup_scp, test_teardown)
{
    // Set up received packet
    icc_mhu_sel_ctx_t* icc_ctx = (icc_mhu_sel_ctx_t*)icc_recv_cb_ctx[(uint32_t)ICC_MSCP2MSCP_HANDLE];
    icc_ctx->payload.header.msg_header.command = ICC_SEL_TRANSFER_TO_BMC;
    icc_ctx->payload.record.default_info.record_id = 0xABCD;

    will_return_always(__wrap_FpFwLockAcquire, 123);
    expect_value_count(__wrap_FpFwLockRelease, OldState, 123, -1);

    // Push received SEL to queue
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);

    expect_function_call(__wrap_DfwkAsyncRequestInitialize);
    expect_function_call(__wrap_DfwkAsyncRequestSetCompletionRoutine);
    expect_function_call(__wrap_DfwkInterfaceSendAsync);

    expect_function_call(__wrap_fpfw_icc_base_recv);

    icc_recv_cb[(uint32_t)ICC_MSCP2MSCP_HANDLE](icc_recv_cb_ctx[(uint32_t)ICC_MSCP2MSCP_HANDLE],
                                                sizeof(icc_mhu_sel_payload_t),
                                                0); // FPFW_STATUS_SUCCESS
    // Call DFWK dispatch to simulate sending SEL via ICC
    // Pop from queue
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    // Expect ICC send to MCP0
    expect_value(__wrap_fpfw_icc_base_send, icc_ctx, (fpfw_icc_base_ctx_t*)ICC_MSCP2MSCP_HANDLE);
    expect_function_call(__wrap_fpfw_icc_base_send);

    assert_non_null(last_async_request_sent);
    sel_svc_dispatch(last_async_request_sent, sel_svc_dispatch_ctx);

    // Invoke ICC send complete callback to complete the flow
    expect_function_call(__wrap_DfwkAsyncRequestComplete);

    // Expecting Flush the queue again until it's empty
    expect_function_call(__wrap_DfwkAsyncRequestSetCompletionRoutine);
    expect_function_call(__wrap_DfwkInterfaceSendAsync);
    icc_send_cb[(uint32_t)ICC_MSCP2MSCP_HANDLE](icc_send_cb_ctx[(uint32_t)ICC_MSCP2MSCP_HANDLE], 0); // FPFW_STATUS_SUCCESS

    // Pop from queue
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    // Queue is empty. Request will be completed
    expect_function_call(__wrap_DfwkAsyncRequestComplete);
    assert_non_null(last_async_request_sent);
    sel_svc_dispatch(last_async_request_sent, sel_svc_dispatch_ctx);
}

TEST_FUNCTION(test_sel_mgr_icc_receive_event_mcp0, test_setup_mcp0, test_teardown)
{
    // Set up received packet
    icc_mhu_sel_ctx_t* icc_ctx = (icc_mhu_sel_ctx_t*)icc_recv_cb_ctx[(uint32_t)ICC_DIE2DIE_HANDLE];
    icc_ctx->payload.header.msg_header.command = ICC_SEL_TRANSFER_TO_BMC;
    icc_ctx->payload.record.default_info.record_id = 0xABCD;

    will_return_always(__wrap_FpFwLockAcquire, 123);
    expect_value_count(__wrap_FpFwLockRelease, OldState, 123, -1);

    // Push received SEL to queue
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_idsw_get_cpu_type, CPU_MCP);

    expect_function_call(__wrap_DfwkAsyncRequestSetCompletionRoutine);
    expect_function_call(__wrap_DfwkInterfaceSendAsync);

    expect_function_call(__wrap_fpfw_icc_base_recv);

    icc_recv_cb[(uint32_t)ICC_DIE2DIE_HANDLE](icc_recv_cb_ctx[(uint32_t)ICC_DIE2DIE_HANDLE],
                                              sizeof(icc_mhu_sel_payload_t),
                                              0); // FPFW_STATUS_SUCCESS

    // Call DFWK dispatch to simulate sending SEL via ICC
    // Pop from queue
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    // Expect PLDM raise event to BMC
#ifdef PLDM_DRV_WORKAROUND
    expect_function_call(__wrap_pldm_drv_raise_platform_event);
#else
    expect_function_call(__wrap_fpfw_pldm_service_raise_platform_event);
#endif

    assert_non_null(last_async_request_sent);
    sel_svc_dispatch(last_async_request_sent, sel_svc_dispatch_ctx);

    // Invoke ICC send complete callback to complete the flow
    expect_function_call(__wrap_DfwkAsyncRequestComplete);

    // Expecting Flush the queue again until it's empty
    expect_function_call(__wrap_DfwkAsyncRequestSetCompletionRoutine);
    expect_function_call(__wrap_DfwkInterfaceSendAsync);
    pldm_raise_event_cb(FPFW_PLDM_CC_SUCCESS, pldm_raise_event_ctx);

    // Pop from queue
    expect_function_call(__wrap_FpFwLockAcquire);
    expect_function_call(__wrap_FpFwLockRelease);

    // Queue is empty. Request will be completed
    expect_function_call(__wrap_DfwkAsyncRequestComplete);
    assert_non_null(last_async_request_sent);
    sel_svc_dispatch(last_async_request_sent, sel_svc_dispatch_ctx);
}