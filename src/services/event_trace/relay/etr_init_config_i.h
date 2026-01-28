//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file etr_init_config_i.h
 *  Defines initialization configuration data for the Event Trace Relay and ET MTS client.
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include <event_trace_relay.h>
#include <message_transfer_service.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/
/* Definitions required for the ETR-MTS Queue */

/* Each core can queue 2 event trace buffers to MCP. 
   Number of cores per die = 5 (SCP, MCP, SDM, CDED, HSP)
   +2 to account for the host read requests and +1 for the request from MCP Die 1 to MCP Die 0.
   So ETR_MTS_CLIENT_MAX_MESSAGES = 5 * 2 + 2 + 1 = 13 */
#define ETR_MTS_CLIENT_MAX_MESSAGES (13)
#define ETR_MTS_CLIENT_BLOCK_POOL_SIZE (MAX_TRP_MSG_BLOCK_SIZE * ETR_MTS_CLIENT_MAX_MESSAGES)

/* ETR Worker Thread and Queue Names and Configuration */
#define ETR_WORKER_THREAD_NAME          ("etr-worker_thread")
#define ETR_BLOCK_POOL_NAME             ("etr-mts-block_pool")
#define ETR_WORK_QUEUE_NAME             ("etr-mts-work_queue")
#define ETR_FLAGS_NAME                  ("etr-mts-flags")

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/
/* @brief Initialize the HSP communication for the ETR service.
 * 
 * This function sets up the HSP ICC Base context and prepares it to receive requests from the HSP.
 * 
 * @param p_service  The service context.
 * @param p_config   The service configuration.
 */
void etr_initialize_hsp_communication(etr_service_context_t* p_service, const etr_service_config_t* p_config);

