//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_gpio.c
 * This file contains the implementation for the DDR Manager GPIO APIs
 *
 */

/*------------- Includes -----------------*/
#include "ddr_manager_i.h"

#include <AtomicOperations.h> // for FPFwAtomicCallDataMemoryBarrier
#include <scp_exp_csr_regs.h>
#include <silibs_platform.h>
#include <silibs_scp_exp_top_regs.h>
#include <silibs_scp_top_regs.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MEM_HOT_GPIO      BIT1
#define THERMAL_TRIP_GPIO BIT2

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static vptr_scp_exp_csr_reg scp_exp_csr_regs =
    (vptr_scp_exp_csr_reg)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS);

/*------------- Functions ----------------*/

/**
 * @brief Sets the DDR thermal trip GPIO
 *      This will trigger the thermal trip event to CPLD/BMC and is considered catastrophic/fatal
 */
void ddr_manager_set_thermal_trip_gpio()
{
    uint32_t reg_val = MMIO_READ32(&scp_exp_csr_regs->thermal_io_reg);
    reg_val |= THERMAL_TRIP_GPIO;
    MMIO_WRITE32(&scp_exp_csr_regs->thermal_io_reg, reg_val);
    FPFwAtomicCallDataMemoryBarrier();
}

void ddr_manager_set_memhot_gpio()
{
    // Assert the GPIO pin to trigger the memory hot event to CPLD/BMC

    uint32_t reg_val = MMIO_READ32(&scp_exp_csr_regs->thermal_io_reg);
    reg_val |= MEM_HOT_GPIO;
    MMIO_WRITE32(&scp_exp_csr_regs->thermal_io_reg, reg_val);
    FPFwAtomicCallDataMemoryBarrier();
}

void ddr_manager_clear_memhot_gpio()
{
    // Deassert the GPIO pin to clear the memory hot event to CPLD/BMC

    uint32_t reg_val = MMIO_READ32(&scp_exp_csr_regs->thermal_io_reg);
    reg_val &= ~MEM_HOT_GPIO;
    MMIO_WRITE32(&scp_exp_csr_regs->thermal_io_reg, reg_val);
    FPFwAtomicCallDataMemoryBarrier();
}