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
#include <cper.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <interrupts.h>
#include <intu_lib.h>
#include <kng_atu_mappings.h>
#include <kng_soc_constants.h>
#include <pcie_irq.h>
#include <pciess.h>
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
#include <vab_rpss_top_regs.h>

/*-- Symbolic Constant Macros (defines) --*/
#define RAS_RPSS_OFFSET \
    (VAB_RPSS_TOP_RPSS_ADDRESS + RPSS_ADDR_MAP_RAS_NODE_ADDRESS + RPSS_ERG0_ERR0FR_LO_ADDRESS)
#define VAB_RPSS_OFFSET (VAB_RPSS_TOP_RPSS_ADDRESS)
#define RAS_RPSS_TOWER_OFFSET                                       \
    (VAB_RPSS_TOP_RPSS_ADDRESS + RPSS_ADDR_MAP_RPSS_TOWER_ADDRESS + \
     NOCNI_NITOWER_RPSS_ACE5LITE_TARGET_SLAVE_N_MM_NOCNI_NITOWER_RPSS_2_ADDRESS)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static silibs_status_t pcie_interrupt_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base);
static silibs_status_t rpss_ras_node_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base);
static silibs_status_t rpss_tower_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base);

/*-- Declarations (Statics and globals) --*/
/* TODO We need to track what has been handled in each sub-handler since it can service multiple indices at once */
vab_generic_handler_t vab_rpss_intu0_handlers[VAB_RPSS_INTU0_NUM_CFGS] = {
    NULL,                         // VAB_RPSS_INTU0_NITOWER_NS
    NULL,                         // VAB_RPSS_INTU0_NITOWER_S
    vab_tcu_handler,              // VAB_RPSS_INTU0_TCU_RAS_CRI
    vab_tbu0_handler,             // VAB_RPSS_INTU0_TBU0_RAS_CRI
    vab_tbu1_handler,             // VAB_RPSS_INTU0_TBU1_RAS_CRI
    vab_fabric_parity_handler,    // VAB_RPSS_INTU0_FABRIC_PARITY_CRI
    vab_tower_handler,            // VAB_RPSS_INTU0_NITOWER_CRI
    vab_ras_node_handler,         // VAB_RPSS_INTU0_RAS_CRI
    vab_csr_parity_handler,       // VAB_RPSS_INTU0_CSR_PARITY_CRI
    vab_pcr_parity_handler,       // VAB_RPSS_INTU0_PCR_PARITY_CRI
    NULL,                         // VAB_RPSS_INTU0_HW_ASSERT_CRI
    NULL,                         // VAB_RPSS_INTU0_TBU_CRIT_ERR
    pcie_interrupt_handler,       // VAB_RPSS_INTU0_RPSS_SCP_INT
    NULL,                         // VAB_RPSS_INTU0_RPSS_MCP_INT
    NULL,                         // VAB_RPSS_INTU0_RPSS_HSP_INT
    NULL,                         // VAB_RPSS_INTU0_RPSS_NITOWER_NS_INT
    NULL,                         // VAB_RPSS_INTU0_RPSS_NITOWER_S_INT
    vab_integ_pcr_parity_handler, // VAB_RPSS_INTU0_RPSS_PCR_PARITY_CRI
    vab_integ_csr_parity_handler, // VAB_RPSS_INTU0_RPSS_CSR_PARITY_CRI
    rpss_ras_node_handler,        // VAB_RPSS_INTU0_RPSS_RAS_CRI
    rpss_tower_handler,           // VAB_RPSS_INTU0_RPSS_NITOWER_CRI
    NULL,                         // VAB_RPSS_INTU0_RPSS_HW_ASSERT_CRI
    NULL                          // VAB_RPSS_INTU0_RPSS_DBG_CRI
};

vab_generic_handler_t vab_rpss_intu1_handlers[VAB_RPSS_INTU1_NUM_CFGS] = {
    vab_tower_handler,     // VAB_RPSS_INTU1_NITOWER_FHI
    vab_tower_handler,     // VAB_RPSS_INTU1_NITOWER_ERI
    vab_tcu_handler,       // VAB_RPSS_INTU1_TCU_GIC_RAS_FHI
    vab_tcu_handler,       // VAB_RPSS_INTU1_TCU_GIC_RAS_ERI
    vab_tbu0_handler,      // VAB_RPSS_INTU1_TBU0_GIC_RAS_FHI
    vab_tbu0_handler,      // VAB_RPSS_INTU1_TBU0_GIC_RAS_ERI
    vab_tbu1_handler,      // VAB_RPSS_INTU1_TBU1_GIC_RAS_FHI
    vab_tbu1_handler,      // VAB_RPSS_INTU1_TBU1_GIC_RAS_ERI
    vab_ras_node_handler,  // VAB_RPSS_INTU1_RAS_FHI
    vab_ras_node_handler,  // VAB_RPSS_INTU1_RAS_ERI
    rpss_tower_handler,    // VAB_RPSS_INTU1_RPSS_NITOWER_FHI
    rpss_tower_handler,    // VAB_RPSS_INTU1_RPSS_NITOWER_ERI
    rpss_ras_node_handler, // VAB_RPSS_INTU1_RPSS_RAS_FHI
    rpss_ras_node_handler  // VAB_RPSS_INTU1_RPSS_RAS_ERI
};

