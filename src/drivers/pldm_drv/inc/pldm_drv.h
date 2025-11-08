//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pldm_drv.h
 * PLDM driver public API implementation.
 */
#pragma once

/*----------- Nested includes ------------*/
#include <DfwkDriver.h>
#include <fpfw_pldm_service.h>
#include <kng_error.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef struct
{
    DFWK_DEVICE_HEADER header;
    DFWK_QUEUE dispatch_queue;  // DFWK dispatch queue for PLDM device
    DFWK_QUEUE ready_queue;     // Queue to hold requests waiting for PLDM stack to be ready
    DFWK_QUEUE svc_queue;       // Queue to hold requests for PLDM PE to be free

    bool is_ready;              // true if PLDM stack is ready
    bool has_pending_request;   // true if there is a pending PLDM request
} pldm_device_t;

typedef struct
{
    DFWK_INTERFACE_HEADER header;
} pldm_interface_t;

typedef enum
{
    PLDM_GET_READY_ASYNC = 0x0001,
    PLDM_SEND_PLATFORM_EVENT_ASYNC = 0x0002,
} pldm_request_type_t;

typedef struct
{
    DFWK_ASYNC_REQUEST_HEADER header;
    fpfw_status_t status;
    pldm_platform_event_config_t* pe_config;
    pldm_platform_event_notification* pe_notification;
    fpfw_pldm_cc_t completion_code;
} pldm_request_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Initialize PLDM device
 * 
 * @param schedule DFWK schedule object
 */
void pldm_device_initialize(PDFWK_SCHEDULE schedule);

/**
 * @brief Initialize PLDM interface
 * 
 */
void pldm_interface_initialize();

/**
 * @brief Initialize PLDM driver
 * 
 * @return fpfw_status_t FPFW_STATUS_SUCCESS if succeeded, otherwise error code
 */
fpfw_status_t pldm_driver_initialize();

/**
 * @brief Register PLDM platform event ready notification
 * 
 * @param request PLDM request object
 * @return fpfw_status_t FPFW_STATUS_SUCCESS if succeeded, otherwise error code
 */
fpfw_status_t pldm_drv_register_platform_event_ready_notification(pldm_request_t *request);

/**
 * @brief Raise PLDM platform event
 * 
 * @param request PLDM request object
 * @param pe_config Platform event configuration
 * @param pe_notification Platform event notification
 * @return fpfw_status_t FPFW_STATUS_SUCCESS if succeeded, otherwise error code
 */
fpfw_status_t pldm_drv_raise_platform_event(pldm_request_t *request,
                                            pldm_platform_event_config_t* pe_config,
                                            pldm_platform_event_notification* pe_notification);