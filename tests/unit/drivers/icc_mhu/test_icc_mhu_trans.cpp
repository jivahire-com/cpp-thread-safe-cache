//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_icc_mhu_trans.cpp
 * Tests the MHU ICC Transport Driver
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:icc_mhu

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {

#include <DfwkHost.h>
#include <FPFwInterrupts.h>
#include <FpFwUtils.h>                    // for FPFW_UNUSED
#include <fpfw_icc_transport_interface.h> // Leverage the transport library interrface
#include <fpfw_status.h>                  // for fpfw_status_t
#include <icc_mhu.h>
#include <icc_mhu_cfg.h>
#include <mhu_icc_transport.h>
#include <mhu_icc_transport_i.h>
#include <stddef.h> // for NULL
#include <stdint.h> // for uint32_t
#include <tx_api.h>
}

/*-- Symbolic Constant Macros (defines) --*/

#define TEST_RECV_IRQ_NUM               (1)
#define TEST_SHARED_MEM_SIZE            (sizeof(icc_mhu_header_t) + (8))
#define TEST_ASYNC_SEND_RETRY_PERIOD_NS (FPFW_DUR_MS(10)) //! Threadx timer resolution is 10ms

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

static DFWK_SCHEDULE s_test_schedule;

mhu_icc_transport_device_config_t s_test_dev_config = {
    .recv_channel =
        {
            .ch_shared_mem_size = TEST_SHARED_MEM_SIZE,
        },
    .send_channel =
        {
            .ch_shared_mem_size = TEST_SHARED_MEM_SIZE,
        },
    .async_send_retry_period = TEST_ASYNC_SEND_RETRY_PERIOD_NS,
    .async_send_retry_max = 2,
};

static mhu_icc_transport_device_t s_test_mhu_icc_transport_device;
static mhu_icc_transport_intrf_t s_test_mhu_icc_transport_interface;

static uint8_t s_test_recv_buffer[TEST_SHARED_MEM_SIZE];
static uint8_t s_test_send_buffer[TEST_SHARED_MEM_SIZE];
static icc_mhu_packet_t* s_test_recv_packet = (icc_mhu_packet_t*)s_test_recv_buffer;
static icc_mhu_packet_t* s_test_send_packet = (icc_mhu_packet_t*)s_test_send_buffer;

