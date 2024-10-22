//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file startup_shutdown_interface_test.cpp
 * Startup shutdown service interface tests
 */

/*------------- Includes -----------------*/
#include "sos_test.h" // for POWER_TEST

#include <cstddef> // for NULL
#include <cstdint> // for uintptr_t

extern "C" {

#include <CMockaWrapper.h> // for expect_value, check_expected_ptr, Cmo...
#include <DfwkCommon.h>    // for PDFWK_DEVICE_HEADER, DFWK_ASYNC_REQUE...
#include <DfwkStatus.h>
#include <startup_shutdown.h>
#include <startup_shutdown_init.h>
#include <startup_shutdown_ssi.h>

} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/
#define TEST_TYPE_COLD          COLD_BOOT
#define TEST_STAGE_BOOT         STARTUP_PHASE_MSCP_ASYNC
#define TEST_COMPLETION_ROUTINE ((DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE)0x123456)
#define TEST_COMPLETION_CONTEXT ((void*)0x65430)
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
DFWK_SYNC_REQUEST_HEADER s_sync_header = {};
DFWK_ASYNC_REQUEST_HEADER s_async_header = {};

/*------------- Functions ----------------*/
//
// Mocks
//
extern "C" {
// wrapper function that checks expected values for the incoming parameters
int32_t __wrap_DfwkInterfaceSendSync(PDFWK_INTERFACE_HEADER Interface, PDFWK_SYNC_REQUEST_HEADER Request)
{
    check_expected_ptr(Interface);
    check_expected_ptr(Request);
    s_sync_header = *Request;
    return mock_type(int32_t);
}

void __wrap_DfwkInterfaceSendAsync(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request)
{
    check_expected_ptr(Interface);
    check_expected_ptr(Request);
    s_async_header = *Request;
}

void __wrap_DfwkAsyncRequestSetCompletionRoutine(PDFWK_ASYNC_REQUEST_HEADER Request,
                                                 DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
                                                 void* CompletionContext)
{
    check_expected_ptr(Request);
    check_expected_ptr(CompletionRoutine);
    check_expected_ptr(CompletionContext);
}

void __real_ssi_startup_stage_start(PDFWK_INTERFACE_HEADER p_interface,
                                    pssi_request_t p_request,
                                    ssi_startup_stage_t stage,
                                    ssi_startup_type_t boot_type,
                                    DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                                    void* p_completion_context);

void __real_ssi_startup_stage_complete(PDFWK_INTERFACE_HEADER p_interface,
                                       pssi_request_t p_request,
                                       ssi_startup_stage_t stage,
                                       ssi_startup_type_t boot_type,
                                       DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                                       void* p_completion_context);

void __real_ssi_shutdown_quiesce(PDFWK_INTERFACE_HEADER p_interface,
                                 pssi_request_t p_request,
                                 ssi_shutdown_type_t shutdown_type,
                                 DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                                 void* p_completion_context);

} // extern "C"

//
// Tests
//

// sos interfaces
SOS_TEST(sos_register_ssi, NULL, NULL)
{
    startup_ssi_registration_t test_registration;
    sos_interface_t test_interface;

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &test_interface.header);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    // call the function
    assert_int_equal(sos_register_ssi(&test_interface.header, &test_registration, &test_interface.header), DFWK_SUCCESS);

    // check observable updates
    assert_int_equal(test_registration.p_ssi_interface, &test_interface.header);
    assert_int_equal(s_sync_header.RequestType, STARTUP_REGISTER_SSI_SYNC);
}

SOS_TEST(sos_reset_timeout, NULL, NULL)
{
    sos_interface_t test_interface;
    sos_stage_timeout_t test_timeout;

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &test_interface.header);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    // call the function
    assert_int_equal(sos_reset_timeout(&test_interface.header, test_timeout), DFWK_SUCCESS);

    // check observable updates
    assert_int_equal(s_sync_header.RequestType, STARTUP_RESET_TIMEOUT_SYNC);
}

SOS_TEST(sos_start_phase, NULL, NULL)
{
    sos_interface_t test_interface;
    startup_start_phase_request_t test_request;

    expect_value(__wrap_DfwkInterfaceSendAsync, Interface, &test_interface.header);
    expect_any(__wrap_DfwkInterfaceSendAsync, Request);

    expect_any(__wrap_DfwkAsyncRequestSetCompletionRoutine, Request);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionRoutine, TEST_COMPLETION_ROUTINE);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionContext, TEST_COMPLETION_CONTEXT);

    // call the function
    sos_start_phase(&test_interface.header, &test_request, TEST_TYPE_COLD, TEST_STAGE_BOOT, TEST_COMPLETION_ROUTINE, TEST_COMPLETION_CONTEXT);

    // check observable updates
    assert_int_equal(s_async_header.RequestType, STARTUP_REQUEST_START_PHASE_ASYNC);
}

