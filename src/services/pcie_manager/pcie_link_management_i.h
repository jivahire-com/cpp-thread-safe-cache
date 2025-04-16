//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_link_management_i.h
 * Functions that implement link training and recovery functionality for
 * all RPSS.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <pcie_manager_i.h>
#include <scp_pcie_manager.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Handles the PCIe link down event for a given RPSS (and RP).
 *        This function is called when a link down event is detected.
 *        It will initiate the link re-training process for the specified RP.
 *
 * @param ctx[in]  Pointer to the PCIe manager context associated with this RPSS + RP
 * @param cmpl[in] Pointer to the completion request containing the event data.
 *
 * @retval None - An asynchronous event is returned once link training is complete.
 */
void handle_pcie_link_down_event(pcie_manager_context_t* ctx, pciess_completion_request_t* cmpl);

/**
 * @brief Initiates link training for all enabled RPs within the RPSS.
 *
 * @param ctx[in]  Pointer to the PCIe manager context associated with this RPSS + RP
 *
 * @retval None - An asynchronous event is returned once link training is complete.
 */
void initiate_link_training_on_rpss(pcie_manager_context_t* ctx);
