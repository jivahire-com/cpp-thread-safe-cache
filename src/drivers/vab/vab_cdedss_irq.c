//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Service routines for VAB RPSS domain.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FPFwInterrupts.h>
#include <FpFwAssert.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <interrupts.h>
#include <intu_lib.h>
#include <kng_atu_mappings.h>
#include <kng_soc_constants.h>
#include <pcie_irq.h>
#include <silibs_common.h>
#include <silibs_platform.h>
#include <silibs_status.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <tower_isr.h>
#include <vab.h>
#include <vab_events.h>
#include <vab_intu.h>
#include <vab_irq.h>
#include <vab_regs.h>

/*-- Symbolic Constant Macros (defines) --*/
#define RAS_IOSS_TOWER_OFFSET                                               \
    (VAB_CDED_IOSS_TOP_IOSS_ADDRESS + IOSS_TOP_NOCNI_NITOWER_IOSS_ADDRESS + \
     NOCNI_NITOWER_IOSS_AXI5_TARGET_IOSS_OB_AXI_S_MM_NOCNI_NITOWER_IOSS_2_ADDRESS)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static silibs_status_t ioss_tower_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base);

/*-- Declarations (Statics and globals) --*/

/*
 * TODO:
 * We need to track what has been handled in each sub-handler since it
 * can service multiple indices at once.
 */
vab_generic_handler_t vab_cdedss_ioss_intu0_handlers[VAB_CDEDSS_INTU0_NUM_CFGS] = {
    NULL,                         // VAB_CDEDSS_IOSS_INTU0_VAB5_NITOWER_NS_INT
    NULL,                         // VAB_CDEDSS_IOSS_INTU0_VAB5_NITOWER_S_INT
    vab_tcu_handler,              // VAB_CDEDSS_IOSS_INTU0_VAB5_TCU_RAS_CRI
    vab_tbu0_handler,             // VAB_CDEDSS_IOSS_INTU0_VAB5_TBU0_RAS_CRI
    vab_tbu1_handler,             // VAB_CDEDSS_IOSS_INTU0_VAB5_TBU1_RAS_CRI
    NULL,                         // VAB_CDEDSS_IOSS_INTU0_VAB5_FABRIC_PARITY_CRI
    vab_tower_handler,            // VAB_CDEDSS_IOSS_INTU0_VAB5_NITOWER_CRI
    vab_ras_node_handler,         // VAB_CDEDSS_IOSS_INTU0_VAB5_RAS_CRI
    vab_csr_parity_handler,       // VAB_CDEDSS_IOSS_INTU0_VAB5_CSR_PARITY_CRI
    vab_pcr_parity_handler,       // VAB_CDEDSS_IOSS_INTU0_VAB5_PCR_PARITY_CRI
    NULL,                         // VAB_CDEDSS_IOSS_INTU0_VAB5_HW_ASSERT_CRI
    NULL,                         // VAB_CDEDSS_IOSS_INTU0_VAB5_TBU_CRIT_ERR
    NULL,                         // VAB_CDEDSS_IOSS_INTU0_CDEDSS_DBG_CRI
    NULL,                         // VAB_CDEDSS_IOSS_INTU0_UNUSED_INT_PIN_13
    NULL,                         // VAB_CDEDSS_IOSS_INTU0_UNUSED_INT_PIN_14
    NULL,                         // VAB_CDEDSS_IOSS_INTU0_CDEDSS_NITOWER_NS_INT
    NULL,                         // VAB_CDEDSS_IOSS_INTU0_CDEDSS_NITOWER_S_INT
    vab_integ_pcr_parity_handler, // VAB_CDEDSS_IOSS_INTU0_CDEDSS_PCR_PARITY_CRI
    vab_integ_csr_parity_handler, // VAB_CDEDSS_IOSS_INTU0_CDEDSS_CSR_PARITY_CRI
    NULL,                         // VAB_CDEDSS_IOSS_INTU0_UNUSED_INT_PIN_19
    NULL,                         // VAB_CDEDSS_IOSS_INTU0_CDEDSS_NITOWER_CRI
    NULL,                         // VAB_CDEDSS_IOSS_INTU0_CDEDSS_HW_ASSERT_CRI
    NULL,                         // VAB_CDEDSS_IOSS_INTU0_UNUSED_INT_PIN_22
    NULL,                         // VAB_CDEDSS_IOSS_INTU0_UNUSED_INT_PIN_23
    NULL,                         // VAB_CDEDSS_IOSS_INTU0_UNUSED_INT_PIN_24
    NULL,                         // VAB_CDEDSS_IOSS_INTU0_IOSS_NITOWER_NS_INT
    NULL,                         // VAB_CDEDSS_IOSS_INTU0_IOSS_NITOWER_S_INT
    vab_integ_pcr_parity_handler, // VAB_CDEDSS_IOSS_INTU0_IOSS_PCR_PARITY_CRI
    vab_integ_csr_parity_handler, // VAB_CDEDSS_IOSS_INTU0_IOSS_CSR_PARITY_CRI
    NULL,                         // VAB_CDEDSS_IOSS_INTU0_UNUSED_INT_PIN_29
    ioss_tower_handler,           // VAB_CDEDSS_IOSS_INTU0_IOSS_NITOWER_CRI
    NULL,                         // VAB_CDEDSS_IOSS_INTU0_IOSS_HW_ASSERT_CRI
};

