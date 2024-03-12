//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file event_trace_collector_i.h
 *  Private methods
 */

#pragma once

/*--------------- Includes ---------------*/

#include <event_trace_collector.h>
#include <IFpFwEventTracingBuffers.h>
#include <IFpFwEventTracingController.h>

#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

// Callbacks for controller config
void etc_buffer_complete_cb(void* p_context, PFPFW_ET_CORE_BUFFER_HEADER p_core_buffer_header, PFPFW_ET_CONTROLLER p_controller);
void etc_on_event_cb(void* p_event, void* p_context);
uint64_t etc_get_ticks_cb();

// Default request processing
void etc_process_request(etc_service_context_t* p_service, etc_service_completion_request_t* p_request);