SOS_TEST(sos_start_phase_sync, NULL, NULL)
{
    sos_interface_t test_interface;

    expect_value(__wrap_DfwkInterfaceSendSync, Interface, &test_interface.header);
    expect_any(__wrap_DfwkInterfaceSendSync, Request);
    will_return(__wrap_DfwkInterfaceSendSync, DFWK_SUCCESS);

    // call the function
    sos_start_phase(&test_interface.header, NULL, TEST_TYPE_COLD, TEST_STAGE_BOOT, NULL, NULL);

    // check observable updates
    assert_int_equal(s_sync_header.RequestType, STARTUP_REQUEST_START_PHASE_SYNC);
}

SOS_TEST(sos_shutdown, NULL, NULL)
{
    sos_interface_t test_interface;
    startup_shutdown_request_t test_request;

    expect_value(__wrap_DfwkInterfaceSendAsync, Interface, &test_interface.header);
    expect_any(__wrap_DfwkInterfaceSendAsync, Request);

    expect_any(__wrap_DfwkAsyncRequestSetCompletionRoutine, Request);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionRoutine, TEST_COMPLETION_ROUTINE);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionContext, TEST_COMPLETION_CONTEXT);

    // call the function
    sos_shutdown(&test_interface.header, &test_request, SHUTDOWN, TEST_COMPLETION_ROUTINE, TEST_COMPLETION_CONTEXT);

    // check observable updates
    assert_int_equal(s_async_header.RequestType, STARTUP_REQUEST_SHUTDOWN_ASYNC);
}

// ssi interfaces
SOS_TEST(ssi_startup_stage_start, NULL, NULL)
{
    DFWK_INTERFACE_HEADER test_interface;
    ssi_request_t test_request;

    expect_value(__wrap_DfwkInterfaceSendAsync, Interface, &test_interface);
    expect_any(__wrap_DfwkInterfaceSendAsync, Request);

    expect_any(__wrap_DfwkAsyncRequestSetCompletionRoutine, Request);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionRoutine, TEST_COMPLETION_ROUTINE);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionContext, TEST_COMPLETION_CONTEXT);

    // call the function
    __real_ssi_startup_stage_start(&test_interface, &test_request, TEST_STAGE_BOOT, TEST_TYPE_COLD, TEST_COMPLETION_ROUTINE, TEST_COMPLETION_CONTEXT);

    // check observable updates
    assert_int_equal(s_async_header.RequestType, SSI_STARTUP_STAGE_START_ASYNC);
}

SOS_TEST(ssi_startup_stage_complete, NULL, NULL)
{
    DFWK_INTERFACE_HEADER test_interface;
    ssi_request_t test_request;

    expect_value(__wrap_DfwkInterfaceSendAsync, Interface, &test_interface);
    expect_any(__wrap_DfwkInterfaceSendAsync, Request);

    expect_any(__wrap_DfwkAsyncRequestSetCompletionRoutine, Request);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionRoutine, TEST_COMPLETION_ROUTINE);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionContext, TEST_COMPLETION_CONTEXT);

    // call the function
    __real_ssi_startup_stage_complete(&test_interface, &test_request, TEST_STAGE_BOOT, TEST_TYPE_COLD, TEST_COMPLETION_ROUTINE, TEST_COMPLETION_CONTEXT);

    // check observable updates
    assert_int_equal(s_async_header.RequestType, SSI_STARTUP_STAGE_COMPLETE_ASYNC);
}

SOS_TEST(ssi_shutdown_quiesce, NULL, NULL)
{
    DFWK_INTERFACE_HEADER test_interface;
    ssi_request_t test_request;

    expect_value(__wrap_DfwkInterfaceSendAsync, Interface, &test_interface);
    expect_any(__wrap_DfwkInterfaceSendAsync, Request);

    expect_any(__wrap_DfwkAsyncRequestSetCompletionRoutine, Request);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionRoutine, TEST_COMPLETION_ROUTINE);
    expect_value(__wrap_DfwkAsyncRequestSetCompletionRoutine, CompletionContext, TEST_COMPLETION_CONTEXT);

    // call the function
    __real_ssi_shutdown_quiesce(&test_interface, &test_request, SHUTDOWN, TEST_COMPLETION_ROUTINE, TEST_COMPLETION_CONTEXT);

    // check observable updates
    assert_int_equal(s_async_header.RequestType, SSI_SHUTDOWN_QUIESCE_ASYNC);
}