extern "C" {

uint32_t __wrap_FPFwCoreInterruptRegisterCallback(uint32_t irqnum, FPFwCoreInterruptHandler handler, void* arg)
{
    FPFW_UNUSED(irqnum);
    FPFW_UNUSED(handler);
    FPFW_UNUSED(arg);
    return mock_type(uint32_t);
}

uint32_t __wrap_FPFwCoreInterruptEnableVector(uint32_t irqnum)
{
    FPFW_UNUSED(irqnum);
    return mock_type(uint32_t);
}

uint32_t __wrap_FPFwCoreInterruptDisableVector(uint32_t irqnum)
{
    FPFW_UNUSED(irqnum);
    return mock_type(uint32_t);
}

void __wrap_DfwkAsyncRequestComplete(PDFWK_ASYNC_REQUEST_HEADER Request)
{
    FPFW_UNUSED(Request);
    function_called();
}

void __wrap_DfwkDeviceInitialize(PDFWK_DEVICE_HEADER Device, PDFWK_SCHEDULE Schedule)
{
    FPFW_UNUSED(Device);
    FPFW_UNUSED(Schedule);
    function_called();
}

void __wrap_DfwkQueueEnqueueRequest(PDFWK_QUEUE Queue, PDFWK_ASYNC_REQUEST_HEADER Request)
{
    FPFW_UNUSED(Queue);
    FPFW_UNUSED(Request);
    function_called();
}

fpfw_status_t __wrap_fpfw_timer_create(fpfw_timer_t* timer,
                                       fpfw_timer_variant_t variant,
                                       fpfw_dur_t period,
                                       fpfw_timer_callback cb,
                                       void* ctx)
{
    FPFW_UNUSED(timer);
    FPFW_UNUSED(variant);
    FPFW_UNUSED(period);
    FPFW_UNUSED(cb);
    FPFW_UNUSED(ctx);
    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_timer_enable(fpfw_timer_t* timer, fpfw_dur_t delay)
{
    FPFW_UNUSED(timer);
    assert_true(FPFW_TIMER_VALID_TIME(delay));
    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_timer_reset(fpfw_timer_t* timer)
{
    FPFW_UNUSED(timer);
    return mock_type(fpfw_status_t);
}

uint32_t __wrap_icc_mhu_get_packet(picc_mhu_channel_t p_channel, picc_mhu_packet_t p_dest_mhu_packet)
{
    FPFW_UNUSED(p_channel);
    FPFW_UNUSED(p_dest_mhu_packet);
    return mock_type(uint32_t);
}

uint32_t __wrap_icc_mhu_send_packet(picc_mhu_channel_t p_channel, picc_mhu_packet_t p_src_mhu_packet)
{
    FPFW_UNUSED(p_channel);
    FPFW_UNUSED(p_src_mhu_packet);
    return mock_type(uint32_t);
}

bool __wrap_icc_mhu_check_packet_pending(picc_mhu_channel_t p_channel)
{
    FPFW_UNUSED(p_channel);
    return mock_type(bool);
}

static int test_setup(void** ctx)
{
    FPFW_UNUSED(ctx);

    expect_function_call(__wrap_DfwkDeviceInitialize);
    will_return(__wrap_FPFwCoreInterruptRegisterCallback, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_timer_create, FPFW_STATUS_SUCCESS);

    fpfw_status_t status =
        mhu_icc_transport_device_init(&s_test_mhu_icc_transport_device, &s_test_schedule, &s_test_dev_config);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);

    status = mhu_icc_transport_interface_init(&s_test_mhu_icc_transport_device, &s_test_mhu_icc_transport_interface);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);

    return 0;
}

static int test_teardown(void** ctx)
{
    FPFW_UNUSED(ctx);

    memset(&s_test_mhu_icc_transport_device, 0, sizeof(s_test_mhu_icc_transport_device));
    memset(&s_test_mhu_icc_transport_interface, 0, sizeof(s_test_mhu_icc_transport_interface));

    memset(&s_test_recv_buffer, 0, sizeof(s_test_recv_buffer));
    memset(&s_test_send_buffer, 0, sizeof(s_test_send_buffer));

    return 0;
}
}

//
// Public Header Tests
//

TEST_FUNCTION(test_mhu_icc_transport_device_init_null, NULL, NULL)
{

    fpfw_status_t status = mhu_icc_transport_device_init(NULL, NULL, NULL);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR);

    status = mhu_icc_transport_device_init(&s_test_mhu_icc_transport_device, NULL, NULL);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR);

    status = mhu_icc_transport_device_init(&s_test_mhu_icc_transport_device, &s_test_schedule, NULL);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR);
}

TEST_FUNCTION(test_mhu_icc_transport_device_init_invalid_size, NULL, NULL)
{
    mhu_icc_transport_device_config_t bad_config;
    bad_config.recv_channel.ch_shared_mem_size = 2;
    bad_config.send_channel.ch_shared_mem_size = 1;

    fpfw_status_t status = mhu_icc_transport_device_init(&s_test_mhu_icc_transport_device, &s_test_schedule, &bad_config);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_INVALID_SIZE_ARG_ERR);

    bad_config.recv_channel.ch_shared_mem_size = sizeof(icc_mhu_header_t) - 1;
    bad_config.send_channel.ch_shared_mem_size = sizeof(icc_mhu_header_t) - 1;

    status = mhu_icc_transport_device_init(&s_test_mhu_icc_transport_device, &s_test_schedule, &bad_config);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_INVALID_SIZE_ARG_ERR);
}

TEST_FUNCTION(test_mhu_icc_transport_device_init_invalid_cfg_arg, NULL, NULL)
{
    mhu_icc_transport_device_config_t bad_config;
    bad_config.recv_channel.ch_shared_mem_size = TEST_SHARED_MEM_SIZE;
    bad_config.send_channel.ch_shared_mem_size = TEST_SHARED_MEM_SIZE;
    bad_config.async_send_retry_period = 0; //! Invalid async timer period

    fpfw_status_t status = mhu_icc_transport_device_init(&s_test_mhu_icc_transport_device, &s_test_schedule, &bad_config);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_INVALID_ARG_ERR);
}

