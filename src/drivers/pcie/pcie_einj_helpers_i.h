//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * This section is used to describe the file and any relevant
 * information related to it purpose and how it should be used.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkPtrTypes.h>
#include <silibs_status.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Inject errors into the PCIe subsystem.
 *
 * @param req Pointer to the request header containing the error injection parameters.
 *
 * @return SILIBS_SUCCESS on success, or an error code on failure.
 */
silibs_status_t handle_pcie_error_injection(PDFWK_SYNC_REQUEST_HEADER req);
