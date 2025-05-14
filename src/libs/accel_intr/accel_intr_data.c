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

    [ACCEL_SCP_INTR_EMCPU_WDT_ERR] =
        {
            .accel_irq_bit = SDM_EXT_EMCPU_WDT_ERR_INTR,
            .accel_irq_init_fn = accel_intr_emcpu_wdt_err_init,
            .accel_irq_fn = accel_intr_emcpu_wdt_err_fn
        },

    [ACCEL_SCP_INTR_CP_FATAL_ERR] =
        {
            .accel_irq_bit = SDM_EXT_CP_FATAL_ERR_INTR,
            .accel_irq_init_fn = accel_intr_cded_cp_err_init,
            .accel_irq_fn = accel_intr_cded_cp_err_fn
        },

    [ACCEL_SCP_INTR_UE_ECC_ERR] =
        {
            .accel_irq_bit = SDM_EXT_UE_ECC_ERR_INTR,
            .accel_irq_init_fn = accel_intr_ue_ecc_err_init,
            .accel_irq_fn = accel_intr_ue_ecc_err_fn
        },

    [ACCEL_SCP_INTR_CSR_PARITY_ERR] =
        {
            .accel_irq_bit = SDM_EXT_CSR_PARITY_ERR_INTR,
            .accel_irq_init_fn = accel_intr_single_level_irq_init,
            .accel_irq_fn = accel_intr_single_level_irq_fn
        },

    [ACCEL_SCP_INTR_SDM_WDT_ERR] =
        {
            .accel_irq_bit = SDM_EXT_SDM_WDT_ERR_INTR,
            .accel_irq_init_fn = accel_intr_sdm_wdt_err_init,
            .accel_irq_fn = accel_intr_sdm_wdt_err_fn
        },

    [ACCEL_SCP_INTR_FAB_WDT_ERR] =
        {
            .accel_irq_bit = SDM_EXT_FAB_WDT_ERR_INTR,
            .accel_irq_init_fn = accel_intr_fab_wdt_err_init,
            .accel_irq_fn = accel_intr_fab_wdt_err_fn
        },

    [ACCEL_SCP_INTR_AXI_UNSUPP_INTR_STATUS] =
        {
            .accel_irq_bit = SDM_EXT_AXI_UNSUPP_INTR_STATUS_INTR,
            .accel_irq_init_fn = accel_intr_single_level_irq_init,
            .accel_irq_fn = accel_intr_single_level_irq_fn
        },

    [ACCEL_SCP_INTR_MBX_I2E] =
        {
            .accel_irq_bit = SDM_EXT_MBX_I2E_INTR,
            .accel_irq_init_fn = accel_intr_single_level_irq_init,
            .accel_irq_fn = NULL,
        },
};

const accel_intr_irq_data_t accel_irq_mcp_data[ACCEL_MCP_INTR_MAX] = {
    [ACCEL_MCP_INTR_MBX_I2E] =
        {
            .accel_irq_bit = SDM_EXT_MBX_I2E_INTR,
            .accel_irq_init_fn = accel_intr_single_level_irq_init,
            .accel_irq_fn = NULL,
        },
};

/**
 * @brief List of level 3 category id for CCMP_FATAL_INT
 *
 */
static const CDED_INT_CATEGORY ccmp_f_cat_arr[CDED_REGS_CCMP_CFG_REGS_ARRAY_COUNT] = {
    CDED_CATEGORY_ID_COMPRESSOR0_FATAL,
    CDED_CATEGORY_ID_COMPRESSOR1_FATAL,
};

/**
 * @brief List of level 3 category id for DCMP_FATAL_INT
 *
 */
