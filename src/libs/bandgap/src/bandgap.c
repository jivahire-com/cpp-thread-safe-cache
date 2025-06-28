//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file bandgap.c
 * This file contains functions to configure and control the bandgap reference circuitry.
 * It provides functionality to set trim values and enable/disable the bandgap circuit.
 */

/*------------- Includes -----------------*/
#include "bandgap.h"

#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <scp_top_regs.h>
#include <silibs_platform.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*------------- Functions ----------------*/
void configure_bandgap_trim(uint32_t slope_trim, uint32_t offset_trim)
{
    uint32_t* ref_macro_trim_reg_addr =
        (uint32_t*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_REF_MACRO_TRIM_REG_ADDRESS);
    scp_exp_csr_ref_macro_trim_reg ref_macro_trim = {{0}};
    ref_macro_trim.slope_trim = slope_trim;
    ref_macro_trim.offset_trim = offset_trim;
    MMIO_WRITE32(ref_macro_trim_reg_addr, ref_macro_trim.as_uint32);
}

void enable_bandgap_circuitry(bool enable)
{
    uint32_t* ref_macro_ctrl_reg_addr =
        (uint32_t*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_REF_MACRO_CTRL_REG_ADDRESS);
    scp_exp_csr_ref_macro_ctrl_reg ref_macro_ctrl;
    ref_macro_ctrl.as_uint32 = MMIO_READ32(ref_macro_ctrl_reg_addr);

    if (enable)
    {
        ref_macro_ctrl.enable = 1;
    }
    else
    {
        ref_macro_ctrl.enable = 0;
    }

    MMIO_WRITE32(ref_macro_ctrl_reg_addr, ref_macro_ctrl.as_uint32);
}