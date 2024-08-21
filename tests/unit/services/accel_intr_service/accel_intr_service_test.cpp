//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr_client_test.cpp
 * Accel Interrupt service tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...
#include <cstddef>         // for NULL
#include <cstdint>         // for uintptr_t

extern "C" {

#include <CMockaWrapper.h>           // for expect_value, check_expected_ptr, Cmo...
#include <DfwkCommon.h>              // for PDFWK_DEVICE_HEADER, DFWK_ASYNC_REQUE...
#include <DfwkStatus.h>              // for DFWK_SUCCESS
#include <DfwkThreadXHost.h>         // for DFWK_THREADX_HOST
#include <accel_intr_service.h>      // for accel_intr_service_init, accel_intr_service_interface_init...
#include <accel_intr_service_dfwk.h> // for accel_intr_service_t, accel_intr_service_interf...

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*-- Declarations (Statics and globals) --*/
static DFWK_ASYNC_REQUEST_DISPATCH s_dispatch_routine = NULL;
static DFWK_REQUEST_DISPATCH_SYNC s_dispatch_routine_sync = NULL;

/*------------- Functions ----------------*/

/****************************
 * MOCKS
 ****************************/

/**
 * @brief Mock function for DfwkInterfaceInitialize
 *
 * @param[in] Interface : Interface passed from caller function
 * @param[in] Device : Device passed from caller function
 * @param[in] DispatchQueue : Queue used for Async Messages, passed from caller function
 * @param[in] DispatchSync : DispatchSync. Can be NULL
 *
 */
void __wrap_DfwkInterfaceInitialize(PDFWK_INTERFACE_HEADER Interface,
                                    PDFWK_DEVICE_HEADER Device,
                                    PDFWK_QUEUE DispatchQueue,
                                    DFWK_REQUEST_DISPATCH_SYNC DispatchSync)
{

    check_expected_ptr(Interface);
    check_expected_ptr(Device);
    check_expected_ptr(DispatchQueue);
    check_expected_ptr(DispatchSync);
    // save the dispatch routine for later use
    s_dispatch_routine_sync = DispatchSync;
}

/**
 * @brief Mock function for DfwkDeviceInitialize
 *
 * @param[in] Device : Device passed from caller function
 * @param[in] Schedule : Schedule passed from caller function
 *
 */
void __wrap_DfwkDeviceInitialize(PDFWK_DEVICE_HEADER Device, PDFWK_SCHEDULE Schedule)
{
    check_expected_ptr(Device);
    check_expected_ptr(Schedule);
}

/**
 * @brief Mock function for DfwkQueueInitialize
 *
 * @param[in] Queue : Queue used for Async Messages, passed from caller function
 * @param[in] Device : Device passed from caller function
 * @param[in] DispatchRoutine : DispatchRoutine passed from caller function
 * @param[in] DispatchContext : DispatchContextpassed from caller function
 * @param[in] QueueTyp : Type of Queue
 *
 */
void __wrap_DfwkQueueInitialize(PDFWK_QUEUE Queue,
                                PDFWK_DEVICE_HEADER Device,
                                DFWK_ASYNC_REQUEST_DISPATCH DispatchRoutine,
                                void* DispatchContext,
                                DFWK_QUEUE_TYPE QueueType)
{
    check_expected_ptr(Queue);
    check_expected_ptr(Device);
    check_expected_ptr(DispatchRoutine);
    check_expected_ptr(DispatchContext);
    check_expected(QueueType);
    // save the dispatch routine for later use
    s_dispatch_routine = DispatchRoutine;
}

/**
 * @brief Mock function for DfwkAsyncRequestComplete
 *
 * @param[in] Request : Request header sent by caller function
 *
 */
void __wrap_DfwkAsyncRequestComplete(PDFWK_ASYNC_REQUEST_HEADER Request)
{
    check_expected_ptr(Request);
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

/**
 * @brief Mock function for accel_intr_handle_fatal_intr_recvd
 *
 * @param[in] IRQnum : IRQnum to identify between SDM / CDED Accel IP
 *
 */
void __wrap_accel_intr_handle_fatal_intr_recvd(uint32_t IRQnum)
{
    check_expected(IRQnum);
}

/**
 * @brief Mock function for accel_intr_handle_sdm_msg_recvd
 *
 * @param[in] IRQnum : IRQnum to identify between SDM / CDED Accel IP
 *
 */
void __wrap_accel_intr_handle_sdm_msg_recvd(uint32_t IRQnum)
{
    check_expected(IRQnum);
}

} // extern "C"

/****************************
 * TESTS
 ****************************/

/**
 * @brief Tests call paths in accel_intr_service
 */
TEST_FUNCTION(test_accel_intr_service, NULL, NULL)
{
    accel_intr_service_t accel_intr_service_device = {};
    DFWK_THREADX_HOST test_host = {};
    accel_intr_service_interface_t accel_intr_service_interface = {};

    expect_value(__wrap_DfwkDeviceInitialize, Device, &accel_intr_service_device);
    expect_value(__wrap_DfwkDeviceInitialize, Schedule, &(test_host.Schedule));

    expect_value(__wrap_DfwkQueueInitialize, Queue, &(accel_intr_service_device.default_queue));
    expect_value(__wrap_DfwkQueueInitialize, Device, &(accel_intr_service_device.header));
    expect_any(__wrap_DfwkQueueInitialize, DispatchRoutine);
    expect_value(__wrap_DfwkQueueInitialize, DispatchContext, &(accel_intr_service_device.header));
    expect_value(__wrap_DfwkQueueInitialize, QueueType, DfwkQueueType_SerializedDispatch);

    expect_value(__wrap_DfwkInterfaceInitialize, Interface, &accel_intr_service_interface);
    expect_value(__wrap_DfwkInterfaceInitialize, Device, &(accel_intr_service_device.header));
    expect_value(__wrap_DfwkInterfaceInitialize, DispatchQueue, &(accel_intr_service_device.default_queue));
    expect_any(__wrap_DfwkInterfaceInitialize, DispatchSync);

    accel_intr_service_init(&accel_intr_service_device, &(test_host.Schedule));

    accel_intr_service_interface_init(&accel_intr_service_device, &accel_intr_service_interface);
}

/**
 * @brief Tests default path for async dispatch
 */
TEST_FUNCTION(test_accel_intr_service_dispatch_default, NULL, NULL)
{
    DFWK_ASYNC_REQUEST_HEADER test_request;
    accel_intr_service_t accel_intr_service_device;
    test_request.RequestType = -1; // invalid request type

    expect_value(__wrap_FpFwAssert, expression, false);

    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&test_request, &accel_intr_service_device.header);
}

