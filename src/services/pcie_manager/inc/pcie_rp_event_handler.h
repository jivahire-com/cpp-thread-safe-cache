//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implements PCIe rp event processing helper function
 */

/*------------- Includes -----------------*/
#include <pcie_manager_i.h> // for rpss_req_completion_cb, rpss_service_t...
#include <pcie_ss_common.h> // pciess_entity
#include <scp_pcie_manager.h> // for pcie_manager_context_t, pciess_complet...
#include <stdint.h>           // for uint8_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
/*--------- Function Prototypes ----------*/

/**
 *  @brief      Process the response for the WAIT_FOR_EVENT Async Request
 *
 *  @param[in]  rpss_id PCIESS Index
 *  @param[in]  req Completion Req send the to the manager in response to the completion '
 *                  of the WAIT_FOR_EVENT async request.
 *
 *  @retval     None.
 *
 */
void process_wait_for_event_data(pcie_manager_context_t* ctx, pciess_completion_request_t* req);
