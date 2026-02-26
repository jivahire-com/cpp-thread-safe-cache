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
#include <hsp_firmware_headers.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/
#define ETR_EVENT_FLAG_NEW_MTS_MSG                  (1U << 1)
#define ETR_EVENT_FLAG_SEND_DCP_NOTIFICATION        (1U << 2)
#define ETR_EVENT_FLAG_PROCESS_HSP_BUFFER           (1U << 3)
#define ETR_EVENT_FLAG_ANY_VALID                    (ETR_EVENT_FLAG_NEW_MTS_MSG | ETR_EVENT_FLAG_SEND_DCP_NOTIFICATION | ETR_EVENT_FLAG_PROCESS_HSP_BUFFER)

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

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

/**
 * @brief Notify that a HSP DDR buffer or an ASIC DDR buffer is available to be read.
 *
 * @return None
 */
void notify_ddr_buffer_available();

/**
 * @brief Set ThreadX event flags for the ETR worker thread.
 * 
 * @param flags The event flags to set.
 * @return None 
 */
void set_etr_thread_event_flags(ULONG flags);

#ifdef _WIN32   // Unit Test Only
/**
 * @brief Sets an override address for the ATU test payload header used in unit tests.
 *
 * @param atu_test_addr -> Pointer to the test payload header to use.
 *
 * @return None
 */
void etr_set_override_atu_test_address(p_hsp_log_payload_header_t atu_test_addr);
#endif
