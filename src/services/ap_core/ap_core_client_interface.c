//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ap_core_client_interface.c
 * Implements the APcore service client interface.
 */

/*------------- Includes -----------------*/

#include <DfwkCommon.h> // for DfwkInterfaceSendSync, DfwkAsyncRe...
#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <ap_core.h>
#include <stddef.h> // for NULL
#include <stdint.h> // for int32_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void ap_core_set_rvbaraddr(PDFWK_INTERFACE_HEADER p_interface,
                           pap_core_asynchronous_request_t p_request,
                           uint64_t rvbaraddr,
                           DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                           void* p_completion_context)
{
    FPFW_RUNTIME_ASSERT(p_interface != NULL);
    FPFW_RUNTIME_ASSERT(p_request != NULL);
    FPFW_RUNTIME_ASSERT(p_request->header.AllocatedSize >= sizeof(ap_core_asynchronous_request_t));

    p_request->header.RequestType = APCORE_SET_RVBARADDR_ASYNC;
    p_request->data.core_id = rvbaraddr;
    DfwkAsyncRequestSetCompletionRoutine(&p_request->header, completion_routine, p_completion_context);
    // send the async request
    DfwkInterfaceSendAsync(p_interface, (void*)p_request);
}

void ap_core_core_power_on(PDFWK_INTERFACE_HEADER p_interface,
                           pap_core_asynchronous_request_t p_request,
                           uint32_t core_id,
                           DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                           void* p_completion_context)
{
    FPFW_RUNTIME_ASSERT(p_interface != NULL);
    FPFW_RUNTIME_ASSERT(p_request != NULL);
    FPFW_RUNTIME_ASSERT(p_request->header.AllocatedSize >= sizeof(ap_core_asynchronous_request_t));

    p_request->header.RequestType = APCORE_CORE_POWER_ON_ASYNC;
    p_request->data.core_id = core_id;
    DfwkAsyncRequestSetCompletionRoutine(&p_request->header, completion_routine, p_completion_context);
    // send the async request
    DfwkInterfaceSendAsync(p_interface, (void*)p_request);
}

void ap_core_core_power_off(PDFWK_INTERFACE_HEADER p_interface,
                            pap_core_asynchronous_request_t p_request,
                            uint32_t core_id,
                            DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                            void* p_completion_context)
{
    FPFW_RUNTIME_ASSERT(p_interface != NULL);
    FPFW_RUNTIME_ASSERT(p_request != NULL);
    FPFW_RUNTIME_ASSERT(p_request->header.AllocatedSize >= sizeof(ap_core_asynchronous_request_t));

    p_request->header.RequestType = APCORE_CORE_POWER_OFF_ASYNC;
    p_request->data.core_id = core_id;
    DfwkAsyncRequestSetCompletionRoutine(&p_request->header, completion_routine, p_completion_context);
    // send the async request
    DfwkInterfaceSendAsync(p_interface, (void*)p_request);
}