vab_generic_handler_t vab_cdedss_ioss_intu1_handlers[VAB_CDEDSS_INTU1_NUM_CFGS] = {
    vab_tower_handler,    // VAB_CDEDSS_IOSS_INTU1_VAB5_NITOWER_FHI
    vab_tower_handler,    // VAB_CDEDSS_IOSS_INTU1_VAB5_NITOWER_ERI
    vab_tcu_handler,      // VAB_CDEDSS_IOSS_INTU1_VAB5_TCU_GIC_RAS_FHI
    vab_tcu_handler,      // VAB_CDEDSS_IOSS_INTU1_VAB5_TCU_GIC_RAS_ERI
    vab_tbu0_handler,     // VAB_CDEDSS_IOSS_INTU1_VAB5_TBU0_GIC_RAS_FHI
    vab_tbu0_handler,     // VAB_CDEDSS_IOSS_INTU1_VAB5_TBU0_GIC_RAS_ERI
    vab_tbu1_handler,     // VAB_CDEDSS_IOSS_INTU1_VAB5_TBU1_GIC_RAS_FHI
    vab_tbu1_handler,     // VAB_CDEDSS_IOSS_INTU1_VAB5_TBU1_GIC_RAS_ERI
    vab_ras_node_handler, // VAB_CDEDSS_IOSS_INTU1_VAB5_RAS_FHI
    vab_ras_node_handler, // VAB_CDEDSS_IOSS_INTU1_VAB5_RAS_ERI
    NULL,                 // VAB_CDEDSS_IOSS_INTU1_CDEDSS_NITOWER_FHI
    NULL,                 // VAB_CDEDSS_IOSS_INTU1_CDEDSS_NITOWER_ERI
    NULL,                 // VAB_CDEDSS_IOSS_INTU1_UNUSED_INT_PIN_12
    NULL,                 // VAB_CDEDSS_IOSS_INTU1_UNUSED_INT_PIN_13
    ioss_tower_handler,   // VAB_CDEDSS_IOSS_INTU1_IOSS_NITOWER_FHI
    ioss_tower_handler,   // VAB_CDEDSS_IOSS_INTU1_IOSS_NITOWER_ERI
};

/*------------- Functions ----------------*/
static silibs_status_t ioss_tower_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base)
{
    return tower_fmu_handler(TOWER_SDMSS, (vab_id == D0_VAB5_CDEDSS_IOSS) ? SOC_D0 : SOC_D1, base + RAS_IOSS_TOWER_OFFSET);
}

void process_vab_cdedss_probe(vab_isr_ctx_t* ctx)
{
    vabss_int_probe_t* probe = &ctx->probe;
    for (uint8_t idx = 0; idx < probe->num_intu0_pins; idx++)
    {
        if (probe->intu0[idx].asserted)
        {
            if (vab_cdedss_ioss_intu0_handlers[idx])
            {
                vab_cdedss_ioss_intu0_handlers[idx](ctx->vab_id, ctx->vab_base);
            }
            else
            {
                VAB_ET_WARNING_PARAM(VAB_ET_TYPE_CDEDSS_INTU0_UNSUPPORTED, idx);
                FPFW_DBGPRINT_ALWAYS(
                    "|VAB_CDEDSS_IOSS| INTU0 Index %d for VAB %d is not supported! Ignoring...\n",
                    idx,
                    ctx->vab_id);
            }
        }
    }

    for (uint8_t idx = 0; idx < probe->num_intu1_pins; idx++)
    {
        if (probe->intu1[idx].asserted)
        {
            if (vab_cdedss_ioss_intu1_handlers[idx])
            {
                vab_cdedss_ioss_intu1_handlers[idx](ctx->vab_id, ctx->vab_base);
            }
            else
            {
                VAB_ET_WARNING_PARAM(VAB_ET_TYPE_CDEDSS_INTU1_UNSUPPORTED, idx);
                FPFW_DBGPRINT_ALWAYS(
                    "|VAB_CDEDSS_IOSS| INTU1 Index %d for VAB %d is not supported! Ignoring...\n",
                    idx,
                    ctx->vab_id);
            }
        }
    }
}
