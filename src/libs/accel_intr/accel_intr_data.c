//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr_data.c
 * This file contains DATA related to Accel IP interrupts handled in SCP
 *
 */

/*-------------------------------- Features ---------------------------------*/

/*-------------------------------- Includes ---------------------------------*/
#include "accel_intr_priv.h"    // for accel_intr_single_level_irq_init
#include "sdm_ext_interrupts.h" // for SDM_EXT_AXI_BURST_ERR_INTR, SDM_EX...

#include <cded_interrupts.h>
#include <cded_regs_regs_regs.h>
#include <fpfw_timer_port.h>
#include <stddef.h> // for NULL

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

// Store interrupt specific functions.
// When value is NULL, no action will be taken.
// clang-format off
const accel_intr_irq_data_t accel_irq_scp_data[ACCEL_SCP_INTR_MAX] = {

    [ACCEL_SCP_INTR_EMCPU_WDT_ERR] = {
        .accel_irq_bit = SDM_EXT_EMCPU_WDT_ERR_INTR,
        .accel_irq_init_fn = NULL,
        .accel_irq_fn = accel_intr_fatal_isr
    },

    [ACCEL_SCP_INTR_TCM_UE_ECC_ERR] =
        {
            .accel_irq_bit = SDM_EXT_TCM_UE_ECC_ERR_INTR,
            .accel_irq_init_fn = NULL,
            .accel_irq_fn = accel_intr_tcm_ue_emcpu_lockcup_isr
        },

    [ACCEL_SCP_INTR_EMCPU_LOCKUP_ERR] =
        {
            .accel_irq_bit = SDM_EXT_LOCKUP_ERR_INT,
            .accel_irq_init_fn = NULL,
            .accel_irq_fn = accel_intr_tcm_ue_emcpu_lockcup_isr
        },

    [ACCEL_SCP_INTR_SDM_MSG0_INTR] =
    {
        .accel_irq_bit = SDM_EXT_SDM_MSG0_INTR,
        .accel_irq_init_fn = NULL,
        .accel_irq_fn = accel_intr_fatal_isr
    },
};

// clang-format on

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/
