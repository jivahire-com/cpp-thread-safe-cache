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

#include <fpfw_timer_port.h>
#include <stddef.h> // for NULL

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

// Store interrupt specific functions.
// When value is NULL, no action will be taken.
// clang-format off
const accel_intr_irq_data_t accel_intr_irq_data[ACCEL_INTR_MAX] = {

    // ACCEL_INTR_EMCPU_WDT_ERR
    {
        .accel_irq_bit = SDM_EXT_EMCPU_WDT_ERR_INTR,
        .accel_irq_init_fn = accel_intr_emcpu_wdt_err_init,
        .accel_irq_fn = accel_intr_emcpu_wdt_err_fn
    },

    // ACCEL_INTR_CP_FATAL_ERR
    {
        .accel_irq_bit = SDM_EXT_CP_FATAL_ERR_INTR,
        .accel_irq_init_fn = accel_intr_single_level_irq_init,
        .accel_irq_fn = accel_intr_single_level_irq_fn
    },

    // ACCEL_INTR_UE_ECC_ERR
    {
        .accel_irq_bit = SDM_EXT_UE_ECC_ERR_INTR,
        .accel_irq_init_fn = accel_intr_ue_ecc_err_init,
        .accel_irq_fn = accel_intr_ue_ecc_err_fn
    },

    // ACCEL_INTR_CSR_PARITY_ERR
    {
        .accel_irq_bit = SDM_EXT_CSR_PARITY_ERR_INTR,
        .accel_irq_init_fn = accel_intr_single_level_irq_init,
        .accel_irq_fn = accel_intr_single_level_irq_fn
    },

    // ACCEL_INTR_SDM_WDT_ERR
    {
        .accel_irq_bit = SDM_EXT_SDM_WDT_ERR_INTR,
        .accel_irq_init_fn = accel_intr_sdm_wdt_err_init,
        .accel_irq_fn = accel_intr_sdm_wdt_err_fn
    },

    // ACCEL_INTR_FAB_WDT_ERR
    {
        .accel_irq_bit = SDM_EXT_FAB_WDT_ERR_INTR,
        .accel_irq_init_fn = accel_intr_fab_wdt_err_init,
        .accel_irq_fn = accel_intr_fab_wdt_err_fn
    },

    // ACCEL_INTR_AXI_BURST_ERR
    {
        .accel_irq_bit = SDM_EXT_AXI_BURST_ERR_INTR,
        .accel_irq_init_fn = accel_intr_single_level_irq_init,
        .accel_irq_fn = accel_intr_single_level_irq_fn
    },

    // ACCEL_INTR_AXI_UNSUPP_INTR_STATUS
    {
        .accel_irq_bit = SDM_EXT_AXI_UNSUPP_INTR_STATUS_INTR,
        .accel_irq_init_fn = accel_intr_single_level_irq_init,
        .accel_irq_fn = accel_intr_single_level_irq_fn
    },

    // ACCEL_INTR_STYRESE_REQ_ERR
    {
        .accel_irq_bit = SDM_EXT_STYRESE_REQ_ERR_INTR,
        .accel_irq_init_fn = accel_intr_single_level_irq_init,
        .accel_irq_fn = accel_intr_single_level_irq_fn
    }
};
// clang-format on

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/

eACCEL_INTR accel_intr_get_accel_intr_enum_from_irq_bit(SDM_EXT_INTERRUPT_NUMBER irq_bit)
{
    switch (irq_bit)
    {
    case SDM_EXT_EMCPU_WDT_ERR_INTR:
        return ACCEL_INTR_EMCPU_WDT_ERR;
    case SDM_EXT_CP_FATAL_ERR_INTR:
        return ACCEL_INTR_CP_FATAL_ERR;
    case SDM_EXT_UE_ECC_ERR_INTR:
        return ACCEL_INTR_UE_ECC_ERR;
    case SDM_EXT_CSR_PARITY_ERR_INTR:
        return ACCEL_INTR_CSR_PARITY_ERR;
    case SDM_EXT_SDM_WDT_ERR_INTR:
        return ACCEL_INTR_SDM_WDT_ERR;
    case SDM_EXT_FAB_WDT_ERR_INTR:
        return ACCEL_INTR_FAB_WDT_ERR;
    case SDM_EXT_AXI_BURST_ERR_INTR:
        return ACCEL_INTR_AXI_BURST_ERR;
    case SDM_EXT_AXI_UNSUPP_INTR_STATUS_INTR:
        return ACCEL_INTR_AXI_UNSUPP_INTR_STATUS;
    case SDM_EXT_STYRESE_REQ_ERR_INTR:
        return ACCEL_INTR_STYRESE_REQ_ERR;
    default:
        return ACCEL_INTR_MAX;
    }
}
