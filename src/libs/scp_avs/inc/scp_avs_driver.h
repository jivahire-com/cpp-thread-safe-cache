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
#define MODULE_NAME_AVS "[AVS] "
#define NEWLINE     "\n"

// set to 1 for more verbose logs
#define AVS_ENABLE_TRACE_LEVEL_LOG 0

#if AVS_ENABLE_TRACE_LEVEL_LOG
#define AVS_LOG_TRACE(fmt, ...) printf(MODULE_NAME_AVS fmt NEWLINE, ##__VA_ARGS__)
#else
#define AVS_LOG_TRACE(fmt, ...) 
#endif
#define AVS_LOG_INFO(fmt, ...) printf(MODULE_NAME_AVS fmt NEWLINE, ##__VA_ARGS__)
#define AVS_LOG_WARN(fmt, ...) printf(MODULE_NAME_AVS fmt NEWLINE, ##__VA_ARGS__)
#define AVS_LOG_CRIT(fmt, ...) printf(MODULE_NAME_AVS fmt NEWLINE, ##__VA_ARGS__)

#define MAX_AVS_INST    4 // Die0 (AVS0 - AVS3), Die1 (AVS4 packaging pin, but is AVS0 on Die1)
// Hardware can support 16 commands.  But there are only 2 VRs per AVSBus. With 3 commands (VCT) sent to each VR, 6 is the max commands sent per AVSBus.
#define MAX_AVS_MULTI_READ_CMDS 6

/*-------------- Typedefs ----------------*/
typedef enum _scp_avs_rail_count_t {
    AVS_RAIL0,
    AVS_RAIL1,
    MAX_AVS_RAILS,
}scp_avs_rail_count_t;

typedef struct _scp_avs_response_data_t {
    uint16_t data;
    avs_error_t error;
} scp_avs_response_data_t;

typedef struct _scp_avs_response_multi_t {
    scp_avs_response_data_t avs_response_multi[AVS_CMD_BUFF_SIZE];
} scp_avs_response_multi_t;

typedef struct _scp_avs_request_t {
    DFWK_ASYNC_REQUEST_HEADER Header;
    union {
        scp_avs_vr_vct_t avs_response_vct;  // Response structure (scp_avs_vr_vct_t) used when reading AVS VCT. Have the client provide a pointer to this.
        scp_avs_response_data_t avs_response_single_resp;   // Single read of voltage (1LSB=1mV), current (1LSB=10mA), or temperature(1LSB=0.1C).
        scp_avs_response_multi_t avs_resp_multi;
    };
    int avs_response_status;
    scp_avs_command_params_t avs_params; 
} scp_avs_request_t, *pscp_avs_request;

typedef struct _scp_avs_isr_request_t {
    DFWK_ASYNC_REQUEST_HEADER Header;
    pscp_avs_request outstanding_client_request;
    bool avs_crc_error;
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
    SCP_AVS_STATUS_CRC_ERROR,
} scp_avs_status_t;

/*--------- Function Prototypes ----------*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 *    Reads data from the voltage rail  
 *
 *    @param[in]  Interface
 *    @param[in]  Request
 *    @param[in]  CompletionRoutine
 *    @param[in]  CompletionContext
 *
 */
void scp_avs_client_read(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request, DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine, void *CompletionContext);

/**
 *
 *    Writes data to the voltage rail  
 *
 *    @param[in]  Interface
 *    @param[in]  Request
 *    @param[in]  CompletionRoutine
 *    @param[in]  CompletionContext
 *
 */
void scp_avs_client_write(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request, DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine, void *CompletionContext);

/**
 *
 *    Read voltage, current, temperature from the voltage rails  
 *
 *    @param[in]  Interface
 *    @param[in]  Request
 *    @param[in]  CompletionRoutine
 *    @param[in]  CompletionContext
 *
 */
void scp_avs_client_read_all(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request, DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine, void *CompletionContext);

/**
 *
 *    Reads data from multiple client specified rails  
 *
 *    @param[in]  Interface
 *    @param[in]  Request
 *    @param[in]  CompletionRoutine
 *    @param[in]  CompletionContext
 *    @param[in]  count
 *
 */
void scp_avs_client_read_multi(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request, DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine, void *CompletionContext, uint8_t count);

/**
 *
 *    Read voltage, current, temperature from the voltage rails  
 *
 *    @param[in]  Interface
 *    @param[in]  Request
 *    @param[in]  CompletionRoutine
 *    @param[in]  CompletionContext
 *
 */
void scp_avs_client_write_multi(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request, DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine, void *CompletionContext);

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