/**
 * @brief Tests FATAL Interrupt path for async dispatch
 */
TEST_FUNCTION(test_accel_intr_service_dispatch_fatal, NULL, NULL)
{
    accel_intr_service_request_t accel_intr_service_request;
    accel_intr_service_t accel_intr_service_device;
    accel_intr_service_request.header.RequestType = ACCEL_INTR_SERVICE_FATAL_INTR_RECVD;
    accel_intr_service_request.IRQnum = 0x1;

    expect_value(__wrap_accel_intr_handle_fatal_intr_recvd, IRQnum, accel_intr_service_request.IRQnum);
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &accel_intr_service_request.header);

    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&(accel_intr_service_request.header), &accel_intr_service_device.header);
}

/**
 * @brief Tests SDM_MSG Interrupt path for async dispatch
 */
TEST_FUNCTION(test_accel_intr_service_dispatch_sdm_msg, NULL, NULL)
{
    accel_intr_service_request_t accel_intr_service_request;
    accel_intr_service_t accel_intr_service_device;
    accel_intr_service_request.header.RequestType = ACCEL_INTR_SERVICE_SDM_MSG_RECVD;
    accel_intr_service_request.IRQnum = 0x1;

    expect_value(__wrap_accel_intr_handle_sdm_msg_recvd, IRQnum, accel_intr_service_request.IRQnum);
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &accel_intr_service_request.header);

    assert_non_null(s_dispatch_routine);
    s_dispatch_routine(&(accel_intr_service_request.header), &accel_intr_service_device.header);
}
