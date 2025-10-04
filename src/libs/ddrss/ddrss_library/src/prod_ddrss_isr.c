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
#include <bug_check.h>
#include <cper.h>
#include <ddrss.h>
#include <ddrss_intu.h>
#include <ddrss_lib.h>
#include <health_monitor.h>
#include <idsw_kng.h>
#include <nvic.h>
#include <silibs_platform.h>
#include <silibs_status.h>
#include <stdint.h>
#include <stdio.h>

volatile uint8_t prod_ddrss_mc_ras_ce_en[DDRSS_MAX_MC_NUM_PER_DIE][DDRSS_RAS_NODE_ID_MC_ERG_MAX];

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Functions ---------------*/
static int ddrss_get_and_probe_ras_agent(uint32_t mc, DDRSS_RAS_NODE_ID ras_agent_entity_id, ras_agent_entity_t** ras_agent)
{
    uint32_t sub_sts;
    acpi_err_sec_memory_t ddr_ras_cper = {0};
    acpi_err_sec_mem_vendor_t ddr_vendor_cper = {0};
    ras_error_record_t record = {0};

    sub_sts = ddrss_get_ras_agent(mc, ras_agent_entity_id, ras_agent);
    if (sub_sts == SILIBS_SUCCESS)
    {
        ras_arm_agent_probe(*ras_agent, &record);
        ras_print_record(&record);
        sub_sts = ddrss_convert_ras_rec_to_cper(mc, &record, &ddr_ras_cper, &ddr_vendor_cper);
        ddrss_print_cper(&ddr_ras_cper, &ddr_vendor_cper);
        if (sub_sts == SILIBS_SUCCESS)
        {
            acpi_error_severity_t severity = (record.err_type & RAS_UNCORRECTABLE_ERROR)
                                                 ? ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL
                                                 : ACPI_ERROR_SEVERITY_CORRECTED;
            acpi_cper_section_t std_cper_section;
            std_cper_section.sec_mem = ddr_ras_cper;
            hm_submit_cper(ACPI_ERROR_DOMAIN_DDR, severity, &std_cper_section, sizeof(std_cper_section));

            acpi_cper_section_t vendor_cper_section;
            vendor_cper_section.sec_ddr_mem_vendor = ddr_vendor_cper;
            hm_submit_cper(ACPI_ERROR_DOMAIN_DDR, severity, &vendor_cper_section, sizeof(vendor_cper_section));
        }

        // For ER0/ER2 and ER4/ER6 of ERG0, need to reset the CEC threshold.
        DDRSS_RAS_NODE_ID erg_id = DDRSS_IS_RAS_ERG(record.reporting_agent, DDRSS_RAS_NODE_ID_MC_ERG0)
                                       ? DDRSS_RAS_NODE_ID_MC_ERG0
                                       : DDRSS_RAS_NODE_ID_MC_ERG1;
        if ((erg_id == DDRSS_RAS_NODE_ID_MC_ERG0) && record.ce_count_valid && (record.ce_count > 0))
        {
            ddrss_ras_update_ce_threshold(mc, 1 << record.position, true);
        }

        if (record.handler)
        {
            if (record.handler(&record))
            {
                printf("Error encountered while handling record!\n");
            }
        }
        else
        {
            printf("Record was marked invalid! No further handling will be done.\n");
        }

        if ((erg_id == DDRSS_RAS_NODE_ID_MC_ERG0) && record.status_valid && (record.status & DDR_ERG0_ERR0STATUS_LO_CE_MASK))
        {
            // It is CE, needs to disable CE RAS interrupt.
            // A polling thread ddr_poll_ecc_ce_errors() will enable it again after a configurable interval.
            sub_sts = prod_ddrss_set_ras_erg_ce_interrupt_enable(mc, erg_id, false);
            if (sub_sts != SILIBS_SUCCESS)
            {
                printf("Error in disabling CE RAS interrupt!\n");
            }
        }
    }
    return sub_sts;
}

