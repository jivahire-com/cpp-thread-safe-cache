//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Instantiates PCIe root port devices using SCP driver framework.
 */

/*------------- Includes -----------------*/
#define __NO_CSR_TYPEDEFS__     // Needed to avoid huge buffers in ap_top_regs.h
#define __NO_ADDRMAP_TYPEDEFS__ // Needed to avoid huge buffers in ap_top_regs.h
#include <DfwkDriver.h>         // for PDFWK_SYNC_REQUEST_HEADER
#include <FpFwAssert.h>         // for FPFW_RUNTIME_ASSERT
#include <ap_top_regs.h>        // for AP_TOP_D0_VAB_RPSS0_ADDRESS
#include <atu_lib.h>            // for atu_map, ATU_ID_MSCP, atu_map...
#include <idsw.h>               // for idsw_get_platform_sdv, _PLAT_ID
#include <idsw_kng.h>
#include <kng_atu_mappings.h>          // for ATU_MAPPING_RPSS0_CFG, ATU_MA...
#include <kng_soc_constants.h>         // for NUM_RPSS, RPSS_INSTANCE
#include <pcie_dfwk.h>                 // for pcie_sync_request_t
#include <pcie_knobs.h>                // for PCIE_BIFURCATION_1X16, PCIE_C...
#include <pcie_platform_overrides_i.h> // for plat_overrides_pre_pciess_con...
#include <pcie_ss_common.h>            // for pcie_ss_entity_t
#include <pciess.h>                    // for pciess_get_entity, pciess_con...
#include <silibs_status.h>             // for SILIBS_SUCCESS, silibs_status_t
#include <stdbool.h>                   // for true, false
#include <stdint.h>                    // for uint64_t, uint8_t
#include <stdio.h>                     // for NULL
#include <vab_rpss_top_regs.h>         // for VAB_RPSS_TOP_RPSS_ADDRESS

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static atu_map_entry_t atu_pciess_config_map[NUM_RPSS] = {
    ATU_MAPPING_RPSS0_CFG(),
    ATU_MAPPING_RPSS1_CFG(),
    ATU_MAPPING_RPSS2_CFG(),
    ATU_MAPPING_RPSS3_CFG(),
    ATU_MAPPING_RPSS4_CFG(),
    ATU_MAPPING_RPSS5_CFG(),
    ATU_MAPPING_RPSS6_CFG(),
    ATU_MAPPING_RPSS7_CFG(),
};

static uint64_t rpss_addrs[NUM_RPSS] = {
    AP_TOP_D0_VAB_RPSS0_ADDRESS,
    AP_TOP_D0_VAB_RPSS1_ADDRESS,
    AP_TOP_D0_VAB_RPSS2_ADDRESS,
    AP_TOP_D0_VAB_RPSS3_ADDRESS,
    AP_TOP_D1_VAB_RPSS0_ADDRESS,
    AP_TOP_D1_VAB_RPSS1_ADDRESS,
    AP_TOP_D1_VAB_RPSS2_ADDRESS,
    AP_TOP_D1_VAB_RPSS3_ADDRESS,
};

static pcie_cfg_t pcie_cfg_np = {
    .pcie_cap_gen_speed = PCIE_GEN_5,
    .pcie_bifurcation_mode = PCIE_BIFURCATION_1X16,
    .pcie_cxl_support = false,
    .pcie_stagger_time = PCIE_STAGGER_NONE,
    .pcie_clock_mode = PCIE_CLOCK_SRNS,
    .pcie_fast_link_mode = true,
    .pcie_gen3_eq_disable = true,
    .pcie_gen4_eq_disable = true,
    .pcie_gen5_eq_disable = true,
    .pcie_eq_bypass_support = true,
    .pcie_no_eq_support = true,
};

/*------------- Functions ----------------*/
int begin_rpss_init(PDFWK_SYNC_REQUEST_HEADER req)
{
    pcie_sync_request_t* r = (pcie_sync_request_t*)req;
    uint8_t rpss_id = r->rpss_index;
    silibs_status_t sts = SILIBS_SUCCESS;

    /* Map rpss in the ATU */
    sts = atu_map(ATU_ID_MSCP, &atu_pciess_config_map[rpss_id]);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    /* Setup resolved addresses */
    uint64_t ap_subsystem_config_addr = rpss_addrs[rpss_id] + VAB_RPSS_TOP_RPSS_ADDRESS;
    uint64_t resolved_subsystem_config_addr = atu_pciess_config_map[rpss_id].mscp_start_address;

    pcie_ss_entity_t* rpss = pciess_get_entity((RPSS_INSTANCE)rpss_id);
    FPFW_RUNTIME_ASSERT(rpss != NULL);

    sts = pciess_config_entity(rpss, resolved_subsystem_config_addr, ap_subsystem_config_addr, &pcie_cfg_np, true, true);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    /* Override settings based on the platform we are running on */
    plat_overrides_pre_pciess_config_ss_for_bifur(rpss);

    sts = pciess_config_ss_for_bifur(rpss);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    sts = pciess_deassert_por_reset(rpss);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    /* Do not unmap ATU entries as they are required for runtime handling */
    return sts;
}

int begin_rpss_pre_rp_ready_init(PDFWK_SYNC_REQUEST_HEADER req)
{
    pcie_sync_request_t* r = (pcie_sync_request_t*)req;
    silibs_status_t sts = SILIBS_SUCCESS;
    pcie_ss_entity_t* rpss = pciess_get_entity(r->rpss_index);
    FPFW_RUNTIME_ASSERT(rpss != NULL);

    /*
     * Skip SRAM init done check on sim platforms that don't have a PHY.
     * SVP bug to fix this to allow testing -
     * https://azurecsi.visualstudio.com/1P-SoC-Modeling/_workitems/edit/1841638
     */
    if (idsw_get_platform_sdv() == PLATFORM_RVP_EVT_SILICON)
    {
        sts = pciess_phys_sram_init_done(rpss);
        FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);
    }

    sts = pciess_rps_pre_rp_ready_init(rpss);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    return sts;
}

int begin_rpss_post_rp_ready_init(PDFWK_SYNC_REQUEST_HEADER req)
{
    pcie_sync_request_t* r = (pcie_sync_request_t*)req;
    silibs_status_t sts = SILIBS_SUCCESS;

    pcie_ss_entity_t* rpss = pciess_get_entity(r->rpss_index);
    FPFW_RUNTIME_ASSERT(rpss != NULL);

    /*
     * CDM in reset never goes to 0x0 on SVP so skip it for now.
     * SVP Bug 1841610
     * https://azurecsi.visualstudio.com/1P-SoC-Modeling/_workitems/edit/1841610
     */
    if (idsw_get_platform_sdv() != PLATFORM_SVP_SIM)
    {
        sts = pciess_rps_ready(rpss);
        FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);
    }

    sts = pciess_rps_post_rp_ready_init(rpss);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    return sts;
}
