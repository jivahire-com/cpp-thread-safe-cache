//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_ras_i.h
 * Provides wrappers around silibs APIs to probe the various RAS nodes that are
 * part of each PCIe RPSS and uplevel any errors found.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkPtrTypes.h>
#include <silibs_status.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/*
 * @brief Handle a request to probe the PCIe VSECRAS RAS Agent.
 *
 * @param[in] req Pointer to the synchronous request header containing the request details.
 *
 * @return SILIBS_SUCCESS: No errors found after probing the VSECRAS agent.
 *         SILIBS_E_DEVICE: Errors found and returned during probing.
 */
silibs_status_t handle_pcie_vsecras_probe_request(PDFWK_SYNC_REQUEST_HEADER req);

/*
 * @brief Handle a request to probe the PCIe DTIM RAS Agent.
 *
 * @param[in] req Pointer to the synchronous request header containing the request details.
 *
 * @return SILIBS_SUCCESS: No errors found after probing the DTIM agent.
 *         SILIBS_E_DEVICE: Errors found and returned during probing.
 */
silibs_status_t handle_pcie_dtim_probe_request(PDFWK_SYNC_REQUEST_HEADER req);

/*
 * @brief Handle a request to probe the PCIe LTIM RAS Agent.
 *
 * @param[in] req Pointer to the synchronous request header containing the request details.
 *
 * @return SILIBS_SUCCESS: No errors found after probing the LTIM agent.
 *         SILIBS_E_DEVICE: Errors found and returned during probing.
 */
silibs_status_t handle_pcie_ltim_probe_request(PDFWK_SYNC_REQUEST_HEADER req);
