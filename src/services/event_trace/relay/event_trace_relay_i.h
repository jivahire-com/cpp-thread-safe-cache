//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file event_trace_relay_i.h
 *  Private methods
 */

#pragma once

/*--------------- Includes ---------------*/

#include <event_trace_relay.h>

/*-- Symbolic Constant Macros (defines) --*/

#define ETR_CHECK_TX_STATUS(status)             \
    if ((status) != TX_SUCCESS)             \
    {                                       \
        FPFwErrorRaise(status, 0, 0, 0, 0); \
    }

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Process a request from the thread request queue.

 * @param p_service  The service context.
 * @param p_data     The request data.
 */
void etr_process_request(etr_service_context_t* p_service, etr_service_request_t* p_request);

/**
 * Helper functions to handle different ICC events. Queues requests to the ICC thread request queue.
 */
void etr_icc_handle_etc();
void etr_icc_handle_etdcc();
void etr_icc_handle_hsp(void* context, size_t output_size_bytes, fpfw_status_t status);
