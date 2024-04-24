//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * This section is used to describe the file and any relevant
 * information related to it purpose and how it should be used.
 */

/*------------- Includes -----------------*/
#include <DfwkPtrTypes.h>
#include <FpFwCMocka.h>
#include <cmocka.h>

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