TEST_FUNCTION(test_mhu_icc_transport_device_init_success, NULL, NULL)
{
    expect_function_call(__wrap_DfwkDeviceInitialize);
    will_return(__wrap_FPFwCoreInterruptRegisterCallback, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_timer_create, FPFW_STATUS_SUCCESS);

    fpfw_status_t status =
        mhu_icc_transport_device_init(&s_test_mhu_icc_transport_device, &s_test_schedule, &s_test_dev_config);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
}

TEST_FUNCTION(test_mhu_icc_transport_interface_init_null, NULL, NULL)
{
    fpfw_status_t status = mhu_icc_transport_interface_init(NULL, NULL);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR);

    status = mhu_icc_transport_interface_init(&s_test_mhu_icc_transport_device, NULL);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR);
}

TEST_FUNCTION(test_mhu_icc_transport_interface_init_success, NULL, NULL)
{
    // Init the device
    expect_function_call(__wrap_DfwkDeviceInitialize);
    will_return(__wrap_FPFwCoreInterruptRegisterCallback, FPFW_STATUS_SUCCESS);
    will_return(__wrap_fpfw_timer_create, FPFW_STATUS_SUCCESS);

    fpfw_status_t status =
        mhu_icc_transport_device_init(&s_test_mhu_icc_transport_device, &s_test_schedule, &s_test_dev_config);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);

    // Init the interface
    status = mhu_icc_transport_interface_init(&s_test_mhu_icc_transport_device, &s_test_mhu_icc_transport_interface);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
}

TEST_FUNCTION(testmhu_icc_transport_dispatcher_match_cb_false, NULL, NULL)
{
    s_test_recv_packet->header.msg_header.command = 1;

    FPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST recv_req;
    recv_req.Input.PayloadBuffer = (uintptr_t)s_test_recv_packet;

    fpfw_icc_dispatch_table_entry current_entry;
    current_entry.cmd_code = 0;

    bool status = mhu_icc_transport_dispatcher_match_cb(&recv_req, &current_entry, NULL);
    assert_false(status);
}

TEST_FUNCTION(testmhu_icc_transport_dispatcher_match_cb_true, NULL, NULL)
{
    s_test_recv_packet->header.msg_header.command = 1;

    FPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST recv_req;
    recv_req.Input.PayloadBuffer = (uintptr_t)s_test_recv_packet;

    fpfw_icc_dispatch_table_entry current_entry;
    current_entry.cmd_code = 1;

    bool status = mhu_icc_transport_dispatcher_match_cb(&recv_req, &current_entry, NULL);
    assert_true(status);
}

//
// Private Header Tests
//

TEST_FUNCTION(test_mhu_icc_transport_dispatch_sync_null, NULL, NULL)
{
    DFWK_SYNC_REQUEST_HEADER sync_req;

    fpfw_status_t status = mhu_icc_transport_dispatch_sync(NULL);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR);

    sync_req.OwningInterface = NULL;
    status = mhu_icc_transport_dispatch_sync(&sync_req);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR);

    s_test_mhu_icc_transport_interface.device = NULL;
    sync_req.OwningInterface = &s_test_mhu_icc_transport_interface.base_interface;
    status = mhu_icc_transport_dispatch_sync(&sync_req);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_NULL_ARG_ERR);
}

TEST_FUNCTION(test_mhu_icc_transport_dispatch_sync_unsupported, test_setup, test_teardown)
{

    FPFW_ICC_TRANSPORT_SYNC_GET_MAX_MESG_SIZE_REQUEST max_size_req;
    max_size_req.Header.RequestType = 0xFF;
    max_size_req.Header.OwningInterface = (PDFWK_INTERFACE_HEADER)&s_test_mhu_icc_transport_interface;

    fpfw_status_t status = mhu_icc_transport_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&max_size_req);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_UNSUPPORTED_REQ_ERR);
}

