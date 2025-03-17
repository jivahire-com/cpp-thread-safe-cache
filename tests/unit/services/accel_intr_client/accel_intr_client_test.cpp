//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr_client_test.cpp
 * Accel Interrupt client tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...
#include <cstddef>         // for NULL
#include <cstdint>         // for uintptr_t

extern "C" {

#include <CMockaWrapper.h>           // for expect_value, check_expected_ptr, Cmo...
#include <DfwkCommon.h>              // for PDFWK_DEVICE_HEADER, DFWK_ASYNC_REQUE...
#include <DfwkStatus.h>              // for DFWK_SUCCESS
#include <accel_intr_client.h>       // for accel_intr_client_init, send_fatal_intr_async_request...
#include <accel_intr_service_dfwk.h> // for accel_intr_service_t, accel_intr_service_interf...
#include <accelip_id.h>              // for ACCEL_ID_SDM, ACCEL_ID_CDED

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CurCompletionRoutine;
PDFWK_ASYNC_REQUEST_HEADER CurHeader;

/*------------- Functions ----------------*/

/****************************
 * MOCKS
 ****************************/

/**
 * @brief Mock function for DfwkAsyncRequestSetCompletionRoutine
 *
 * @param[in] Request : Request passed from caller function
 * @param[in] CompletionRoutine : Completion Routine. Can be NULL
 * @param[in] CompletionContext : Completion Context. Can be NULL
 *
 */
void __wrap_DfwkAsyncRequestSetCompletionRoutine(PDFWK_ASYNC_REQUEST_HEADER Request,
                                                 DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
                                                 void* CompletionContext)
{
    assert_non_null(Request);
    assert_non_null(CompletionRoutine);
    assert_null(CompletionContext);
    CurCompletionRoutine = CompletionRoutine;
    CurHeader = Request;
}

/**
 * @brief Mock function for DfwkInterfaceSendAsync
 *
 * @param[in] Interface : Interface passed from caller function
 * @param[in] Request : Request passed from caller function
 *
 */
void __wrap_DfwkInterfaceSendAsync(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request)
{
    assert_non_null(Request);
    check_expected_ptr(Interface);
}

/**
 * @brief Mock function for DfwkAsyncRequestInitialize
 *
 * @param[in] Request : Request passed from caller function
 * @param[in] RequestSize : Size of Request sent
 *
 */
void __wrap_DfwkAsyncRequestInitialize(PDFWK_ASYNC_REQUEST_HEADER Request, size_t RequestSize)
{
    check_expected_ptr(Request);
    assert_int_equal(RequestSize, sizeof(DFWK_ASYNC_REQUEST_HEADER));
    Request->AllocatedSize = RequestSize;
}

/**
 * @brief Mock function for DfwkClientInterfaceOpen
 *
 * @param[in] Interface : Interface passed from caller function
 *
 */
int32_t __wrap_DfwkClientInterfaceOpen(PDFWK_INTERFACE_HEADER Interface)
{
    check_expected_ptr(Interface);
    return mock_type(int32_t);
}

/**
 * @brief Mock function for FpFwAssert
 *
 * @param[in] expression : Expression to validate
 *
 */
void __wrap_FpFwAssert(int expression)
{
    check_expected(expression);
}

} // extern "C"

/****************************
 * TESTS
 ****************************/

/**
 * @brief Tests call paths in accel_intr_client
 */
TEST_FUNCTION(test_accel_intr_client, NULL, NULL)
{
    accel_intr_service_interface_t accel_intr_interface = {};

    expect_value(__wrap_DfwkClientInterfaceOpen, Interface, &(accel_intr_interface.header));
    will_return(__wrap_DfwkClientInterfaceOpen, DFWK_SUCCESS);

    expect_any_always(__wrap_DfwkAsyncRequestInitialize, Request);

    accel_intr_client_init(&accel_intr_interface);

    expect_value_count(__wrap_DfwkInterfaceSendAsync, Interface, &accel_intr_interface, 2);

    send_fatal_intr_async_request(ACCEL_ID_SDM);

    assert_non_null(CurCompletionRoutine);
    CurCompletionRoutine(CurHeader, NULL);

    send_mailbox_async_request(ACCEL_ID_SDM);
    assert_non_null(CurCompletionRoutine);
    CurCompletionRoutine(CurHeader, NULL);
}
