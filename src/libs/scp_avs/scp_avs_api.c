//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_avs_api.c
 * AVS module driver interface.
 */

/*------------- Includes -----------------*/
#include <DfwkDriver.h>
#include <DfwkThreadXHost.h>
#include <bug_check.h>
#include <debug.h>
#include <scp_avs.h>
#include <scp_avs_driver.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

void scp_avs_client_read(PDFWK_INTERFACE_HEADER Interface,
                         PDFWK_ASYNC_REQUEST_HEADER Request,
                         DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
                         void* CompletionContext)
{
    pscp_avs_request avs_read_request = (pscp_avs_request)Request;
    BUG_ASSERT_PARAM(Request->AllocatedSize >= sizeof(scp_avs_request_t), Request->AllocatedSize, sizeof(scp_avs_request_t));
    avs_read_request->Header.RequestType = AVS_REQUEST_READ_DATA;
    DfwkAsyncRequestSetCompletionRoutine(Request, CompletionRoutine, CompletionContext);
    DfwkInterfaceSendAsync(Interface, Request);
}

void scp_avs_client_write(PDFWK_INTERFACE_HEADER Interface,
                          PDFWK_ASYNC_REQUEST_HEADER Request,
                          DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
                          void* CompletionContext)
{
    pscp_avs_request avs_write_request = (pscp_avs_request)Request;
    BUG_ASSERT_PARAM(Request->AllocatedSize >= sizeof(scp_avs_request_t), Request->AllocatedSize, sizeof(scp_avs_request_t));
    avs_write_request->Header.RequestType = AVS_REQUEST_WRITE_DATA;
    DfwkAsyncRequestSetCompletionRoutine(Request, CompletionRoutine, CompletionContext);
    DfwkInterfaceSendAsync(Interface, Request);
}

void scp_avs_client_read_all(PDFWK_INTERFACE_HEADER Interface,
                             PDFWK_ASYNC_REQUEST_HEADER Request,
                             DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
                             void* CompletionContext)
{
    pscp_avs_request avs_read_all_request = (pscp_avs_request)Request;
    BUG_ASSERT_PARAM(Request->AllocatedSize >= sizeof(scp_avs_request_t), Request->AllocatedSize, sizeof(scp_avs_request_t));
    avs_read_all_request->Header.RequestType = AVS_REQUEST_READ_ALL_VCT;
    DfwkAsyncRequestSetCompletionRoutine(Request, CompletionRoutine, CompletionContext);
    DfwkInterfaceSendAsync(Interface, Request);
}

void scp_avs_client_read_multi(PDFWK_INTERFACE_HEADER Interface,
                               PDFWK_ASYNC_REQUEST_HEADER Request,
                               DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
                               void* CompletionContext,
                               uint8_t count)
{
    pscp_avs_request avs_read_multi_request = (pscp_avs_request)Request;
    BUG_ASSERT_PARAM(Request->AllocatedSize >= sizeof(scp_avs_request_t), Request->AllocatedSize, sizeof(scp_avs_request_t));
    avs_read_multi_request->Header.RequestType = AVS_REQUEST_READ_MULTI;
    avs_read_multi_request->avs_params.cmd_count = count;
    DfwkAsyncRequestSetCompletionRoutine(Request, CompletionRoutine, CompletionContext);
    DfwkInterfaceSendAsync(Interface, Request);
}
void scp_avs_client_write_multi(PDFWK_INTERFACE_HEADER Interface,
                                PDFWK_ASYNC_REQUEST_HEADER Request,
                                DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
                                void* CompletionContext)
{
    pscp_avs_request avs_write_multi_request = (pscp_avs_request)Request;
    BUG_ASSERT_PARAM(Request->AllocatedSize >= sizeof(scp_avs_request_t), Request->AllocatedSize, sizeof(scp_avs_request_t));
    avs_write_multi_request->Header.RequestType = AVS_REQUEST_WRITE_MULTI;
    DfwkAsyncRequestSetCompletionRoutine(Request, CompletionRoutine, CompletionContext);
    DfwkInterfaceSendAsync(Interface, Request);
}
