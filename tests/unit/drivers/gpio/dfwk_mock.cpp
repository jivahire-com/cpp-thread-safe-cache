//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dfwk_mock.cpp
 *
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwUtils.h>        // for FPFW_UNUSED
#include <gpio.h>             // for pgpio_request_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_DfwkDeviceInitialize(PDFWK_DEVICE_HEADER Device, PDFWK_SCHEDULE Schedule)
{
    check_expected_ptr(Device);
    check_expected_ptr(Schedule);

    function_called();
}

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

    function_called();
}

void __wrap_DfwkInterfaceInitialize(PDFWK_INTERFACE_HEADER Interface,
                                    PDFWK_DEVICE_HEADER Device,
                                    PDFWK_QUEUE DispatchQueue,
                                    DFWK_REQUEST_DISPATCH_SYNC DispatchSync)
{
    check_expected_ptr(Interface);
    check_expected_ptr(Device);
    check_expected_ptr(DispatchQueue);
    check_expected_ptr(DispatchSync);

    function_called();
}

bool __wrap_DfwkQueueDequeueRequest(PDFWK_QUEUE Queue, PDFWK_ASYNC_REQUEST_HEADER* Request)
{
    assert_non_null(Queue);
    assert_non_null(Request);
    *Request = mock_type(PDFWK_ASYNC_REQUEST_HEADER);

    function_called();

    return mock_type(bool);
}

void __wrap_DfwkQueueEnqueueRequest(PDFWK_QUEUE Queue, PDFWK_ASYNC_REQUEST_HEADER Request)
{
    pgpio_request_t gpio_request = (pgpio_request_t)Request;

    check_expected_ptr(Queue);
    assert_non_null(Request);
    check_expected(Request->RequestType);
    check_expected(gpio_request->gpio_pin_id);

    function_called();
}

void __wrap_DfwkAsyncRequestComplete(PDFWK_ASYNC_REQUEST_HEADER Request)
{
    assert_non_null(Request);

    function_called();
}

void __wrap_DfwkScheduleQueueDispatch(PDFWK_SCHEDULE Schedule, PDFWK_QUEUE Queue)
{
    FPFW_UNUSED(Schedule);
    FPFW_UNUSED(Queue);
}

void __wrap_DfwkAsyncRequestInitialize(PDFWK_ASYNC_REQUEST_HEADER Request, size_t RequestSize)
{
    check_expected_ptr(Request);
    check_expected(RequestSize);

    function_called();
}

void __wrap_DfwkAsyncRequestSetCompletionRoutine(PDFWK_ASYNC_REQUEST_HEADER Request,
                                          DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
                                          void* CompletionContext)
{
    check_expected_ptr(Request);
    check_expected_ptr(CompletionRoutine);
    check_expected_ptr(CompletionContext);

    function_called();
}

void __wrap_DfwkInterfaceSendAsync(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request)
{
    check_expected_ptr(Interface);
    check_expected_ptr(Request);

    function_called();
}

//
// Tests
//

} // extern "C"