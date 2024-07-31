//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Helper functions to implement functionality supporting the PCIe CLI.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <pcie_dfwk.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *  @brief Routes a synchronous request sent by the pcie cli to the
 *         driver to its appropriate handler.
 *
 *  @param[in] pcie_sync_request_t
 *      Synchronous request sent through by the driver
 *
 *  @return
 *      None
 */
void handle_cli_request(pcie_sync_request_t* r);
