//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_avs_driver.h
 * Header containing definitions for the AVS module driver interface.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <debug.h>
#include <DfwkDriver.h>
#include <DfwkThreadXHost.h>
#include <FpFwAssert.h>
#include <scp_avs.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

typedef struct _scp_avs_request_t {
    DFWK_ASYNC_REQUEST_HEADER Header;
    scp_avs_vr_vct_t *avs_response_data;  // Response structure (scp_avs_vr_vct_t) used when reading AVS VCT. Have the client provide a pointer to this.
    scp_avs_command_params_t *avs_params; 
} scp_avs_request_t, *pscp_avs_request;

/* 
Todo: Long term - have the client provide an array with 16 bytes, which contains the 
commands, rail, value,  Array of 16, part of the request would tell how many 
commands were sent (the array filled in).
*/
typedef struct {
    DFWK_SYNC_REQUEST_HEADER Header;
    pscp_avs_error_count avs_request_errors;
} scp_avs_get_request_t, *pscp_avs_get_request;

/*--------- Function Prototypes ----------*/

#ifdef __cplusplus
extern "C" {
#endif

static inline void scp_avs_client_read(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request, DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine, void *CompletionContext)
{
    pscp_avs_request avs_read_request = (pscp_avs_request) Request;
    FPFW_RUNTIME_ASSERT(Request->AllocatedSize >= sizeof(scp_avs_request_t));

    avs_read_request->Header.RequestType = AVS_REQUEST_READ_DATA; 
    DfwkAsyncRequestSetCompletionRoutine(Request, CompletionRoutine, CompletionContext); 
    DfwkInterfaceSendAsync(Interface, Request); 
}

static inline void scp_avs_client_write(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request, DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine, void *CompletionContext)
{
    pscp_avs_request avs_write_request = (pscp_avs_request) Request;
    FPFW_RUNTIME_ASSERT(Request->AllocatedSize >= sizeof(scp_avs_request_t));

    avs_write_request->Header.RequestType = AVS_REQUEST_WRITE_DATA; 
    DfwkAsyncRequestSetCompletionRoutine(Request, CompletionRoutine, CompletionContext); 
    DfwkInterfaceSendAsync(Interface, Request);  
}

static inline void scp_avs_client_read_all(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request, DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine, void *CompletionContext)
{
    pscp_avs_request avs_read_all_request = (pscp_avs_request) Request;
    FPFW_RUNTIME_ASSERT(Request->AllocatedSize >= sizeof(scp_avs_request_t));

    avs_read_all_request->Header.RequestType = AVS_REQUEST_READ_ALL_VCT; 
    DfwkAsyncRequestSetCompletionRoutine(Request, CompletionRoutine, CompletionContext); 
    DfwkInterfaceSendAsync(Interface, Request);  
}

static inline void scp_avs_client_read_multi(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request, DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine, void *CompletionContext, uint8_t count)
{
    FPFW_UNUSED(count); // for now unused
    pscp_avs_request avs_read_multi_request = (pscp_avs_request) Request;
    FPFW_RUNTIME_ASSERT(Request->AllocatedSize >= sizeof(scp_avs_request_t));

    avs_read_multi_request->Header.RequestType = AVS_REQUEST_READ_MULTI; 
    DfwkAsyncRequestSetCompletionRoutine(Request, CompletionRoutine, CompletionContext); 
    DfwkInterfaceSendAsync(Interface, Request);
}

static inline void scp_avs_client_write_multi(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request, DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine, void *CompletionContext, uint8_t count)
{
    FPFW_UNUSED(count); // for now unused
    pscp_avs_request avs_write_multi_request = (pscp_avs_request) Request;
    FPFW_RUNTIME_ASSERT(Request->AllocatedSize >= sizeof(scp_avs_request_t));

    avs_write_multi_request->Header.RequestType = AVS_REQUEST_WRITE_MULTI; 
    DfwkAsyncRequestSetCompletionRoutine(Request, CompletionRoutine, CompletionContext); 
    DfwkInterfaceSendAsync(Interface, Request);  
}

static inline void scp_avs_get_error_counts(PDFWK_INTERFACE_HEADER Interface)
{
    scp_avs_get_request_t request;
    request.Header.RequestType = AVS_GET_ERROR_COUNTS;

    DfwkInterfaceSendSync(Interface, &request.Header);
}

/**
 *
 *    Initializes the AVS device.  
 *
 *    @param[in]  Device
 *        The device object
 * 
 *    @brief Open the AVS device.  The AVS bus will be configured based on static 
 *           configuration information.
 *
 */
void scp_avs_init(PDFWK_THREADX_HOST thread_dfwk_host);

/**
 *
 *    This will handle the AVS interrupt, and will copy the response buffer into the client buffer.
 *    @param[in]  context
 *        SCP AVS device information.
 *
 */

#ifdef __cplusplus
}
#endif