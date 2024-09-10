//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddrss_prod_isr.c
 * This file contains the DDR subsystem interrupt handlers
 */

/*------------- Includes -----------------*/

#include "ddr_erg0_regs.h"
#include "intu_lib.h"
#include "ras_agent.h"
#include "ras_arm_agent.h"
#include "ras_common.h"

#include <FpFwAssert.h>
#include <arm_intrinsic.h>
#include <ddrss.h>
#include <ddrss_intu.h>
#include <ddrss_lib.h>
#include <idsw_kng.h>
#include <nvic.h>
#include <silibs_platform.h>
#include <silibs_status.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Functions ---------------*/

/**
 * @brief TOP Level DDRSS interrupt handler
 *         The DDRSS library is written to assume that memory controllers are uniquely
 *         numbered across the two dies.
 *
 *        Each pass through this ISR will handle one subtype of possible DDRSS error sources.
 *
 * @todo printf statements don't belong in an ISR but are here for testing purposes
 *
 * @param context - Void pointer to a static array of ints 0-5 representing the DDR controller number the ISR is for
 *
 */
void prod_ddrss_interrupt_handler(void* context)
{
    // Parameter passed to isr will be a DDRSS number [0-5] on the local die.
    // DDR base addresses are different between the dies and are numbered in
    // silicon libs as 0-11 as a result.
    uint32_t local_ddrss = *(uint32_t*)(context);
    KNG_DIE_ID die_num = idsw_get_die_id();

    uint32_t ddrss = (die_num == DIE_1) ? local_ddrss + 6 : local_ddrss; // 0-11
    uint32_t ddr_intu_err = 0;
    uint32_t mc = ddrss * 2;
    uint32_t ddr_intu_sts = 0;
    uintptr_t ddrss_base = 0;
    ras_agent_entity_t* ras_agent[2];
    ras_error_record_t record;
    int sts = SILIBS_SUCCESS;
    int sub_sts = SILIBS_SUCCESS;

    // Read DDRSS INTU status
    printf("DDRSS %d ISR Enter\n", (unsigned int)ddrss);

    sts = ddrss_get_component_base(ddrss, DDRSS_COMP_ID_MC0_INTU, &ddrss_base);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    sts = intu_get_interrupt_status(ddrss_base, &ddr_intu_sts);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);
    printf("SS%d INTU STS: 0x%08x\n", (unsigned int)ddrss, (unsigned int)ddr_intu_sts);

    // Only check enabled interrupts
    uint32_t intu_enable = 0;
    sts = ddrss_ddr_intu_get_interrupt_dest_enable(mc, DDRSS_INTU_SCP_INT, &intu_enable);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    ddr_intu_sts = ddr_intu_sts & intu_enable;
    printf("SS%d INTU STS after filtering on enabled: 0x%08x\n", (unsigned int)ddrss, (unsigned int)ddr_intu_sts);

    // DDRSS MC should not route interrupt to MCP or HSP directly
    // Ni-Tower interrupts should be handled by HSP since SCP has no access to it.
    const uint32_t ddrss_unexpected_int = (1 << DDRSS_INTU_MC0_HSP_INT) | (1 << DDRSS_INTU_MC1_HSP_INT) |
                                          (1 << DDRSS_INTU_MC0_MCP_INT) | (1 << DDRSS_INTU_MC1_MCP_INT) |
                                          (1 << DDRSS_INTU_NITOWER_ERI) | (1 << DDRSS_INTU_NITOWER_FHI) |
                                          (1 << DDRSS_INTU_NITOWER_CRI) | (1 << DDRSS_INTU_NITOWER_INTERRUPT) |
                                          (1 << DDRSS_INTU_NITOWER_NS_INTERRUPT);

    uint32_t ddr_intu_clr_sts = 0;
    if (ddr_intu_sts & ddrss_unexpected_int)
    {
        // ERROR condition
        printf("DDRSS received unexpected interrupts!\n");
        ddr_intu_clr_sts |= (ddr_intu_sts & ddrss_unexpected_int);
        // We should really never get here because the intu_enable bits for these should not be set.
        // If, however, we do get here, we need to clear the interrupt and could log to a CPER.
        // TODO: Handle the record by sending to DDR_Manager queue ADO Task#1485473 to be logged to CPER.
    }

    uint32_t int_mask;
    int_mask = (1 << DDRSS_INTU_MC0_CRI_INT);
    if (ddr_intu_sts & int_mask)
    {
        printf("MC0 CRI int\n");
        sub_sts = ddrss_probe_ras_agent(mc, DDRSS_RAS_NODE_ID_MC_ERG1);
        if (sub_sts != SILIBS_SUCCESS)
        {
            ddr_intu_err |= int_mask;
        }
        ddr_intu_clr_sts |= int_mask;
    }

    int_mask = (1 << DDRSS_INTU_MC1_CRI_INT);
    if (ddr_intu_sts & int_mask)
    {
        printf("MC1 CRI int\n");
        sub_sts = ddrss_get_ras_agent(mc + 1, DDRSS_RAS_NODE_ID_MC_ERG1, &ras_agent[1]);
        if (sub_sts != SILIBS_SUCCESS)
        {
            ddr_intu_err |= int_mask;
        }
        else
        {
            ras_arm_agent_probe(ras_agent[1], &record);
            // TODO: Handle the record by sending to DDR_Manager queue ADO Task#1485473
        }
        ddr_intu_clr_sts |= int_mask;
    }

    uint32_t grp_sts;
    uint32_t sra_ras_int_msk = (1 << DDRSS_INTU_SRA_ERI) | (1 << DDRSS_INTU_SRA_FHI);
    if (ddr_intu_sts & sra_ras_int_msk)
    {
        printf("DDR ERI/FHI int\n");
        grp_sts = MMIO_READ32(PROD_DDRSS_MC0_RASERG_REG_ADDR(ddrss_base, errgsr_lo));
        if (grp_sts)
        {
            sub_sts = ddrss_get_ras_agent(mc, DDRSS_RAS_NODE_ID_MC_ERG0, &ras_agent[0]);
            if (sub_sts != SILIBS_SUCCESS)
            {
                printf("Failed to get RAS agent for MC%d ERG0\n", (unsigned int)mc);
                ddr_intu_err |= sra_ras_int_msk;
            }
            else
            {
                ras_arm_agent_probe(ras_agent[0], &record);
                // TODO: Handle the record by sending to DDR_Manager queue ADO Task#1485473
            }
        }
        grp_sts = MMIO_READ32(PROD_DDRSS_MC1_RASERG_REG_ADDR(ddrss_base, errgsr_lo));
        if (grp_sts)
        {
            // Each mc has two ERG0
            sub_sts = ddrss_get_ras_agent(mc + 1, DDRSS_RAS_NODE_ID_MC_ERG0, &ras_agent[0]);
            if (sub_sts != SILIBS_SUCCESS)
            {
                ddr_intu_err |= sra_ras_int_msk;
            }
            else
            {
                ras_arm_agent_probe(ras_agent[0], &record);
                // TODO: Handle the record by sending to DDR_Manager queue. RAS / ADO Task#1485473
            }
        }
        ddr_intu_clr_sts |= (ddr_intu_sts & sra_ras_int_msk);
    }

    int_mask = (1 << DDRSS_INTU_PHY_IRQ);
    if (ddr_intu_sts & int_mask)
    {
        printf("DDR PHY int\n");
        sub_sts = prod_ddrss_phy_interrupt_handler(mc);
        if (sub_sts != SILIBS_SUCCESS)
        {
            ddr_intu_err |= int_mask;
        }
        ddr_intu_clr_sts |= int_mask;
    }

    int_mask = (1 << DDRSS_INTU_PLL_INTERRUPT_OUT);
    if (ddr_intu_sts & int_mask)
    {
        printf("DDR PLL int\n");
        ddr_intu_clr_sts |= int_mask;
    }

    int_mask = (1 << DDRSS_INTU_PCR_PAR_ERR);
    if (ddr_intu_sts & int_mask)
    {
        printf("DDR PCR PAR int\n");
        ddr_intu_clr_sts |= int_mask;
    }

    int_mask = (1 << DDRSS_INTU_INTU_PAR_ERR);
    if (ddr_intu_sts & int_mask)
    {
        printf("DDR INTU PAR int\n");
        ddr_intu_clr_sts |= int_mask;
    }

    int_mask = (1 << DDRSS_INTU_MC0_SCP_INT);
    if (ddr_intu_sts & int_mask)
    {
        printf("MC0 SCP int\n");
        sub_sts = prod_ddrss_mc_interrupt_handler(mc);
        if (sub_sts != SILIBS_SUCCESS)
        {
            ddr_intu_err |= int_mask;
        }
        ddr_intu_clr_sts |= int_mask;
    }

    int_mask = (1 << DDRSS_INTU_MC1_SCP_INT);
    if (ddr_intu_sts & int_mask)
    {
        printf("MC1 SCP int\n");
        sub_sts = prod_ddrss_mc_interrupt_handler(mc + 1);
        if (sub_sts != SILIBS_SUCCESS)
        {
            ddr_intu_err |= int_mask;
        }
        ddr_intu_clr_sts |= int_mask;
    }

    if (ddr_intu_clr_sts)
    {
        ddrss_ddr_intu_clear_interrupt(mc, ddr_intu_clr_sts);
        ddrss_ddr_intu_clear_destination_interrupt(mc, DDRSS_INTU_SCP_INT);
    }

    if (sub_sts != SILIBS_SUCCESS)
    {
        sts = SILIBS_E_DEVICE;
        printf("DDR interrupt handler failed for INTU int mask 0x%08x\n", (unsigned int)ddr_intu_err);
    }

    printf("DDRSS %d ISR Exit with sts=%d\n\n", (int)ddrss, sts);

    __DSB();
}

