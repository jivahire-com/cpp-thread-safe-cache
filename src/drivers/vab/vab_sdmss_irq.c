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
#include <mesh_error_handler.h>
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
#include <vab_init.h>
#include <vab_intu.h>
#include <vab_irq.h>
#include <vab_regs.h>
#include <vab_sdm_top_regs.h>

/*-- Symbolic Constant Macros (defines) --*/
#define RAS_SDMSS_TOWER_OFFSET \
    (VAB_SDM_TOP_SDMSS_ADDRESS + SDMSS_CONFIG_SDMSS_TOWER_ADDRESS + NOCNI_NITOWER_SDM_ACE5LITE_TARGET_VAB_AXI_MM_NOCNI_NITOWER_SDM_2_ADDRESS)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static silibs_status_t d2d_isr(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base);
static silibs_status_t sdmss_tower_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base);
static silibs_status_t d2dss_cfg0_tower_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base);
static silibs_status_t d2dss_cfg1_tower_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base);

/*-- Declarations (Statics and globals) --*/
/* TODO: We need to track what has been handled in each sub-handler since it can service multiple indices at once */
vab_generic_handler_t vab_sdmss_intu0_handlers[VAB_SDMSS_INTU0_NUM_CFGS] = {
    NULL,                         // VAB_SDMSS_INTU0_VAB4_NITOWER_NS_INT
    NULL,                         // VAB_SDMSS_INTU0_VAB4_NITOWER_S_INT
    vab_tcu_handler,              // VAB_SDMSS_INTU0_VAB4_TCU_RAS_CRI
    vab_tbu0_handler,             // VAB_SDMSS_INTU0_VAB4_TBU0_RAS_CRI
    vab_tbu1_handler,             // VAB_SDMSS_INTU0_VAB4_TBU1_RAS_CRI
    vab_fabric_parity_handler,    // VAB_SDMSS_INTU0_VAB4_FABRIC_PARITY_CRI
    vab_tower_handler,            // VAB_SDMSS_INTU0_VAB4_NITOWER_CRI
    vab_ras_node_handler,         // VAB_SDMSS_INTU0_VAB4_RAS_CRI
    vab_csr_parity_handler,       // VAB_SDMSS_INTU0_VAB4_CSR_PARITY_CRI
    vab_pcr_parity_handler,       // VAB_SDMSS_INTU0_VAB4_PCR_PARITY_CRI
    NULL,                         // VAB_SDMSS_INTU0_VAB4_HW_ASSERT_CRI
    NULL,                         // VAB_SDMSS_INTU0_VAB4_TBU_CRIT_ERR
    vab_integ_pcr_parity_handler, // VAB_SDMSS_INTU0_SDMSS_PCR_PARITY_CRI
    vab_integ_csr_parity_handler, // VAB_SDMSS_INTU0_SDMSS_CSR_PARITY_CRI
    NULL,                         // VAB_SDMSS_INTU0_SDMSS_HW_ASSERT_CRI
    sdmss_tower_handler,          // VAB_SDMSS_INTU0_SDMSS_NITOWER_CRI
    NULL,                         // VAB_SDMSS_INTU0_SDMSS_NITOWER_NS_INT
    NULL,                         // VAB_SDMSS_INTU0_SDMSS_NITOWER_S_INT
    NULL,                         // VAB_SDMSS_INTU0_D2DSS_PCR0_PLL_ERROR_INT
    vab_integ_pcr_parity_handler, // VAB_SDMSS_INTU0_D2DSS_PCR0_CSR_PARITY_INT
    d2dss_cfg0_tower_handler,     // VAB_SDMSS_INTU0_D2DSS_CFG0_NITOWER_CRI
    NULL,                         // VAB_SDMSS_INTU0_D2DSS_CFG0_NITOWER_NS_INT
    NULL,                         // VAB_SDMSS_INTU0_D2DSS_CFG0_NITOWER_S_INT
    d2d_isr,                      // VAB_SDMSS_INTU0_D2DSS_0_3_RAS_CRI
    d2d_isr,                      // VAB_SDMSS_INTU0_D2DSS_0_3_RAS_FHI
    NULL,                         // VAB_SDMSS_INTU0_D2DSS_PCR1_PLL_ERROR_INT
    vab_integ_pcr_parity_handler, // VAB_SDMSS_INTU0_D2DSS_PCR1_CSR_PARITY_INT
    d2dss_cfg1_tower_handler,     // VAB_SDMSS_INTU0_D2DSS_CFG1_NITOWER_CRI
    NULL,                         // VAB_SDMSS_INTU0_D2DSS_CFG1_NITOWER_NS_INT
    NULL,                         // VAB_SDMSS_INTU0_D2DSS_CFG1_NITOWER_S_INT
    d2d_isr,                      // VAB_SDMSS_INTU0_D2DSS_4_7_RAS_CRI
    d2d_isr,                      // VAB_SDMSS_INTU0_D2DSS_4_7_RAS_FHI
};

