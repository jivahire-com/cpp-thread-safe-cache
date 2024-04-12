//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_avs_test_mocks.h
 * Mocks for scp_avs
 */

/*------------- Includes -----------------*/

extern "C" {
#include <DfwkClient.h>
#include <DfwkDriver.h>
#include <DfwkThreadXHost.h>
}

#include <CMockaWrapper.h>
#include <cstddef>
#include <cstdint>
extern "C" {

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
/*----------- Mock Functions -------------*/
void __wrap_DfwkInterfaceSendAsync(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request);
void __wrap_DfwkAsyncRequestSetCompletionRoutine(PDFWK_ASYNC_REQUEST_HEADER Request, DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine, void* CompletionContext);
}