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
#include <mscp_exp_rmss_memory_map.h>

#pragma once

/*-- Symbolic Constant Macros (defines) --*/

#define KB                          (1024)
#define MCP_VTOR_ALIGNMENT_OFFSET   0x80 //Bits 0-6 of VTOR register are reserved in cortex-M7

#define MCP_BL_DATA_SECTION_SIZE    (32 * KB) //Lower 32KB of DTCM is used the data, bss, heap and stack of bootloader
#define MCP_ITCM_RAM_BASE           (MCP_TOP_MCP_INST_RAM_ADDRESS + MCP_VTOR_ALIGNMENT_OFFSET)
#define MCP_ITCM_RAM_SIZE           (MCP_TOP_MCP_INST_RAM_SIZE - MCP_VTOR_ALIGNMENT_OFFSET)
#define MCP_DTCM_RAM_BASE           (MCP_TOP_MCP_DATA_RAM_ADDRESS)
#define MCP_DTCM_RAM_SIZE           (MCP_TOP_MCP_DATA_RAM_SIZE - MCP_BL_DATA_SECTION_SIZE)   
#define MCP_MSCP_EXP_SRAM0_ADDR     (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM0_ADDRESS)
#define MCP_MSCP_EXP_SRAM1_ADDR     (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM1_ADDRESS)
#define MCP_BOOT_RAM_BASE            MCP_MSCP_EXP_SRAM0_ADDR 
#define MCP_MAX_IMAGE_SIZE          (450 * KB) // MSCP_EXP RAM each slot is 1MB and with ITCM/DTCM each 512KB, the compressed main image of FW could be this value
                                               // in combined elf with bootloader
#define MCP_RMSS_RAM_DATA_BASE      MCP_EXP_RODATA_RMSS_REGION_BASE
#define MCP_RMSS_RAM_DATA_SIZE      MCP_EXP_RODATA_RMSS_REGION_SIZE

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/