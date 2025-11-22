//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file et_mts_client.h
 * Header definitions for MCP and SCP Firmware Event Trace MTS Client.
 *
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/  
#include <event_trace_collector.h> // for etc_service_completion_request_t

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/**
 * @brief Initializes the Event Trace MTS client. 
 *
 * @param[in] void
 *
 * @retval void
 */
void event_trace_mts_client_init(void);

/**
 * @brief Notify the Event Trace MTS Client that a buffer is complete.
 *
 * This function can be used to trigger processing of the buffer by the
 * Event Trace MTS Client.
 *
 * @param[in] void
 *
 * @retval fpfw_status_t indicating success or failure of the operation.
 */
fpfw_status_t et_mts_notify_buffer_complete (void* p_buffer, etc_service_completion_request_t* p_request);

/**
 * @brief Worker thread function for the Event Trace MTS client.
 *
 * @param[in] thread_input
 *
 * @retval void
 */
void et_mts_worker_thread_func(ULONG thread_input);