/**
 * @brief     Convert rh record to rh cper record
 *
 *
 * @param[in] mc            - Memory controller
 * @param[in] sample_type   - info aout how the rh was sampled
 * @param[in] p_rh_tel        - pointer to rh telemetry record
 * @param[in] p_rh_cper     - pointer to rh cper telemetry record
 *
 * @return void
 */
void prod_ddrss_convert_rh_rec_to_rh_cper(uint32_t mc,
                                          rh_tlm_sample_type sample_type,
                                          ddrss_rhm_tm_evt_t* p_rh_tel,
                                          acpi_err_sec_ddrss_rhm_tm_t* p_rh_cper)
{
    if (p_rh_tel == NULL || p_rh_cper == NULL)
    {
        return;
    }

    p_rh_cper->tlm_sample_type = sample_type;
    p_rh_cper->mc = mc;
    p_rh_cper->valid = p_rh_tel->valid;
    p_rh_cper->timestamp = p_rh_tel->timestamp;
    p_rh_cper->dropped = p_rh_tel->dropped;
    p_rh_cper->type = p_rh_tel->type;
    p_rh_cper->parity_err = p_rh_tel->parity_err;
    p_rh_cper->act_rank = p_rh_tel->act_rank;
    p_rh_cper->act_bg = p_rh_tel->act_bg;
    p_rh_cper->act_bank = p_rh_tel->act_bank;
    p_rh_cper->act_row = p_rh_tel->act_row;
    p_rh_cper->smc = p_rh_tel->smc;
    p_rh_cper->soc = p_rh_tel->soc;
    p_rh_cper->spill_count = p_rh_tel->spill_count;

    for (uint8_t i = 0; i < ACPI_DDRSS_RHM_TM_MAX_WAYS; i++)
    {
        p_rh_cper->way_address[i] = p_rh_tel->way_info[i].address;
        p_rh_cper->way_count[i] = p_rh_tel->way_info[i].count;
        p_rh_cper->way_lock[i] = p_rh_tel->way_info[i].lock;
    }
}

/**
 * @brief     Check if a DDRSS interrupt is pending and enabled for this context.
 *
 * This function reads the DDRSS INTU status register, masks off any interrupts
 * that are not currently enabled for the SCP destination, and returns whether
 * any valid interrupt bits remain set.
 *
 * @param[in] context  Void pointer to a uint32_t holding the local DDRSS index (0–5).
 *                     On Die1, this is adjusted by +6 to map into the 0–11 range.
 *
 * @return true  if one or more enabled interrupts are pending,
 *         false otherwise.
 */
