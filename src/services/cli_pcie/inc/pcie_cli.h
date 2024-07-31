//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Register and setup the pcie cli
 */

#pragma once

/*----------- Nested includes ------------*/
#include <pcie_dfwk.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *  @brief Initialize the pcie cli using FpFwCli framework.
 *
 *  @param[in] pcie_dev_handle
 *      List of pcie device handles which will be used by the cli to
 *      interface with the pcie driver.
 *
 *  @return
 *      None
 */
void pcie_cli_init(pciess_device_t* pcie_dev_handles);
