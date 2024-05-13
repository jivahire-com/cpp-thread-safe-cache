//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mock_scp_mmap.h
 * Header file containing extern declarations for various SCP memory locations
 */

/*----------- Nested includes ------------*/

#pragma once

/*-- Symbolic Constant Macros (defines) --*/

/*-- Declarations (Statics , globals and externs) --*/

extern void *SCP_MSCP_EXP_SRAM0_ADDR;

extern uint32_t SCP_TOP_SCP_INST_RAM_SIZE;
extern uint32_t SCP_DTCM_RAM_SIZE;

extern uint8_t *SCP_TOP_SCP_INST_RAM_ADDRESS;
extern uint8_t *SCP_TOP_SCP_DATA_RAM_ADDRESS;

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
