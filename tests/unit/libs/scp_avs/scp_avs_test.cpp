//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_avs_test.cpp
 * Test for SCP AVS
 */

/*------------- Includes -----------------*/
#include "inc\scp_avs_test_mocks.h" // IWYU pragma: keep

#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include "DfwkHost.h"  // for DfwkDeviceInitialize
#include "FpFwUtils.h" // for FPFW_UNUSED

#include <DfwkClient.h>
#include <DfwkDriver.h>
#include <scp_avs.h>
#include <scp_avs_driver.h>
#include <scp_avs_driver_i.h>
}

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static DFWK_SCHEDULE s_schedule;
scp_avs_device_t test_avs_device = {
    .config = {},
};
static scp_avs_interface_t test_avs_interface;
static scp_avs_request_t test_avs_Request;
static scp_avs_get_request_t test_avs_get_Request;

/*----------- Mock Functions -------------*/

/*------------- Functions ----------------*/
static void test_scp_request_completion(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext)
{
    check_expected(Request);
    check_expected(CompletionContext);
}

static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);

    test_avs_device.avs_bus_num = AVS_BUS1;

    DfwkDeviceInitialize(&test_avs_device.Header, &s_schedule);

    test_avs_interface.Device = &test_avs_device;

    int32_t result = DfwkClientInterfaceOpen(&test_avs_interface.Header);
    if (DFWK_FAILED(result))
    {
        return result;
    }
    DfwkAsyncRequestInititalize(&test_avs_Request.Header, sizeof(test_avs_Request));
    test_avs_Request.Header.OwningInterface = &test_avs_interface.Header;
    DfwkInterfaceInitialize(&test_avs_interface.Header, &test_avs_device.Header, &test_avs_device.avs_queue, scp_avs_dispatch_sync);

    return 0;
}

static int test_cleanup(void** pContext)
{
    (void)pContext;
    DfwkClientInterfaceClose(&test_avs_interface.Header);
    return 0;
}

TEST_FUNCTION(scp_avs_driver_init_test, test_setup, test_cleanup)
{
    test_avs_device.avs_bus_num = AVS_BUS2;

    scp_avs_driver_initialize(&test_avs_device);
    assert_int_equal(test_avs_device.avs_bus_num, AVS_BUS2);
}

TEST_FUNCTION(scp_avs_interface_init_test, test_setup, test_cleanup)
{
    test_avs_device.avs_bus_num = AVS_BUS1;

    scp_avs_interface_initialize(&test_avs_device, &test_avs_interface);
    assert_int_equal(test_avs_device.avs_bus_num, AVS_BUS1);
}

TEST_FUNCTION(scp_avs_client_read_test, test_setup, test_cleanup)
{
    expect_value(__wrap_DfwkInterfaceSendAsync, Interface, (uintptr_t)&test_avs_interface);
    expect_value(__wrap_DfwkInterfaceSendAsync, Request, (uintptr_t)&test_avs_Request);

    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, Request, (uintptr_t)&test_avs_Request);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionRoutine, test_scp_request_completion);
    expect_any(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionContext);

    scp_avs_client_read(&test_avs_interface.Header, &test_avs_Request.Header, test_scp_request_completion, nullptr);
    assert_int_equal(test_avs_Request.Header.RequestType, AVS_REQUEST_READ_DATA);
}

TEST_FUNCTION(scp_avs_client_write_test, test_setup, test_cleanup)
{
    expect_value(__wrap_DfwkInterfaceSendAsync, Interface, (uintptr_t)&test_avs_interface);
    expect_value(__wrap_DfwkInterfaceSendAsync, Request, (uintptr_t)&test_avs_Request);

    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, Request, (uintptr_t)&test_avs_Request);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionRoutine, test_scp_request_completion);
    expect_any(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionContext);

    scp_avs_client_write(&test_avs_interface.Header, &test_avs_Request.Header, test_scp_request_completion, nullptr);
    assert_int_equal(test_avs_Request.Header.RequestType, AVS_REQUEST_WRITE_DATA);
}