bool prod_ddrss_interrupt_pending(void* context)
{
    uint32_t local_ddrss = *(uint32_t*)(context);
    KNG_DIE_ID die_num = idsw_get_die_id();

    uint32_t ddrss = (die_num == DIE_1) ? local_ddrss + 6 : local_ddrss; // 0-11
    uint32_t mc = ddrss * 2;
    uint32_t ddr_intu_sts = 0;
    uintptr_t ddrss_base = 0;
    int sts = SILIBS_SUCCESS;

    // Read DDRSS INTU status
    sts = ddrss_get_component_base(ddrss, DDRSS_COMP_ID_DDRINTU, &ddrss_base);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    sts = intu_get_interrupt_status(ddrss_base, &ddr_intu_sts);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    // Only check enabled interrupts
    uint32_t intu_enable = 0;
    sts = ddrss_ddr_intu_get_interrupt_dest_enable(mc, DDRSS_INTU_SCP_INT, &intu_enable);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    ddr_intu_sts = ddr_intu_sts & intu_enable;
    if (ddr_intu_sts == 0)
    {
        return false;
    }

    return true;
}

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
    acpi_err_sec_mem_vendor_t ddr_cper = {0};

    uint32_t ddrss = (die_num == DIE_1) ? local_ddrss + 6 : local_ddrss; // 0-11
    uint32_t ddr_intu_err = 0;
    uint32_t mc = ddrss * 2;
    uint32_t ddr_intu_sts = 0;
    uintptr_t ddrss_base = 0;
    ras_agent_entity_t* ras_agent[2] = {0};
    int sts = SILIBS_SUCCESS;
    int sub_sts = SILIBS_SUCCESS;

    // Read DDRSS INTU status
    printf("DDRSS %d ISR Enter\n", (unsigned int)ddrss);

    sts = ddrss_get_component_base(ddrss, DDRSS_COMP_ID_DDRINTU, &ddrss_base);
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

        ddr_cper.module = DDRSS_MC_TO_DDRSS(mc);
        ddr_cper.valid_module = 1;
        ddr_cper.device = DDRSS_IS_SUB_CHANEL0(mc);
        ddr_cper.valid_device = 1;
        ddr_cper.error_type = DDRSS_CPER_ERROR_UNKNOWN;
        ddr_cper.valid_error_type = 1;
        ddr_cper.error_status.error_type = ddr_cper.error_type;
        ddr_cper.error_status.data = 1;
        ddr_cper.valid_error_status = 1;

        acpi_cper_section_t cper_section;
        cper_section.sec_ddr_mem_vendor = ddr_cper;
        hm_submit_cper(ACPI_ERROR_DOMAIN_DDR, ACPI_ERROR_SEVERITY_CORRECTED, &cper_section, sizeof(ddr_cper));
        memset(&ddr_cper, 0, sizeof(ddr_cper));
    }

    uint32_t int_mask;
    int_mask = (1 << DDRSS_INTU_MC0_CRI_INT);
    if (ddr_intu_sts & int_mask)
    {
        printf("MC0 CRI int\n");
        sub_sts = ddrss_get_and_probe_ras_agent(mc, DDRSS_RAS_NODE_ID_MC_ERG1, &ras_agent[1]);
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
        sub_sts = ddrss_get_and_probe_ras_agent(mc + 1, DDRSS_RAS_NODE_ID_MC_ERG1, &ras_agent[1]);
        if (sub_sts != SILIBS_SUCCESS)
        {
            ddr_intu_err |= int_mask;
        }
        ddr_intu_clr_sts |= int_mask;
    }

    uint32_t grp_sts;
    uint32_t sra_ras_int_msk = (1 << DDRSS_INTU_SRA_ERI) | (1 << DDRSS_INTU_SRA_FHI);
    if (ddr_intu_sts & sra_ras_int_msk)
    {
        printf("DDR ERI/FHI int\n");
        sts = ddrss_get_component_base(ddrss, DDRSS_COMP_ID_MC0, &ddrss_base);
        FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);
        grp_sts = MMIO_READ32(PROD_DDRSS_MC0_RASERG_REG_ADDR(ddrss_base, errgsr_lo));
        if (grp_sts)
        {
            sub_sts = ddrss_get_and_probe_ras_agent(mc, DDRSS_RAS_NODE_ID_MC_ERG0, &ras_agent[0]);
            if (sub_sts != SILIBS_SUCCESS)
            {
                printf("Failed to get RAS agent for MC%d ERG0\n", (unsigned int)mc);
                ddr_intu_err |= sra_ras_int_msk;
            }
        }
        grp_sts = MMIO_READ32(PROD_DDRSS_MC1_RASERG_REG_ADDR(ddrss_base, errgsr_lo));
        if (grp_sts)
        {
            sub_sts = ddrss_get_and_probe_ras_agent(mc + 1, DDRSS_RAS_NODE_ID_MC_ERG0, &ras_agent[0]);
            if (sub_sts != SILIBS_SUCCESS)
            {
                printf("Failed to get RAS agent for MC%d ERG0\n", (unsigned int)mc);
                ddr_intu_err |= sra_ras_int_msk;
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

        printf("CPER:DDR PLL");
        prod_ddrss_get_intr_event_cper(mc, DDRSS_INTU_PLL_INTERRUPT_OUT, &ddr_cper);

        acpi_cper_section_t cper_section;
        cper_section.sec_ddr_mem_vendor = ddr_cper;
        hm_submit_cper(ACPI_ERROR_DOMAIN_DDR, ACPI_ERROR_SEVERITY_CORRECTED, &cper_section, sizeof(ddr_cper));
        memset(&ddr_cper, 0, sizeof(ddr_cper));
    }

    int_mask = (1 << DDRSS_INTU_PCR_PAR_ERR);
    if (ddr_intu_sts & int_mask)
    {
        printf("DDR PCR PAR int\n");
        ddr_intu_clr_sts |= int_mask;

        printf("CPER:DDR PCR PAR");
        prod_ddrss_get_intr_event_cper(mc, DDRSS_INTU_PCR_PAR_ERR, &ddr_cper);

        acpi_cper_section_t cper_section;
        cper_section.sec_ddr_mem_vendor = ddr_cper;
        hm_submit_cper(ACPI_ERROR_DOMAIN_DDR, ACPI_ERROR_SEVERITY_CORRECTED, &cper_section, sizeof(ddr_cper));
        memset(&ddr_cper, 0, sizeof(ddr_cper));
    }

    int_mask = (1 << DDRSS_INTU_INTU_PAR_ERR);
    if (ddr_intu_sts & int_mask)
    {
        printf("DDR INTU PAR int\n");
        ddr_intu_clr_sts |= int_mask;

        printf("CPER:DDR INTU PAR");
        prod_ddrss_get_intr_event_cper(mc, DDRSS_INTU_INTU_PAR_ERR, &ddr_cper);

        acpi_cper_section_t cper_section;
        cper_section.sec_ddr_mem_vendor = ddr_cper;
        hm_submit_cper(ACPI_ERROR_DOMAIN_DDR, ACPI_ERROR_SEVERITY_CORRECTED, &cper_section, sizeof(ddr_cper));
        memset(&ddr_cper, 0, sizeof(ddr_cper));
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
        printf("DDR int handler failed for INTU mask 0x%08x\n", (unsigned int)ddr_intu_err);
    }

    // If DDR RAS CRI is active, trigger crash dump since it is fatal.
    bool is_cri = (ddr_intu_clr_sts & ((1 << DDRSS_INTU_MC0_CRI_INT) | (1 << DDRSS_INTU_MC1_CRI_INT))) ? true : false;
    if (is_cri)
    {
        uint32_t mc_cri = mc + ((ddr_intu_clr_sts & (1 << DDRSS_INTU_MC0_CRI_INT)) ? 0 : 1);
        printf("Force CD due to fatal DDR CRI from MC%d\n", (unsigned int)mc_cri);
        BUG_CHECK(KNG_E_FAIL, mc_cri, ddr_intu_sts);
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
    acpi_err_sec_mem_vendor_t ddr_cper = {0};

    // Read PHY interrupt status
    sts = ddrss_get_phy_interrupt_status(mc, &phy_int_sts);
    if (sts != SILIBS_SUCCESS)
    {
        printf("Failed to get INTU enable status.  Retval = %d\n", sts);
    }

    if (phy_int_sts & csr_PhyTrngFailEn_MASK)
    {
        phy_int_clr |= csr_PhyTrngFailEn_MASK;
    }

    if (phy_int_sts & csr_PhyTrngCmpltEn_MASK)
    {
        phy_int_clr |= csr_PhyTrngCmpltEn_MASK;
    }

    if (phy_int_sts & csr_PhyInitCmpltEn_MASK)
    {
        phy_int_clr |= csr_PhyInitCmpltEn_MASK;
    }

    if (phy_int_sts & csr_PhyAcsmParityErrEn_MASK)
    { // ACSM is an internal memory within the PHY
        phy_int_clr |= csr_PhyAcsmParityErrEn_MASK;
        ddr_cper.vendor_err_info.phy_fatal.phyacsmparityerr = 1;
        ddr_cper.vendor_err_info.phy_fatal.valid_phyacsmparityerr = 1;
        ddr_cper.valid_vendor_err_info = 1;
    }

    if (phy_int_sts & csr_PhyPIEParityErrEn_MASK)
    { // PIE is an internal memory within the PHY
        phy_int_clr |= csr_PhyPIEParityErrEn_MASK;
        ddr_cper.vendor_err_info.phy_fatal.phypieparityerr = 1;
        ddr_cper.vendor_err_info.phy_fatal.valid_phypieparityerr = 1;
        ddr_cper.valid_vendor_err_info = 1;
    }

    if (phy_int_sts & csr_PhyRdfPtrChkErrEn_MASK)
    {
        phy_int_clr |= csr_PhyRdfPtrChkErrEn_MASK;
        ddr_cper.vendor_err_info.phy_fatal.phyrdfptrchkerr = 1;
        ddr_cper.vendor_err_info.phy_fatal.valid_phyrdfptrchkerr = 1;
        ddr_cper.valid_vendor_err_info = 1;
    }

    if (phy_int_sts & csr_PhyEccEn_MASK)
    {
        phy_int_clr |= csr_PhyEccEn_MASK;
        ddr_cper.vendor_err_info.phy_fatal.phyeccen = 1;
        ddr_cper.vendor_err_info.phy_fatal.valid_phyeccen = 1;
        ddr_cper.valid_vendor_err_info = 1;
    }

    if (phy_int_sts & csr_PhyPIEProgErrEn_MASK)
    {
        uint32_t arc_ecc = MMIO_READ32(tDRTUB | csr_ArcEccIndications_ADDR);
        phy_int_clr |= csr_PhyPIEProgErrEn_MASK;
        (void)arc_ecc;
        ddr_cper.vendor_err_info.phy_fatal.phypieprogerr = 1;
        ddr_cper.vendor_err_info.phy_fatal.valid_phypieprogerr = 1;
        ddr_cper.valid_vendor_err_info = 1;
    }

    if (phy_int_sts & csr_PhyTxPPTEn_MASK)
    {
        phy_int_clr |= csr_PhyTxPPTEn_MASK;
        ddr_cper.vendor_err_info.phy_fatal.phytxppt = 1;
        ddr_cper.vendor_err_info.phy_fatal.valid_phytxppt = 1;
        ddr_cper.valid_vendor_err_info = 1;
    }

    if (phy_int_sts & csr_PhyAlertEn_MASK)
    {
        phy_int_clr |= csr_PhyAlertEn_MASK;
        ddr_cper.vendor_err_info.phy_fatal.phyalert = 1;
        ddr_cper.vendor_err_info.phy_fatal.valid_phyalert = 1;
        ddr_cper.valid_vendor_err_info = 1;
    }

    if (phy_int_sts & (csr_PhyAcsmParityErrEn_MASK | csr_PhyPIEParityErrEn_MASK | csr_PhyRdfPtrChkErrEn_MASK |
                       csr_PhyEccEn_MASK | csr_PhyPIEProgErrEn_MASK | csr_PhyTxPPTEn_MASK | csr_PhyAlertEn_MASK))
    {
        ddr_cper.module = DDRSS_MC_TO_DDRSS(mc);
        ddr_cper.valid_module = 1;
        ddr_cper.device = DDRSS_IS_SUB_CHANEL0(mc);
        ddr_cper.valid_device = 1;
        ddr_cper.error_type = DDRSS_CPER_PHY_FATAL;
        ddr_cper.valid_error_type = 1;
        ddr_cper.error_status.error_type = ddr_cper.error_type;
        ddr_cper.error_status.data = 1;
        ddr_cper.valid_error_status = 1;
        acpi_cper_section_t cper_section;
        cper_section.sec_ddr_mem_vendor = ddr_cper;
        hm_submit_cper(ACPI_ERROR_DOMAIN_DDR, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL, &cper_section, sizeof(ddr_cper));
        memset(&ddr_cper, 0, sizeof(ddr_cper));
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
    acpi_err_sec_mem_vendor_t ddr_cper = {0};

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
            printf("sub_sts res %d\n", sub_sts);
            mc_intu_err |= (1 << DDRSS_INTU_MC_RMTELEMETRYAVAIL);
        }
        else
        {
            acpi_err_sec_ddrss_rhm_tm_t rh_cper_section = {0};
            acpi_cper_section_t cper_section;
            cper_section.sec_rh_tlm = rh_cper_section;
            prod_ddrss_convert_rh_rec_to_rh_cper(mc, RHTLM_SAMPLE_EVENT, &ddrss_rm_telemetry, &rh_cper_section);
            hm_submit_cper(ACPI_ERROR_DOMAIN_RHTLM, ACPI_ERROR_SEVERITY_INFORMATIONAL, &cper_section, sizeof(cper_section));
        }
        mc_intu_clr_sts |= (1 << DDRSS_INTU_MC_RMTELEMETRYAVAIL);
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

        printf("CPER:MC media ESC trans changed");
        prod_ddrss_get_intr_event_cper(mc, DDRSS_INTU_MC_MEDIAECSTRANSPCHANGED, &ddr_cper);

        acpi_cper_section_t cper_section;
        cper_section.sec_ddr_mem_vendor = ddr_cper;
        hm_submit_cper(ACPI_ERROR_DOMAIN_DDR, ACPI_ERROR_SEVERITY_CORRECTED, &cper_section, sizeof(acpi_err_sec_mem_vendor_t));
        memset(&ddr_cper, 0, sizeof(ddr_cper));
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

        printf("CPER:MC media ref temp changed");
        prod_ddrss_get_intr_event_cper(mc, DDRSS_INTU_MC_MEDIAREFTEMPCHANGED, &ddr_cper);
        acpi_cper_section_t cper_section;
        cper_section.sec_ddr_mem_vendor = ddr_cper;
        hm_submit_cper(ACPI_ERROR_DOMAIN_DDR, ACPI_ERROR_SEVERITY_CORRECTED, &cper_section, sizeof(acpi_err_sec_mem_vendor_t));
        memset(&ddr_cper, 0, sizeof(ddr_cper));
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

        printf("CPER:MC media ref temp changed");
        prod_ddrss_get_intr_event_cper(mc, DDRSS_INTU_MC_MEDIAREFTEMPHIGH, &ddr_cper);
        acpi_cper_section_t cper_section;
        cper_section.sec_ddr_mem_vendor = ddr_cper;
        hm_submit_cper(ACPI_ERROR_DOMAIN_DDR, ACPI_ERROR_SEVERITY_CORRECTED, &cper_section, sizeof(acpi_err_sec_mem_vendor_t));
        memset(&ddr_cper, 0, sizeof(ddr_cper));
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
        printf("MC int handler failed for INTU mask 0x%08x\n", (unsigned int)mc_intu_err);
    }

    return sts;
}

