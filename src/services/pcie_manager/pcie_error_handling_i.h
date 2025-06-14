//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_error_handling_i.h
 * Implements PCIe error and RAS event handling functions as well as invokes
 * the relevant APIs to log CPER and/or SEL events.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <common_types.h>
#include <scp_pcie_manager.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Get the GUID for the vendor-defined PCIe error domain.
 *
 * @retval Pointer to the GUID for the vendor-defined PCIe error domain.
 */
const guid_t* get_pcie_vendor_defined_error_domain_guid(void);

/**
 * @brief Handle a PCIe VSECRAS event by iteratively probing the VSECRAS RAS node
 *        on an RP for errors.
 *
 * @param[in] ctx   - Pointer to the PCIe manager context to issue follow up
 *                    requests to the driver.
 * @param[in] cmpl  - Pointer to the completion request containing the RP index and other details.
 *
 * @retval none
 */
void handle_pcie_vsecras_event(pcie_manager_context_t* ctx, pciess_completion_request_t* cmpl);

/**
 * @brief Handle a PCIe DTIM error event by iteratively probing the DTIM RAS node
 *        on an RP for errors.
 *
 * @param[in] ctx   - Pointer to the PCIe manager context to issue follow up
 *                    requests to the driver.
 * @param[in] cmpl  - Pointer to the completion request containing the RP index and other details.
 *
 * @retval none
 */
void handle_pcie_dtim_event(pcie_manager_context_t* ctx, pciess_completion_request_t* cmpl);

/**
 * @brief Handle a PCIe LTIM error event by iteratively probing the DTIM RAS node
 *        on an RP for errors.
 *
 * @param[in] ctx   - Pointer to the PCIe manager context to issue follow up
 *                    requests to the driver.
 * @param[in] cmpl  - Pointer to the completion request containing the RP index and other details.
 *
 * @retval none
 */
void handle_pcie_ltim_event(pcie_manager_context_t* ctx, pciess_completion_request_t* cmpl);