TEST_FUNCTION(scp_avs_client_read_all_test, test_setup, test_cleanup)
{
    expect_value(__wrap_DfwkInterfaceSendAsync, Interface, (uintptr_t)&test_avs_interface);
    expect_value(__wrap_DfwkInterfaceSendAsync, Request, (uintptr_t)&test_avs_Request);

    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, Request, (uintptr_t)&test_avs_Request);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionRoutine, test_scp_request_completion);
    expect_any(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionContext);

    scp_avs_client_read_all(&test_avs_interface.Header, &test_avs_Request.Header, test_scp_request_completion, nullptr);
    assert_int_equal(test_avs_Request.Header.RequestType, AVS_REQUEST_READ_ALL_VCT);
}

TEST_FUNCTION(scp_avs_client_read_multi_test, test_setup, test_cleanup)
{
    uint8_t test_count = 3;

    expect_value(__wrap_DfwkInterfaceSendAsync, Interface, (uintptr_t)&test_avs_interface);
    expect_value(__wrap_DfwkInterfaceSendAsync, Request, (uintptr_t)&test_avs_Request);

    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, Request, (uintptr_t)&test_avs_Request);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionRoutine, test_scp_request_completion);
    expect_any(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionContext);

    scp_avs_client_read_multi(&test_avs_interface.Header, &test_avs_Request.Header, test_scp_request_completion, nullptr, test_count);
    assert_int_equal(test_avs_Request.Header.RequestType, AVS_REQUEST_READ_MULTI);
}

TEST_FUNCTION(scp_avs_client_write_multi_test, test_setup, test_cleanup)
{
    uint8_t test_count = 2;

    expect_value(__wrap_DfwkInterfaceSendAsync, Interface, (uintptr_t)&test_avs_interface);
    expect_value(__wrap_DfwkInterfaceSendAsync, Request, (uintptr_t)&test_avs_Request);

    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, Request, (uintptr_t)&test_avs_Request);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionRoutine, test_scp_request_completion);
    expect_any(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionContext);

    scp_avs_client_write_multi(&test_avs_interface.Header, &test_avs_Request.Header, test_scp_request_completion, nullptr, test_count);
    assert_int_equal(test_avs_Request.Header.RequestType, AVS_REQUEST_WRITE_MULTI);
}

TEST_FUNCTION(scp_avs_dispatch_test_read, test_setup, test_cleanup)
{
    test_avs_Request.Header.RequestType = AVS_REQUEST_READ_DATA;

    scp_avs_dispatch(&test_avs_Request.Header, nullptr);
    assert_int_equal((uintptr_t)test_avs_device.outstanding_request, (uintptr_t)&test_avs_Request);
}

TEST_FUNCTION(scp_avs_dispatch_test_write, test_setup, test_cleanup)
{
    test_avs_Request.Header.RequestType = AVS_REQUEST_WRITE_DATA;

    scp_avs_dispatch(&test_avs_Request.Header, nullptr);
    assert_int_equal((uintptr_t)test_avs_device.outstanding_request, (uintptr_t)&test_avs_Request);
}

TEST_FUNCTION(scp_avs_dispatch_test_read_all, test_setup, test_cleanup)
{
    test_avs_Request.Header.RequestType = AVS_REQUEST_READ_ALL_VCT;

    scp_avs_dispatch(&test_avs_Request.Header, nullptr);
    assert_int_equal((uintptr_t)test_avs_device.outstanding_request, (uintptr_t)&test_avs_Request);
}

TEST_FUNCTION(scp_avs_dispatch_test_read_multi, test_setup, test_cleanup)
{
    test_avs_Request.Header.RequestType = AVS_REQUEST_READ_MULTI;

    scp_avs_dispatch(&test_avs_Request.Header, nullptr);
    assert_int_equal((uintptr_t)test_avs_device.outstanding_request, (uintptr_t)&test_avs_Request);
}

TEST_FUNCTION(scp_avs_dispatch_test_write_multi, test_setup, test_cleanup)
{
    test_avs_Request.Header.RequestType = AVS_REQUEST_WRITE_MULTI;

    scp_avs_dispatch(&test_avs_Request.Header, nullptr);
    assert_int_equal((uintptr_t)test_avs_device.outstanding_request, (uintptr_t)&test_avs_Request);
}

TEST_FUNCTION(scp_avs_dispatch_sync, test_setup, test_cleanup)
{
    test_avs_get_Request.Header.RequestType = AVS_GET_ERROR_COUNTS;

    scp_avs_dispatch_sync(&test_avs_get_Request.Header);
    assert_int_equal(test_avs_get_Request.Header.RequestType, AVS_GET_ERROR_COUNTS);
}
