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
