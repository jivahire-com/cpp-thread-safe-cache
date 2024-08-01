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
#include "real_proto.h" // for __real_DfwkDeviceInitialize, __real_DfwkQueueInitialize

#include <DfwkAsyncRequest.h> // for _DFWK_ASYNC_REQUEST_HEADER
#include <DfwkInterface.h>    // for DFWK_REQUEST_DISPATCH_SYNC
#include <DfwkPtrTypes.h>     // for PDFWK_QUEUE, PDFWK_ASYNC_REQUEST_HEADER
#include <DfwkQueue.h>        // for DFWK_ASYNC_REQUEST_DISPATCH, DFWK_QUEU...
#include <FpFwUtils.h>        // for FPFW_UNUSED

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

    __real_DfwkDeviceInitialize(Device, Schedule);
}

void __wrap_DfwkQueueInitialize(PDFWK_QUEUE Queue,
                                PDFWK_DEVICE_HEADER Device,
                                DFWK_ASYNC_REQUEST_DISPATCH DispatchRoutine,
                                void* DispatchContext,
                                DFWK_QUEUE_TYPE QueueType)
{
    check_expected_ptr(Queue);
    check_expected_ptr(Device);

    function_called();

    __real_DfwkQueueInitialize(Queue, Device, DispatchRoutine, DispatchContext, QueueType);
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

    __real_DfwkInterfaceInitialize(Interface, Device, DispatchQueue, DispatchSync);
}

bool __wrap_DfwkQueueDequeueRequest(PDFWK_QUEUE Queue, PDFWK_ASYNC_REQUEST_HEADER* Request)
{
    assert_non_null(Queue);
    assert_non_null(Request);
    function_called();

    return __real_DfwkQueueDequeueRequest(Queue, Request);
}

void __wrap_DfwkQueueEnqueueRequest(PDFWK_QUEUE Queue, PDFWK_ASYNC_REQUEST_HEADER Request)
{
    assert_non_null(Queue);
    assert_non_null(Request);
    function_called();

    __real_DfwkQueueEnqueueRequest(Queue, Request);
}

void __wrap_DfwkAsyncRequestComplete(PDFWK_ASYNC_REQUEST_HEADER Request)
{
    assert_non_null(Request);
    assert_non_null(Request->CompletionRoutine);
    function_called();

    Request->CompletionRoutine(Request, Request->CompletionContext);
}

void __wrap_DfwkScheduleQueueDispatch(PDFWK_SCHEDULE Schedule, PDFWK_QUEUE Queue)
{
    FPFW_UNUSED(Schedule);
    FPFW_UNUSED(Queue);
}

//
// Tests
//

} // extern "C"