//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_mmap.h
 * Header file containing macros for various SCP memory locations
 */

/*----------- Nested includes ------------*/
#include <assert.h>
#include <mscp_exp_rmss_memory_map.h>
#include <scp_exp_top_regs.h>
#include <scp_top_regs.h>

#pragma once

/*-- Symbolic Constant Macros (defines) --*/

#define KB                          (1024)
#define SCP_VTOR_ALIGNMENT_OFFSET   0x80 //Bits 0-6 of VTOR register are reserved in cortex-M7

#define SCP_BL_DATA_SECTION_SIZE  (32 * KB) //Lower 32KB of DTCM is used the data, bss, heap and stack of bootloader
#define SCP_ITCM_RAM_BASE           (SCP_TOP_SCP_INST_RAM_ADDRESS + SCP_VTOR_ALIGNMENT_OFFSET)
#define SCP_ITCM_RAM_SIZE           (SCP_TOP_SCP_INST_RAM_SIZE - SCP_VTOR_ALIGNMENT_OFFSET)
#define SCP_DTCM_RAM_BASE           (SCP_TOP_SCP_DATA_RAM_ADDRESS)
#define SCP_DTCM_RAM_SIZE           (SCP_TOP_SCP_DATA_RAM_SIZE - SCP_BL_DATA_SECTION_SIZE)
#define SCP_MSCP_EXP_SRAM0_ADDR     (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS)
#define SCP_MSCP_EXP_SRAM1_ADDR     (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS)
#define SCP_BOOT_RAM_BASE            SCP_MSCP_EXP_SRAM0_ADDR
#define SCP_MAX_IMAGE_SIZE          (572 * KB) // MSCP_EXP RAM each slot is 1MB and with ITCM/DTCM each 512KB, the compressed main image of FW could be this value
                                               // in combined elf with bootloader
static_assert(SCP_MAX_IMAGE_SIZE < (SCP_EXP_MSCP_BOOT_DATA_SIZE - 35 * KB), "SCP_MAX_IMAGE_SIZE should be smaller than SCP_EXP_MSCP_BOOT_DATA_SIZE to fit in the reserved boot data region");

#define SCP_RMSS_RAM_DATA_BASE      SCP_EXP_RODATA_RMSS_REGION_BASE
#define SCP_RMSS_RAM_DATA_SIZE      SCP_EXP_RODATA_RMSS_REGION_SIZE
/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
