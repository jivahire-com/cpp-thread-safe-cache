//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * This section is used to describe the file and any relevant
 * information related to it purpose and how it should be used.
 */

/*------------- Includes -----------------*/
#include <DfwkCommon.h>
#include <DfwkThreadXHost.h>
#include <FpFwCMocka.h>
#include <pcie_dfwk.h>
#include <stddef.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void __wrap_pcie_dfwk_init(pciess_device_t* dev, PDFWK_SCHEDULE schedule)
{
    assert_non_null(dev);
    assert_non_null(schedule);
    check_expected_ptr(schedule);
}

void __wrap_DfwkAsyncRequestInititalize(PDFWK_ASYNC_REQUEST_HEADER Request, size_t RequestSize)
{
    check_expected_ptr(Request);
    assert_int_equal(RequestSize, sizeof(pcie_async_request_t));
    Request->AllocatedSize = RequestSize;
}

void __wrap_DfwkAsyncRequestSetCompletionRoutine(PDFWK_ASYNC_REQUEST_HEADER Request,
                                                 DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
                                                 void* CompletionContext)
{
    check_expected_ptr(Request);
    check_expected_ptr(CompletionRoutine);
    check_expected_ptr(CompletionContext);
}

void __wrap_DfwkInterfaceSendAsync(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request)
{
    check_expected_ptr(Interface);
    check_expected_ptr(Request);
}
