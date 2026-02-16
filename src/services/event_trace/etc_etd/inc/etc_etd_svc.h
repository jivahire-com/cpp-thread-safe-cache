//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file etc_etd_svc.h
 * This file is used for client to expose public headers for ETC and ETD initialization.
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include <event_trace_collector.h> // for ETC_SERVICE_CORE_BUFFER_COUNT
#include <event_trace_decoder.h>      // for ETC_SERVICE_CORE_BUFFER_COUNT

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/**
 * @brief Initializes the Event Trace Collection (ETC) service.
 * 
 * This function sets up the ETC service with the necessary configurations,
 * including memory allocation for trace buffers and thread settings.
 * 
 * @note Thread Safe - No
 * @note ISR safe - No
 * @note Blocking call - Yes
 * @note Additional stack requirements - No
 * 
 * @return None
 */
void etc_svc_init(etc_service_context_t* p_etc_service_ctx);

/**
 * @brief Gets the Event Trace Collection (ETC) service context.
 *  
 * This function returns a pointer to the ETC service context, which contains
 * the state and configuration for the ETC service.
 * 
 * @return Pointer to the ETC service context.
 */
etc_service_context_t* get_etc_service_context(void);

/**
 * @brief Initializes the Event Trace Decoder (ETD) service.
 * 
 * This function sets up the ETD service with the necessary configurations,
 * including event queue memory and thread settings.
 * 
 * @note Thread Safe - No
 * @note ISR safe - No
 * @note Blocking call - No
 * @note Additional stack requirements - No
 * 
 * @return None
 */
void etd_svc_init(etd_service_context_t* p_etd_service_ctx);