vab_generic_handler_t vab_sdmss_intu1_handlers[VAB_SDMSS_INTU1_NUM_CFGS] = {
    vab_tower_handler,        // VAB_SDMSS_INTU1_VAB4_NITOWER_FHI
    vab_tower_handler,        // VAB_SDMSS_INTU1_VAB4_NITOWER_ERI
    vab_tcu_handler,          // VAB_SDMSS_INTU1_VAB4_TCU_GIC_RAS_FHI
    vab_tcu_handler,          // VAB_SDMSS_INTU1_VAB4_TCU_GIC_RAS_ERI
    vab_tbu0_handler,         // VAB_SDMSS_INTU1_VAB4_TBU0_GIC_RAS_FHI
    vab_tbu0_handler,         // VAB_SDMSS_INTU1_VAB4_TBU0_GIC_RAS_ERI
    vab_tbu1_handler,         // VAB_SDMSS_INTU1_VAB4_TBU1_GIC_RAS_FHI
    vab_tbu1_handler,         // VAB_SDMSS_INTU1_VAB4_TBU1_GIC_RAS_ERI
    vab_ras_node_handler,     // VAB_SDMSS_INTU1_VAB4_RAS_FHI
    vab_ras_node_handler,     // VAB_SDMSS_INTU1_VAB4_RAS_ERI
    sdmss_tower_handler,      // VAB_SDMSS_INTU1_SDMSS_NITOWER_FHI
    sdmss_tower_handler,      // VAB_SDMSS_INTU1_SDMSS_NITOWER_ERI
    d2dss_cfg0_tower_handler, // VAB_SDMSS_INTU1_D2DSS_CFG0_NITOWER_FHI
    d2dss_cfg0_tower_handler, // VAB_SDMSS_INTU1_D2DSS_CFG0_NITOWER_ERI
    d2dss_cfg1_tower_handler, // VAB_SDMSS_INTU1_D2DSS_CFG1_NITOWER_FHI
    d2dss_cfg1_tower_handler, // VAB_SDMSS_INTU1_D2DSS_CFG1_NITOWER_ERI
};

/*------------- Functions ----------------*/

static silibs_status_t d2d_isr(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base)
{
    /*
     * The D2D error handler is indiscriminate to D2D instance and does
     * not reside within the VAB subsystem.
     */
    FPFW_UNUSED(vab_id);
    FPFW_UNUSED(base);
    d2d_error_isr(NULL);
    return SILIBS_SUCCESS;
}

static silibs_status_t sdmss_tower_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base)
{
    return tower_fmu_handler(TOWER_SDMSS, (vab_id == D0_VAB4_SDMSS) ? SOC_D0 : SOC_D1, base + RAS_SDMSS_TOWER_OFFSET);
}

static silibs_status_t d2dss_cfg0_tower_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base)
{
    FPFW_UNUSED(base);
    return tower_fmu_handler(TOWER_D2DSS0, (vab_id == D0_VAB4_SDMSS) ? SOC_D0 : SOC_D1, 0);
}

static silibs_status_t d2dss_cfg1_tower_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base)
{
    FPFW_UNUSED(base);
    return tower_fmu_handler(TOWER_D2DSS1, (vab_id == D0_VAB4_SDMSS) ? SOC_D0 : SOC_D1, 0);
}

void process_vab_sdmss_probe(vab_isr_ctx_t* ctx)
{
    vabss_int_probe_t* probe = &ctx->probe;
    for (uint8_t idx = 0; idx < probe->num_intu0_pins; idx++)
    {
        if (probe->intu0[idx].asserted)
        {
            if (vab_sdmss_intu0_handlers[idx])
            {
                vab_sdmss_intu0_handlers[idx](ctx->vab_id, ctx->vab_base);
            }
            else
            {
                VAB_ET_WARNING_PARAM(VAB_ET_TYPE_SDMSS_INTU0_UNSUPPORTED, idx);
                FPFW_DBGPRINT_ALWAYS("|VAB_SDMSS| INTU0 Index %d for VAB %d is not supported! Ignoring...\n", idx, ctx->vab_id);
            }
        }
    }

    for (uint8_t idx = 0; idx < probe->num_intu1_pins; idx++)
    {
        if (probe->intu1[idx].asserted)
        {
            if (vab_sdmss_intu1_handlers[idx])
            {
                vab_sdmss_intu1_handlers[idx](ctx->vab_id, ctx->vab_base);
            }
            else
            {
                VAB_ET_WARNING_PARAM(VAB_ET_TYPE_SDMSS_INTU1_UNSUPPORTED, idx);
                FPFW_DBGPRINT_ALWAYS("|VAB_SDMSS| INTU1 Index %d for VAB %d is not supported! Ignoring...\n", idx, ctx->vab_id);
            }
        }
    }
}