int prod_ddrss_get_intr_event_cper(uint32_t mc, uint32_t intr_event, acpi_err_sec_mem_vendor_t* ddr_cper)
{
    switch (intr_event)
    {
    case DDRSS_INTU_MC_MEDIAECSTRANSPCHANGED: {
        ddr_cper->vendor_err_info.misc.mc_evt_ecs_trandchanged = 1;
        ddr_cper->vendor_err_info.misc.valid_mc_evt_ecs_trandchanged = 1;
        ddr_cper->error_type = DDRSS_CPER_EVT_ECS_TRANS_CHANGED;
        break;
    }
    case DDRSS_INTU_MC_MEDIAREFTEMPCHANGED: {
        ddr_cper->vendor_err_info.misc.mc_evtref_tempchanged = 1;
        ddr_cper->vendor_err_info.misc.valid_mc_evtref_tempchanged = 1;
        ddr_cper->error_type = DDRSS_CPER_EVT_REF_TEMP_CHANGED;
        break;
    }
    case DDRSS_INTU_MC_MEDIAREFTEMPHIGH: {
        ddr_cper->vendor_err_info.misc.mc_evtref_temphigh = 1;
        ddr_cper->vendor_err_info.misc.valid_mc_evtref_temphigh = 1;
        ddr_cper->error_type = DDRSS_CPER_EVT_REF_TEMP_HIGH;
        break;
    }
    case DDRSS_INTU_PLL_INTERRUPT_OUT: {
        ddr_cper->vendor_err_info.misc.ddr_intu_pll_out = 1;
        ddr_cper->vendor_err_info.misc.valid_ddr_intu_pll_out = 1;
        ddr_cper->error_type = DDRSS_CPER_INTU_PLL_INTERRUPT;
        break;
    }
    case DDRSS_INTU_PCR_PAR_ERR: {
        ddr_cper->vendor_err_info.misc.ddr_pcr_par_err = 1;
        ddr_cper->vendor_err_info.misc.valid_ddr_pcr_par_err = 1;
        ddr_cper->error_type = DDRSS_CPER_INTU_PCR_PAR_ERR;
        break;
    }
    case DDRSS_INTU_INTU_PAR_ERR: {
        ddr_cper->vendor_err_info.misc.ddr_intu_par_err = 1;
        ddr_cper->vendor_err_info.misc.valid_ddr_intu_par_err = 1;
        ddr_cper->error_type = DDRSS_CPER_INTU_PAR_ERR;
        break;
    }
    default:
        return SILIBS_E_DEVICE;
    }

    ddr_cper->module = DDRSS_MC_TO_DDRSS(mc);
    ddr_cper->valid_module = 1;
    ddr_cper->device = DDRSS_IS_SUB_CHANEL0(mc);
    ddr_cper->valid_device = 1;
    ddr_cper->valid_error_type = 1;
    ddr_cper->error_status.error_type = ddr_cper->error_type;
    ddr_cper->error_status.control = 1;
    ddr_cper->valid_error_status = 1;
    ddr_cper->valid_vendor_err_info = 1;

    return SILIBS_SUCCESS;
}

