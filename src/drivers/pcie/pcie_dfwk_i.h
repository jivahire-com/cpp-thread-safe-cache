//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * PCIe driver private structure and macro definitions.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkTypes.h>
#include <pcie_dfwk.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *  @brief Top-level dispatch handler which services all requests sent to the
 *         PCIe driver.
 *
 *  @param[in] incoming    Pointer to an incoming asynchronous pcie request
 *  @param[in] context     Input context passed along with the request
 *
 *  @return  None - failures will be returned at the time of request completion.
 */
void pcie_default_dispatch(PDFWK_ASYNC_REQUEST_HEADER incoming, void* context);

/**
 *  @brief Dispatch handler for requests sent on the per-root port request
 *         handler queue.
 *
 *  @param[in] req      Pointer to an asynchronous pcie request
 *  @param[in] context  Input context passed along with the request
 *
 *  @return  None - failures will be returned at the time of request completion.
 */
void pcie_per_rp_dispatch(PDFWK_ASYNC_REQUEST_HEADER req, void* context);

/**
 *  @brief Begins link training on the root port associated with the input
 *         request and starts a timer to handle link training timeouts.
 *
 *  @param[in] req Pointer to an asynchronous pcie request.
 *
 *  @return  None - failures will be returned at the time of request completion.
 */
void begin_link_training(PDFWK_ASYNC_REQUEST_HEADER req);

/**
 *  @brief Handles synchronous requests sent to the PCIe driver.
 *
 *  @param[in] incoming Pointer to an incoming synchronous pcie request.
 *
 *  @return  retval - SILIBS_SUCCESS or error status code based on
 *                    errors encountered
 */
int32_t pcie_sched_sync_op(PDFWK_SYNC_REQUEST_HEADER incoming);

/**
 *  @brief Initializes the asynchronous request pool which the pcie request
 *         dispatcher uses to manage incoming requests from the service layer.
 *
 *  @param[in] None
 *
 *  @return    - TX_SUCCESS or TX_ERROR status codes based on
 *               errors encountered when creating threadx primitives used to
 *               manage the request pool.
 */
unsigned int init_async_request_pool();

/**
 *  @brief Adds an async request to the pcie async request dispatch pool.
 *
 *  @param[in] req
 *             Pointer to the request which is to be added to the dispatch pool.
 *
 *  @return    - TX_SUCCESS or TX_ERROR status codes based on errors encountered
 *               in updating the async. request pool.
 */
unsigned int add_async_req_to_pool(pcie_async_request_t* req);

/**
 *  @brief  Retrieves the latest pending async request associated with
 *          an rpss + root port combination.
 *
 *  @param[in] rpss_idx  Root port subsystem index to retrieve an async.
 *                       request for. Must be less than PCIE_RPSS_COUNT (4)
 *                       on each die.
 *
 *  @param[in] rp_idx    Root port index within the rpss to retrieve an async.
 *                       request for - must be one of 0, 1, 2 or 3.
 *
 *  @param[in] req_type  The type of pending async. request to retrieve.
 *
 *  @return    - Pointer to a pcie_async_request_t instance or NULL in case
 *               there are no pending requests of chosen type/when invalid
 *               parameters are passed.
 */
pcie_async_request_t* get_pending_async_req_for_this_rp(uint8_t rpss_idx, uint8_t rp_idx, pcie_rp_async_request_t req_type);

/**
 *  @brief  Completes the latest pending async request associated with
 *          an rpss + root port combination
 *
 *  @param[in] req  Async request to complete
 *
 *  @return    None
 */
void complete_async_req_for_this_rp(pcie_async_request_t* req);
