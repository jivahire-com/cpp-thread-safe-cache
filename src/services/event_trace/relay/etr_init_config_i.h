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
   +1 to account for the host read request and +1 for the request from MCP Die 1 to MCP Die 0.
   So ETR_MAX_MTS_CLIENT_MESSAGES = 5 * 2 + 1 + 1 = 12 */
#define ETR_MAX_MTS_CLIENT_MESSAGES (12)
#define ET_MTS_CLIENT_BLOCK_POOL_SIZE \
    ((sizeof(uint32_t) + MAX_TRP_MSG_BLOCK_SIZE) * ETR_MAX_MTS_CLIENT_MESSAGES)

/* Event Flags for the ETR from the MTS Client */
#define ETR_EVENT_FLAG_NEW_MTS_MSG    (0x1)
#define ETR_EVENT_FLAG_ANY_VALID      (ETR_EVENT_FLAG_NEW_MTS_MSG)

/* ETR Worker Thread and Queue Names and Configuration */
#define ETR_WORKER_THREAD_NAME          ("etr-worker_thread")
#define ETR_BLOCK_POOL_NAME             ("etr-mts-block_pool")
#define ETR_WORK_QUEUE_NAME             ("etr-mts-work_queue")

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

