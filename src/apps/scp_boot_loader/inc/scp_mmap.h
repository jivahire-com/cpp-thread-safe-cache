//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_mmap.h
 * Header file containing macros for various SCP memory locations
 */

/*----------- Nested includes ------------*/

#include <scp_exp_top_regs.h>
#include <scp_top_regs.h>

#pragma once

/*-- Symbolic Constant Macros (defines) --*/

#define KB                          (1024)
#define SCP_ITCM_RAM_BASE           SCP_TOP_SCP_INST_RAM_ADDRESS
#define SCP_ITCM_RAM_SIZE           SCP_TOP_SCP_INST_RAM_SIZE
#define SCP_DTCM_RAM_BASE           SCP_TOP_SCP_DATA_RAM_ADDRESS
#define SCP_DTCM_RAM_SIZE           SCP_TOP_SCP_DATA_RAM_SIZE
#define SCP_MSCP_EXP_SRAM0_ADDR     (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS)
#define SCP_MSCP_EXP_SRAM0_ADDR     (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS)
#define SCP_FMW_MAX_STACK_SIZE      (4 * KB)

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
