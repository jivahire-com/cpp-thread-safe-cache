//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file event_trace_relay_i.h
 *  Private methods
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include <event_trace_relay.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/**
 * @brief Process a request from the thread request queue.
 *
 * @param p_context  The service context.
 * @param p_request  The request data.
 * @param request_type The type of request being processed.
 * @return None
 */
void etr_process_request(etr_service_context_t* p_context, etr_service_request_t* p_request, ETR_SERVICE_REQUEST_TYPE request_type);

/**
 * @brief Handle an incoming HSP message.
 * @note This function is non-static only for unit testing purposes.
 *
 * @param context Pointer to the ETR service context.
 * @param output_size_bytes Size of the output data.
 * @param status Status of the ICC operation.
 */
void etr_icc_handle_hsp(void* context, size_t output_size_bytes, fpfw_status_t status);

/**
 * @brief Worker thread function for processing Event Trace requests.
 * This function is a public function for unit testing purposes.
 *
 * @param thread_input Pointer to the ETR service context.
 * @return None -> This function runs in an infinite loop, so should never return
 */
void etr_worker_thread_func(ULONG thread_input);
