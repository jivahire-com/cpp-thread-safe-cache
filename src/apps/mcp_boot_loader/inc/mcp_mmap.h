//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mcp_mmap.h
 * Header file containing macros for various MCP memory locations
 */

/*----------- Nested includes ------------*/

#include <mcp_exp_top_regs.h>
#include <mcp_top_regs.h>

#pragma once

/*-- Symbolic Constant Macros (defines) --*/

#define KB                          (1024)
#define MCP_ITCM_RAM_BASE           MCP_TOP_MCP_INST_RAM_ADDRESS
#define MCP_ITCM_RAM_SIZE           MCP_TOP_MCP_INST_RAM_SIZE
#define MCP_DTCM_RAM_BASE           MCP_TOP_MCP_DATA_RAM_ADDRESS
#define MCP_DTCM_RAM_SIZE           MCP_TOP_MCP_DATA_RAM_SIZE
#define MCP_MSCP_EXP_SRAM0_ADDR     (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM0_ADDRESS)
#define MCP_MSCP_EXP_SRAM1_ADDR     (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM1_ADDRESS)
#define MCP_FMW_MAX_STACK_SIZE      (4 * KB)

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/