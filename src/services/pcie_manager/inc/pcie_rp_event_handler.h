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
#include <pciess.h>
#include <scp_pcie_manager.h> // for pcie_manager_context_t, pciess_complet...
#include <stdint.h>  // for uint8_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
/*--------- Function Prototypes ----------*/

/**
 *  @brief      Used to check the Link Status of a trained PCIE Link for a 
 *              specified RP 
 *
 *  @param[in]  rpss_idx  PCIESS Index 
 *  @param[in]  rp_idx    Index of the RP within the PCIESS
 *
 *  @retval     
 *              SILIBS_SUCCESS     Link trained to specified speed & width 
 *              SILIBS_E_OVERWRITTEN Link trained but not to specified speed/width 
 *              Other   Link did not train 
 */
silibs_status_t pcie_rp_check_link(uint8_t rpss_idx, uint8_t rp_idx);

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
void process_wait_for_event_data(uint8_t rpss_id, pciess_completion_request_t *req);
