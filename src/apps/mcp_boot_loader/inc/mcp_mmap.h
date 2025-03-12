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
//! Makes the compiler happy!
#ifndef SL_1KB
#define SL_1KB KB
#endif
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
//! @todo Split the region for SCP & MCP to avoid future confusion
//! Total size of the region `SCP_EXP_RODATA_RMSS_REGION_BASE` is 256 KB, of which the initial 192 KB is used by SCP & remaining 64 KB is used for the MCP
#define MCP_RMSS_RAM_DATA_BASE       SCP_EXP_RODATA_RMSS_REGION_BASE + (192 * KB) // 0x013BF490UL
#define MCP_RMSS_RAM_DATA_SIZE      (64 * KB)

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/