/**
 * @brief DDRSS PHY interrupt handler - all errors must be logged to CPER
 *       This will be accomplished by DDR Manager via a message containing the status bits
 *
 * @param mc - Memory controller number (0-23)
 * @return int - SILIBS_SUCCESS on success, error code otherwise
 */
int prod_ddrss_phy_interrupt_handler(uint32_t mc)
{
    int sts = SILIBS_SUCCESS;
    uint32_t phy_int_sts = 0;
    uint32_t phy_int_clr = 0;

    // Read PHY interrupt status
    sts = ddrss_get_phy_interrupt_status(mc, &phy_int_sts);
    if (sts != SILIBS_SUCCESS)
    {
        printf("Failed to get INTU enable status.  Retval = %d\n", sts);
    }

    // TODO: Send DDR Manager a message to log *ALL* errors to CPER and/or SEL
    // ADO Task#1485473

    // TODO: Handle PHY fatal errors

    // TODO: Handle PHY non-fatal errors
    if (phy_int_sts & csr_PhyAcsmParityErrEn_MASK)
    { // ACSM is an internal memory within the PHY
        phy_int_clr |= csr_PhyAcsmParityErrEn_MASK;
    }

    if (phy_int_sts & csr_PhyPIEParityErrEn_MASK)
    { // PIE is an internal memory within the PHY
        phy_int_clr |= csr_PhyPIEParityErrEn_MASK;
    }

    if (phy_int_sts & csr_PhyRdfPtrChkErrEn_MASK)
    {
        phy_int_clr |= csr_PhyRdfPtrChkErrEn_MASK;
    }

    if (phy_int_sts & csr_PhyEccEn_MASK)
    {
        phy_int_clr |= csr_PhyEccEn_MASK;
    }

    if (phy_int_sts & csr_PhyPIEProgErrEn_MASK)
    {
        uint32_t arc_ecc = MMIO_READ32(tDRTUB | csr_ArcEccIndications_ADDR);
        phy_int_clr |= csr_PhyPIEProgErrEn_MASK;
        // Need to fill this info into CPER once the CPER structure is defined. ADO Task#1485473
        (void)arc_ecc;
    }

    if (phy_int_sts & csr_PhyTxPPTEn_MASK)
    {
        phy_int_clr |= csr_PhyTxPPTEn_MASK;
    }

    if (phy_int_sts & csr_PhyAlertEn_MASK)
    {
        phy_int_clr |= csr_PhyAlertEn_MASK;
    }

    // Clear interrupt
    sts = ddrss_clear_phy_interrupt_status(mc, phy_int_clr);
    if (sts != SILIBS_SUCCESS)
    {
        printf("Failed to clear PHY interrupt status.  Retval = %d\n", sts);
    }
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    return sts;
}