TEST_FUNCTION(test_mhu_icc_transport_dispatch_sync_max_size, test_setup, test_teardown)
{

    FPFW_ICC_TRANSPORT_SYNC_GET_MAX_MESG_SIZE_REQUEST max_size_req;
    max_size_req.Header.RequestType = ICC_TRANSPORT_GET_MAX_MESG_SIZE_SYNC_REQUEST_ID;
    max_size_req.Header.OwningInterface = (PDFWK_INTERFACE_HEADER)&s_test_mhu_icc_transport_interface;

    fpfw_status_t status = mhu_icc_transport_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&max_size_req);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    assert_int_equal(max_size_req.Output.MaxMesgSize, TEST_SHARED_MEM_SIZE);
}

TEST_FUNCTION(test_mhu_icc_transport_dispatch_sync_min_size, test_setup, test_teardown)
{
    FPFW_ICC_TRANSPORT_SYNC_GET_MIN_MESG_SIZE_REQUEST min_size_req;
    min_size_req.Header.RequestType = ICC_TRANSPORT_GET_MIN_MESG_SIZE_SYNC_REQUEST_ID;
    min_size_req.Header.OwningInterface = (PDFWK_INTERFACE_HEADER)&s_test_mhu_icc_transport_interface;

    fpfw_status_t status = mhu_icc_transport_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&min_size_req);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    assert_int_equal(min_size_req.Output.MinMesgSize, sizeof(icc_mhu_header_t));
}

TEST_FUNCTION(test_mhu_icc_transport_dispatch_sync_recv_fail, test_setup, test_teardown)
{
    s_test_recv_packet->header.msg_header.payload_size = 0;

    FPFW_ICC_TRANSPORT_SYNC_TRY_RECV_REQUEST recv_try_req;
    recv_try_req.Header.RequestType = ICC_TRANSPORT_TRY_RECV_SYNC_REQUEST_ID;
    recv_try_req.Header.OwningInterface = (PDFWK_INTERFACE_HEADER)&s_test_mhu_icc_transport_interface;
    recv_try_req.Input.PayloadBuffer = (uintptr_t)s_test_recv_packet;
    recv_try_req.Input.BufferSizeBytes = TEST_SHARED_MEM_SIZE;

    will_return(__wrap_icc_mhu_get_packet, ICC_MHU_STATUS_E_CHANNEL_NOT_BUSY);

    fpfw_status_t status = mhu_icc_transport_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&recv_try_req);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_FAILED_ERR);
}

TEST_FUNCTION(test_mhu_icc_transport_dispatch_sync_recv_success, test_setup, test_teardown)
{
    s_test_recv_packet->header.msg_header.payload_size = 0;

    FPFW_ICC_TRANSPORT_SYNC_TRY_RECV_REQUEST recv_try_req;
    recv_try_req.Header.RequestType = ICC_TRANSPORT_TRY_RECV_SYNC_REQUEST_ID;
    recv_try_req.Header.OwningInterface = (PDFWK_INTERFACE_HEADER)&s_test_mhu_icc_transport_interface;
    recv_try_req.Input.PayloadBuffer = (uintptr_t)s_test_recv_packet;
    recv_try_req.Input.BufferSizeBytes = TEST_SHARED_MEM_SIZE;

    will_return(__wrap_icc_mhu_get_packet, ICC_MHU_STATUS_S_SUCCESS);

    fpfw_status_t status = mhu_icc_transport_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&recv_try_req);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
}

TEST_FUNCTION(test_mhu_icc_transport_dispatch_sync_send_busy, test_setup, test_teardown)
{
    FPFW_ICC_TRANSPORT_SYNC_TRY_SEND_REQUEST send_try_req;
    send_try_req.Header.RequestType = ICC_TRANSPORT_TRY_SEND_SYNC_REQUEST_ID;
    send_try_req.Header.OwningInterface = (PDFWK_INTERFACE_HEADER)&s_test_mhu_icc_transport_interface;
    send_try_req.Input.PayloadBuffer = (uintptr_t)s_test_send_packet;
    send_try_req.Input.BufferSizeBytes = TEST_SHARED_MEM_SIZE;

    will_return(__wrap_icc_mhu_send_packet, ICC_MHU_STATUS_E_CHANNEL_BUSY);

    fpfw_status_t status = mhu_icc_transport_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&send_try_req);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_BUSY);
}

