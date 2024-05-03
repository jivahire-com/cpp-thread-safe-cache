//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_avs_test_mocks.cpp
 * Mocks for scp_avs
 */

/*------------- Includes -----------------*/

#include <CMockaWrapper.h>

extern "C" {
#include <DfwkClient.h>
}

extern "C" {

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*----------- Mock Functions -------------*/

/*------------- Functions ----------------*/
void __wrap_DfwkInterfaceSendAsync(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request)
{
    check_expected(Interface);
    check_expected(Request);
}
void __wrap_DfwkAsyncRequestSetCompletionRoutine(PDFWK_ASYNC_REQUEST_HEADER Request,
                                                 DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
                                                 void* CompletionContext)
{

    check_expected(Request);
    check_expected(CompletionRoutine);
    check_expected(CompletionContext);
}
}