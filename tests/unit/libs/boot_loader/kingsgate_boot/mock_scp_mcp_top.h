//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mock_scp_mcp_top.h
 * Header file containing extern declaration for various mock MCP memory locations
 */

/*----------- Nested includes ------------*/

#pragma once

/*-- Symbolic Constant Macros (defines) --*/

/*-- Declarations (Statics , globals and externs) --*/

// Addresses defined to ensure compilation works
// The actual mailbox APIs are mocked and won't do anything
#define SCP_TOP_SCP2HSP_MAILBOX_ADDRESS (0x46000000U)
#define MCP_TOP_MCP2HSP_MAILBOX_ADDRESS (0x45640000U)

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
