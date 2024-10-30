//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_err_inj.h
 * Header file to support implementations of ddr CLI commands.
 */


/*------------- Includes -----------------*/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>


/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

void ddrss_err_inj_atu_map(uint32_t die_num);
void ddrss_err_inj_atu_unmap();
void ddrss_ue_ce_error_injection(int32_t die_num, uint32_t mc, uint64_t p_addr, uint16_t Bit);
bool ddrss_ue_ce_err_inj_validation(uint32_t mc, uint16_t BIT);