int prod_ddrss_mc_interrupt_handler(uint32_t mc)
{
    int sts = SILIBS_SUCCESS;
    int sub_sts = SILIBS_SUCCESS;
    uint32_t intu_enable = 0;
    uint32_t mc_intu_clr_sts = 0;
    uint32_t mc_intu_err = 0;
    uint32_t mc_intu_sts = 0;

    ddrss_mc_intu_get_interrupt_status(mc, &mc_intu_sts);

    // Only check enabled interrupts
    sub_sts = ddrss_mc_intu_get_interrupt_dest_enable(mc, DDRSS_INTU_SCP_INT, &intu_enable);
    if (sub_sts != SILIBS_SUCCESS)
    {
        printf("Failed to get INTU enable status.  Retval = %d\n", sub_sts);
        return sub_sts;
    }

    mc_intu_sts = mc_intu_sts & intu_enable;
    if (!mc_intu_sts)
    {
        return sts;
    }

    if (mc_intu_sts & (1 << DDRSS_INTU_MC_FEDFLUSHDONE))
    {
        // Warm reset is removed, this should not fire.
        printf("MC FEDB flush done int\n");
        mc_intu_clr_sts |= (1 << DDRSS_INTU_MC_FEDFLUSHDONE);
    }

    if (mc_intu_sts & (1 << DDRSS_INTU_MC_RMTELEMETRYAVAIL))
    {
        ddrss_rhm_tm_evt_t ddrss_rm_telemetry;
        printf("MC RM telemetry avail int\n");
        sub_sts = ddrss_get_telemetry_record(mc, DDRSS_TELEMETRY_TYPE_RHM_EVT, &ddrss_rm_telemetry, sizeof(ddrss_rm_telemetry));
        if (sub_sts != SILIBS_SUCCESS)
        {
            mc_intu_err |= (1 << DDRSS_INTU_MC_RMTELEMETRYAVAIL);
        }
        // TODO: Handle the telemetry by sending to DDR_Manager queue: RAS - RowHammer
        // ADO Task#1485473, Task#1486728, Feature#1705011
    }

    if (mc_intu_sts & (1 << DDRSS_INTU_MC_MEDIAECSTRANSPCHANGED))
    {
        printf("MC media ESC trans changed int\n");
        sub_sts = ddrss_mc_event_clear_interrupt(mc, DDRSS_MC_INTR_EVT_ECS_TRANS_CHANGED);
        if (sub_sts != SILIBS_SUCCESS)
        {
            mc_intu_err |= (1 << DDRSS_INTU_MC_MEDIAECSTRANSPCHANGED);
        }
        mc_intu_clr_sts |= (1 << DDRSS_INTU_MC_MEDIAECSTRANSPCHANGED);
    }

    if (mc_intu_sts & (1 << DDRSS_INTU_MC_MEDIAREFTEMPCHANGED))
    {
        printf("MC media ref temp changed int\n");
        sub_sts = ddrss_mc_event_clear_interrupt(mc, DDRSS_MC_INTR_EVT_REF_TEMP_CHANGED);
        if (sub_sts != SILIBS_SUCCESS)
        {
            mc_intu_err |= (1 << DDRSS_MC_INTR_EVT_REF_TEMP_CHANGED);
        }
        mc_intu_clr_sts |= (1 << DDRSS_INTU_MC_MEDIAREFTEMPCHANGED);
        // TODO: BWL: Handle the temperature change by sending to DDR_Manager queue: RAS - Temperature
        // ADO Feature#1140772, Task#1494090
    }

    if (mc_intu_sts & (1 << DDRSS_INTU_MC_MEDIAREFTEMPHIGH))
    {
        printf("MC media ref temp high int\n");
        sub_sts = ddrss_mc_event_clear_interrupt(mc, DDRSS_MC_INTR_EVT_REF_TEMP_HIGH);
        if (sub_sts != SILIBS_SUCCESS)
        {
            mc_intu_err |= (1 << DDRSS_MC_INTR_EVT_REF_TEMP_CHANGED);
        }
        mc_intu_clr_sts |= (1 << DDRSS_INTU_MC_MEDIAREFTEMPHIGH);
        // TODO: BWL: Handle the temperature change by sending to DDR_Manager queue: RAS - Temperature
        // ADO Feature#1140772, Task#1494090
    }

    if (mc_intu_sts & (1 << DDRSS_INTU_MC_PHYINLP3))
    {
        // PHY in LP3 for media data preserving. Not supported in KNG.
        printf("MC PHY in LP3 int\n");
        sub_sts = ddrss_mc_event_clear_interrupt(mc, DDRSS_MC_INTR_EVT_PHY_IN_LP3);
        if (sub_sts != SILIBS_SUCCESS)
        {
            mc_intu_err |= (1 << DDRSS_MC_INTR_EVT_REF_TEMP_CHANGED);
        }
        mc_intu_clr_sts |= (1 << DDRSS_INTU_MC_PHYINLP3);
    }

    if (mc_intu_sts & (1 << DDRSS_INTU_MC_MEDIASCRUBINITDONE))
    {
        printf("MC media scrub init done int\n");
        sub_sts = ddrss_mc_event_clear_interrupt(mc, DDRSS_MC_INTR_EVT_SCRUB_INIT_DONE);
        if (sub_sts != SILIBS_SUCCESS)
        {
            mc_intu_err |= (1 << DDRSS_MC_INTR_EVT_REF_TEMP_CHANGED);
        }
        mc_intu_clr_sts |= (1 << DDRSS_INTU_MC_MEDIASCRUBINITDONE);
    }

    if (mc_intu_clr_sts)
    {
        ddrss_mc_intu_clear_interrupt(mc, mc_intu_clr_sts);
        ddrss_mc_intu_clear_destination_interrupt(mc, DDRSS_INTU_SCP_INT);
    }

    if (sub_sts)
    {
        sts = SILIBS_E_DEVICE;
        printf("MC interrupt handler failed for INTU int mask 0x%08x\n", (unsigned int)mc_intu_err);
    }

    return sts;
}