TEST_FUNCTION(test_mhu_icc_transport_dispatch_sync_send_success, test_setup, test_teardown)
{
    FPFW_ICC_TRANSPORT_SYNC_TRY_SEND_REQUEST send_try_req;
    send_try_req.Header.RequestType = ICC_TRANSPORT_TRY_SEND_SYNC_REQUEST_ID;
    send_try_req.Header.OwningInterface = (PDFWK_INTERFACE_HEADER)&s_test_mhu_icc_transport_interface;
    send_try_req.Input.PayloadBuffer = (uintptr_t)s_test_send_packet;
    send_try_req.Input.BufferSizeBytes = TEST_SHARED_MEM_SIZE;

    will_return(__wrap_icc_mhu_send_packet, ICC_MHU_STATUS_S_SUCCESS);

    fpfw_status_t status = mhu_icc_transport_dispatch_sync((PDFWK_SYNC_REQUEST_HEADER)&send_try_req);
    assert_int_equal(status, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
}

TEST_FUNCTION(test_mhu_icc_transport_dispatch_async_req_recv, test_setup, test_teardown)
{
    struct _DFWK_ASYNC_REQUEST_HEADER async_req;
    async_req.RequestType = ICC_TRANSPORT_RECV_ASYNC_REQUEST_ID;

    expect_function_call(__wrap_DfwkQueueEnqueueRequest);
    mhu_icc_transport_dispatch_async(&async_req, &s_test_mhu_icc_transport_device);
}

TEST_FUNCTION(test_mhu_icc_transport_dispatch_async_req_send, test_setup, test_teardown)
{
    struct _DFWK_ASYNC_REQUEST_HEADER async_req;
    async_req.RequestType = ICC_TRANSPORT_SEND_ASYNC_REQUEST_ID;

    expect_function_call(__wrap_DfwkQueueEnqueueRequest);
    mhu_icc_transport_dispatch_async(&async_req, &s_test_mhu_icc_transport_device);
}

TEST_FUNCTION(test_mhu_icc_transport_dispatch_async_recv, test_setup, test_teardown)
{
    struct _DFWK_ASYNC_REQUEST_HEADER async_req;

    will_return(__wrap_FPFwCoreInterruptEnableVector, 0);
    mhu_icc_transport_dispatch_async_recv(&async_req, &s_test_mhu_icc_transport_device);

    assert_ptr_equal(s_test_mhu_icc_transport_device.async_recv_ctx.req, &async_req);
}

TEST_FUNCTION(test_mhu_icc_transport_dispatch_async_send_fail, test_setup, test_teardown)
{
    FPFW_ICC_TRANSPORT_ASYNC_SEND_REQUEST async_req;
    async_req.Input.PayloadBuffer = (uintptr_t)s_test_send_packet;
    async_req.Input.BufferSizeBytes = TEST_SHARED_MEM_SIZE;

    will_return(__wrap_icc_mhu_send_packet, ICC_MHU_STATUS_E_CHANNEL_BUSY);
    will_return(__wrap_fpfw_timer_enable, FPFW_STATUS_SUCCESS);
    mhu_icc_transport_dispatch_async_send((PDFWK_ASYNC_REQUEST_HEADER)&async_req, &s_test_mhu_icc_transport_device);

    assert_ptr_equal(s_test_mhu_icc_transport_device.async_send_ctx.req, &async_req);
    assert_true(s_test_mhu_icc_transport_device.async_send_ctx.timer_active);
}

TEST_FUNCTION(test_mhu_icc_transport_dispatch_async_send_success_no_timer, test_setup, test_teardown)
{
    FPFW_ICC_TRANSPORT_ASYNC_SEND_REQUEST async_req;
    async_req.Input.PayloadBuffer = (uintptr_t)s_test_send_packet;
    async_req.Input.BufferSizeBytes = TEST_SHARED_MEM_SIZE;

    will_return(__wrap_icc_mhu_send_packet, ICC_MHU_STATUS_S_SUCCESS);
    expect_function_call(__wrap_DfwkAsyncRequestComplete);
    mhu_icc_transport_dispatch_async_send((PDFWK_ASYNC_REQUEST_HEADER)&async_req, &s_test_mhu_icc_transport_device);

    assert_ptr_equal(s_test_mhu_icc_transport_device.async_send_ctx.req, NULL);
}

TEST_FUNCTION(test_mhu_icc_transport_dispatch_async_send_success_timer_active, test_setup, test_teardown)
{
    FPFW_ICC_TRANSPORT_ASYNC_SEND_REQUEST async_req;
    async_req.Input.PayloadBuffer = (uintptr_t)s_test_send_packet;
    async_req.Input.BufferSizeBytes = TEST_SHARED_MEM_SIZE;

    s_test_mhu_icc_transport_device.async_send_ctx.timer_active = true;

    will_return(__wrap_icc_mhu_send_packet, ICC_MHU_STATUS_S_SUCCESS);
    will_return(__wrap_fpfw_timer_reset, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_DfwkAsyncRequestComplete);
    mhu_icc_transport_dispatch_async_send((PDFWK_ASYNC_REQUEST_HEADER)&async_req, &s_test_mhu_icc_transport_device);

    assert_false(s_test_mhu_icc_transport_device.async_send_ctx.timer_active);
    assert_ptr_equal(s_test_mhu_icc_transport_device.async_send_ctx.req, NULL);
}

TEST_FUNCTION(test_mhu_icc_transport_isr_fail, test_setup, test_teardown)
{
    s_test_recv_packet->header.msg_header.payload_size = 0;

    FPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST async_req;
    async_req.Input.PayloadBuffer = (uintptr_t)s_test_recv_packet;
    async_req.Input.BufferSizeBytes = TEST_SHARED_MEM_SIZE;

    s_test_mhu_icc_transport_device.async_recv_ctx.req = (PDFWK_ASYNC_REQUEST_HEADER)&async_req;

    will_return(__wrap_FPFwCoreInterruptDisableVector, 0);
    will_return(__wrap_icc_mhu_check_packet_pending, true);
    will_return(__wrap_icc_mhu_get_packet, ICC_MHU_STATUS_E_CHANNEL_NOT_BUSY);
    expect_function_call(__wrap_DfwkAsyncRequestComplete);
    mhu_icc_transport_isr(&s_test_mhu_icc_transport_device);

    assert_int_equal(async_req.Output.ReceivedBytes, 0);
    assert_int_equal(async_req.Output.Status, FPFW_ICC_TRANSPORT_STATUS_FAILED_ERR);
    assert_ptr_equal(s_test_mhu_icc_transport_device.async_recv_ctx.req, NULL);
}

TEST_FUNCTION(test_mhu_icc_transport_isr_success, test_setup, test_teardown)
{
    s_test_recv_packet->header.msg_header.payload_size = 0;

    FPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST async_req;
    async_req.Input.PayloadBuffer = (uintptr_t)s_test_recv_packet;
    async_req.Input.BufferSizeBytes = TEST_SHARED_MEM_SIZE;

    s_test_mhu_icc_transport_device.async_recv_ctx.req = (PDFWK_ASYNC_REQUEST_HEADER)&async_req;

    will_return(__wrap_FPFwCoreInterruptDisableVector, 0);
    will_return(__wrap_icc_mhu_check_packet_pending, true);
    will_return(__wrap_icc_mhu_get_packet, ICC_MHU_STATUS_S_SUCCESS);
    expect_function_call(__wrap_DfwkAsyncRequestComplete);
    mhu_icc_transport_isr(&s_test_mhu_icc_transport_device);

    assert_int_not_equal(async_req.Output.ReceivedBytes, 0);
    assert_int_equal(async_req.Output.Status, FPFW_ICC_TRANSPORT_STATUS_SUCCESS);
    assert_ptr_equal(s_test_mhu_icc_transport_device.async_recv_ctx.req, NULL);
}