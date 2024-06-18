//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Driver framework mocks for the PCIe driver
 */

/*------------- Includes -----------------*/
#include <DfwkPtrTypes.h> // for PDFWK_ASYNC_REQUEST_HEADER, PDFWK_QUEUE
#include <FpFwCMocka.h>   // IWYU pragma: keep
#include <cmocka.h>       // IWYU pragma: keep

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void __wrap_DfwkQueueEnqueueRequest(PDFWK_QUEUE Queue, PDFWK_ASYNC_REQUEST_HEADER Request)
{
    check_expected_ptr(Queue);
    check_expected_ptr(Request);
}

void __wrap_DfwkAsyncRequestComplete(PDFWK_ASYNC_REQUEST_HEADER Request)
{
    check_expected_ptr(Request);
}
