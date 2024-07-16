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
    union {
        scp_avs_vr_vct_t avs_response_vct;  // Response structure (scp_avs_vr_vct_t) used when reading AVS VCT. Have the client provide a pointer to this.
        int16_t avs_response_single_resp;   // Single read of voltage (1LSB=1mV), current (1LSB=10mA), or temperature(1LSB=0.1C).
    };
    int avs_response_status;
    scp_avs_command_params_t avs_params; 
} scp_avs_request_t, *pscp_avs_request;

typedef struct _scp_avs_isr_request_t {
    DFWK_ASYNC_REQUEST_HEADER Header;
    pscp_avs_request outstanding_client_request;
} scp_avs_isr_request_t, *pscp_avs_isr_request;

/* 
Todo: Long term - have the client provide an array with 16 bytes, which contains the 
commands, rail, value,  Array of 16, part of the request would tell how many 
commands were sent (the array filled in).
*/
typedef struct {
    DFWK_SYNC_REQUEST_HEADER Header;
    pscp_avs_error_count avs_request_errors;
} scp_avs_get_request_t, *pscp_avs_get_request;

typedef struct _scp_avs_device_t {
    DFWK_DEVICE_HEADER Header;
    const scp_avs_config_t config;
    DFWK_QUEUE avs_queue;
    DFWK_QUEUE avs_isr_resp_queue;
    pscp_avs_request outstanding_request;
    scp_avs_isr_request_t isr_request;
    uint8_t avs_bus_num;
    pscp_avs_error_count avs_response_errors;
} scp_avs_device_t, *pscp_avs_device;

typedef struct _scp_avs_interface_t {
    DFWK_INTERFACE_HEADER Header;
    pscp_avs_device Device;
} scp_avs_interface_t, *pscp_avs_interface_t;

typedef enum _scp_avs_status_t {
    SCP_AVS_STATUS_SUCCESS,
    SCP_AVS_STATUS_READ_FAIL,
    SCP_AVS_STATUS_WRITE_FAIL,
    SCP_AVS_STATUS_READ_ALL_VCT_FAIL,
    SCP_AVS_STATUS_READ_MULTI_FAIL,
    SCP_AVS_STATUS_WRITE_MULTI_FAIL,
} scp_avs_status_t;

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
 *    @param[in]  avsDevice
 *    @param[in]  scheduler
 * 
 *    @brief Open the AVS device.  The AVS bus will be configured based on static 
 *           configuration information.
 *
 */
 void scp_avs_init(pscp_avs_device avsDevice, DFWK_SCHEDULE* scheduler);

 /**
 *
 *    Initializes the AVS module interface (synchronous and asynchronous).  
 *
 *    @param[in]  Device
 *    @param[in]  Interface
 * 
 *    @brief Called (num of AVS) X number of clients.
 *           If the client makes a synchronous request, then scp_avs_dispatch_sync is called.
 *           If the client makes an asynchronous request, then the request is placed on the Device queue.
 *
 */
void scp_avs_interface_initialize(pscp_avs_device Device, pscp_avs_interface_t Interface);

#ifdef __cplusplus
}
#endif