/*------------- Functions ----------------*/
static silibs_status_t rpss_ras_node_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base)
{
    ras_agent_entity_t* agent = &(pciess_get_entity((RPSS_INSTANCE)vab_id)->ras_node);
    ras_arm_agent_set_base(agent, base + RAS_RPSS_OFFSET);
    ras_error_record_t record = {0};
    while (ras_agent_probe(agent, &record))
    {
        ras_print_record(&record);
        if (record.handler)
        {
            if (record.handler(&record))
            {
                VAB_ET_ERROR(VAB_ET_TYPE_RPSS_RAS_NODE_HANDLER_ERROR);
                FPFW_DBGPRINT_ALWAYS("Error encountered while handling RPSS RAS Node record\n");
            }
        }
        else
        {
            VAB_ET_ERROR(VAB_ET_TYPE_RPSS_RAS_NODE_INVALID_RECORD);
            FPFW_DBGPRINT_ALWAYS(
                "RPSS RAS Node Record was marked as invalid! No further handling will be done\n");
            continue;
        }
    }
    return SILIBS_SUCCESS;
}

static silibs_status_t rpss_tower_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base)
{
    return tower_fmu_handler(vab_convert_vab_id_to_rpss_tower_id(vab_id),
                             (vab_id <= D0_VAB3_RPSS3) ? SOC_D0 : SOC_D1,
                             base + RAS_RPSS_TOWER_OFFSET);
}

static silibs_status_t pcie_interrupt_handler(SUBSYSTEM_WITH_VAB_ID vab_id, uintptr_t base)
{
    uint8_t rpss_id = (RPSS_INSTANCE)vab_id;
    pcie_ss_entity_t* cur_ss = pciess_get_entity((RPSS_INSTANCE)rpss_id);
    pcie_ss_set_base(cur_ss, base + VAB_RPSS_OFFSET);
    pciess_int_probe_t info;

    /*
     * Probe all INTUs on an rpss and call the PCIe IRQ handler that lives in
     * the PCIe driver. It will process the probe output and take appropriate
     * action.
     */
    if (pciess_probe(cur_ss, &info, INTU_TO_SCP))
    {
        rpss_irq_callback(cur_ss, &info);

        /*
         * If we are here, it means the PCIe irq was a non-fatal/advisory.
         * Clear RPSS INTUs and continue execution
         */
        pciess_clear_handler(cur_ss, &info, INTU_TO_SCP);
    }
    else
    {
        /* This is certainly unexpected, but should this be considered fatal? */
        VAB_ET_ERROR_PARAM(VAB_ET_TYPE_RPSS_NO_VALID_INTERRUPT_SOURCE, rpss_id);
        FPFW_DBGPRINT_ALWAYS("RPSS[%d]: No valid interrupt source found!\n", rpss_id);
    }

    return SILIBS_SUCCESS;
}

void process_vab_rpss_probe(vab_isr_ctx_t* ctx)
{
    vabss_int_probe_t* probe = &ctx->probe;
    for (uint8_t idx = 0; idx < probe->num_intu0_pins; idx++)
    {
        if (probe->intu0[idx].asserted)
        {
            if (vab_rpss_intu0_handlers[idx])
            {
                vab_rpss_intu0_handlers[idx](ctx->vab_id, ctx->vab_base);
            }
            else
            {
                VAB_ET_WARNING_PARAM(VAB_ET_TYPE_RPSS_INTU0_UNSUPPORTED, idx);
                FPFW_DBGPRINT_ALWAYS("|VAB_RPSS| INTU0 Index %d for VAB %d is not supported! Ignoring...\n", idx, ctx->vab_id);
            }
        }
    }

    for (uint8_t idx = 0; idx < probe->num_intu1_pins; idx++)
    {
        if (probe->intu1[idx].asserted)
        {
            if (vab_rpss_intu1_handlers[idx])
            {
                vab_rpss_intu1_handlers[idx](ctx->vab_id, ctx->vab_base);
            }
            else
            {
                VAB_ET_WARNING_PARAM(VAB_ET_TYPE_RPSS_INTU1_UNSUPPORTED, idx);
                FPFW_DBGPRINT_ALWAYS("|VAB_RPSS| INTU1 Index %d for VAB %d is not supported! Ignoring...\n", idx, ctx->vab_id);
            }
        }
    }
}