static const CDED_INT_CATEGORY dcmp_f_cat_arr[CDED_REGS_DCMP_CFG_REGS_ARRAY_COUNT] = {
    CDED_CATEGORY_ID_DECOMPRESSOR0_FATAL,
    CDED_CATEGORY_ID_DECOMPRESSOR1_FATAL,
    CDED_CATEGORY_ID_DECOMPRESSOR2_FATAL,
    CDED_CATEGORY_ID_DECOMPRESSOR3_FATAL,
    CDED_CATEGORY_ID_DECOMPRESSOR4_FATAL,
    CDED_CATEGORY_ID_DECOMPRESSOR5_FATAL,
    CDED_CATEGORY_ID_DECOMPRESSOR6_FATAL,
    CDED_CATEGORY_ID_DECOMPRESSOR7_FATAL,
};

/**
 * @brief List of level 3 category id for AES_FATAL_INT
 *
 */
static const CDED_INT_CATEGORY aes_f_cat_arr[1] = {
    CDED_CATEGORY_ID_AES_FATAL,
};

/**
 * @brief Below list represents configuration "CDED configuration" interrupts
 *
 * Note: .brief string should not exceed length of (CDED_CP_INT_TRACE_STR_LEN - 1)
 */
const cded_cp_l2_irq_data_t cded_cp_fatal_intr[] = {
    {
        .bit_index = CCMP_FATAL_INT,
        .l3_irq_data = ccmp_f_cat_arr,
        .l3_irq_data_count = ARRAY_SIZE(ccmp_f_cat_arr),
        .brief = "fatal compression",
    },

    {
        .bit_index = DCMP_FATAL_INT,
        .l3_irq_data = dcmp_f_cat_arr,
        .l3_irq_data_count = ARRAY_SIZE(dcmp_f_cat_arr),
        .brief = "fatal decompression",
    },

    {
        .bit_index = AES_FATAL_INT,
        .l3_irq_data = aes_f_cat_arr,
        .l3_irq_data_count = ARRAY_SIZE(aes_f_cat_arr),
        .brief = "fatal AES",
    },

    {
        .bit_index = KMP_FATAL_ERR,
        .brief = "fatal KMP",
    },

    {
        .bit_index = IC_XB_PERR,
        .brief = "IC XB PERR",
    },

    {
        .bit_index = CDED_CFG_PERR,
        .brief = "CDED CFG PERR",
    },

    {
        .bit_index = IC_FSM_ERR,
        .brief = "IC FSM ERR",
    },

    {
        .bit_index = IC_CP_ECC_DED,
        .brief = "IC CP ECC DED",
    },

    {
        .bit_index = IC_XB_INVALID,
        .brief = "IC XB INVALID",
    },

    {
        .bit_index = IC_AXI_ERR,
        .brief = "IC AXI ERR",
    },
    // Sentinel condition
    {
        .bit_index = CDED_CFG_MAX_INTERRUPT,
        .brief = "",
    },
};

// clang-format on

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/

eACCEL_SCP_INTR accel_scp_get_intr_enum(SDM_EXT_INTERRUPT_NUMBER irq_bit)
{
    switch (irq_bit)
    {
    case SDM_EXT_EMCPU_WDT_ERR_INTR:
        return ACCEL_SCP_INTR_EMCPU_WDT_ERR;
    case SDM_EXT_CP_FATAL_ERR_INTR:
        return ACCEL_SCP_INTR_CP_FATAL_ERR;
    case SDM_EXT_UE_ECC_ERR_INTR:
        return ACCEL_SCP_INTR_UE_ECC_ERR;
    case SDM_EXT_CSR_PARITY_ERR_INTR:
        return ACCEL_SCP_INTR_CSR_PARITY_ERR;
    case SDM_EXT_SDM_WDT_ERR_INTR:
        return ACCEL_SCP_INTR_SDM_WDT_ERR;
    case SDM_EXT_FAB_WDT_ERR_INTR:
        return ACCEL_SCP_INTR_FAB_WDT_ERR;
    case SDM_EXT_AXI_UNSUPP_INTR_STATUS_INTR:
        return ACCEL_SCP_INTR_AXI_UNSUPP_INTR_STATUS;
    case SDM_EXT_MBX_I2E_INTR:
        return ACCEL_SCP_INTR_MBX_I2E;
    default:
        return ACCEL_SCP_INTR_MAX;
    }
}
