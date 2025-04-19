//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_async_requests_i.h
 * Encapsulates and provides APIs for the rest of the service to send
 * and receive async requests from the PCIe driver.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <scp_pcie_manager.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Send an async request to the PCIe driver for a wait for event
 *        operation. This is used to monitor events reported by the PCIe driver.
 *
 * @param[in] ctx       Pointer to the PCIe manager context for the RPSS.
 * @param[in] async_req Pointer to the async request structure to be sent.
 * @param[in] rp_idx    Index of the root port for which the request is sent.
 *
 * @return None
 */
void send_async_rp_wait_for_event(pcie_manager_context_t* ctx, pcie_async_request_t* async_req, uint8_t rp_idx);

/**
 * @brief Queue maximum number of wait for event requests on all root ports
 *         for a given RPSS. This is a one time operation called by each rpss
 *         management thread so that it can monitor and process events reported
 *         by the PCIe driver.
 *
 * @param[in] ctx  Pointer to the PCIe manager context for the RPSS.
 *
 * @return None
 */
void init_wait_for_event_queue_on_rpss(pcie_manager_context_t* ctx);