int prod_ddrss_get_ras_erg_ce_interrupt_enable(uint32_t mc, DDRSS_RAS_NODE_ID erg_id, bool* enable)
{
    if ((erg_id >= DDRSS_RAS_NODE_ID_MC_ERG_MAX) || (mc >= DDRSS_MAX_MC_NUM) || (!enable))
    {
        return SILIBS_E_PARAM;
    }
    uint32_t local_mc = DDRSS_GET_LOCAL_MC(mc);
    *enable = prod_ddrss_mc_ras_ce_en[local_mc][erg_id];
    return SILIBS_SUCCESS;
}

int prod_ddrss_set_ras_erg_ce_interrupt_enable(uint32_t mc, DDRSS_RAS_NODE_ID erg_id, bool enable)
{
    int sts;

    ras_agent_entity_t* ddrss_ras_agent = NULL;
    if ((erg_id >= DDRSS_RAS_NODE_ID_MC_ERG_MAX) || (mc >= DDRSS_MAX_MC_NUM))
    {
        return SILIBS_E_PARAM;
    }

    uint32_t local_mc = DDRSS_GET_LOCAL_MC(mc);
    sts = ddrss_get_ras_agent(mc, erg_id, &ddrss_ras_agent);
    if (sts == SILIBS_SUCCESS)
    {
        printf("MC%d: %s DDR CE INT\n", (unsigned int)mc, enable ? "Enable" : "Disable");
        if (enable)
        {
            prod_ddrss_mc_ras_ce_en[local_mc][erg_id] = 1;
            sts = ras_arm_agent_enable_signaling_by_type(ddrss_ras_agent, RAS_ARM_CNTRL_CI | RAS_ARM_CNTRL_CED | RAS_ARM_ON_READ_WRITE);
        }
        else
        {
            prod_ddrss_mc_ras_ce_en[local_mc][erg_id] = 0;
            sts = ras_arm_agent_disable_signaling_by_type(ddrss_ras_agent, RAS_ARM_CNTRL_CI | RAS_ARM_CNTRL_CED | RAS_ARM_ON_READ_WRITE);
        }
    }
    return sts;
}
