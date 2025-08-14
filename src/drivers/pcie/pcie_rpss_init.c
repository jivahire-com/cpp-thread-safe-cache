//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_rpss_init.c
 * Instantiates PCIe root port devices using SCP driver framework.
 */

/*------------- Includes -----------------*/
#define __NO_CSR_TYPEDEFS__     // Needed to avoid huge buffers in ap_top_regs.h
#define __NO_ADDRMAP_TYPEDEFS__ // Needed to avoid huge buffers in ap_top_regs.h
#include <DbgPrint.h>
#include <DfwkDriver.h>
#include <ap_top_regs.h>
#include <atu_lib.h>
#include <bug_check.h>
#include <fpfw_cfg_mgr.h>
#include <idsw_kng.h>
#include <kng_atu_mappings.h>
#include <kng_soc_constants.h>
#include <mscp_exp_rmss_memory_map.h>
#include <oi_pcie.h>
#include <pcie_config_i.h>
#include <pcie_dfwk.h>
#include <pcie_platform_overrides_i.h>
#include <pcie_rpss_init_i.h>
#include <pcie_ss_common.h>
#include <pciess.h>
#include <silibs_status.h>
#include <stdbool.h>
#include <stdint.h>
#include <vab.h>
#include <vab_rpss_top_regs.h>

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

/*------------- Functions ----------------*/
int begin_rpss_init(PDFWK_SYNC_REQUEST_HEADER req)
{
    pcie_sync_request_t* r = (pcie_sync_request_t*)req;
    uint8_t rpss_id = r->rpss_index;
    silibs_status_t sts = SILIBS_SUCCESS;

    /* Map rpss in the ATU */
    sts = atu_map(ATU_ID_MSCP, &atu_pciess_config_map[rpss_id]);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, rpss_id, sts);

    /* Setup resolved addresses */
    uint64_t ap_subsystem_config_addr = rpss_addrs[rpss_id] + VAB_RPSS_TOP_RPSS_ADDRESS;
    uint64_t resolved_subsystem_config_addr = atu_pciess_config_map[rpss_id].mscp_start_address;

    pcie_ss_entity_t* rpss = pciess_get_entity(rpss_id);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, rpss_id, sts);

    override_default_pcie_cfg(rpss_id);

    sts = pciess_config_entity(rpss,
                               resolved_subsystem_config_addr,
                               ap_subsystem_config_addr,
                               get_configuration_for_rpss(rpss_id),
                               plat_get_phy_programming_support(),
                               true);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, rpss_id, sts);

    /* Override settings based on the platform we are running on */
    plat_overrides_pre_pciess_config_ss_for_bifur(rpss);

    sts = pciess_config_ss_for_bifur(rpss);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, rpss_id, sts);

    sts = pciess_deassert_por_reset(rpss);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, rpss_id, sts);

    sts = pciess_phys_toggle_clocks(rpss);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, rpss_id, sts);

    pciess_device_interface_t* iface = (pciess_device_interface_t*)(req->OwningInterface);
    pciess_device_t* dev = (pciess_device_t*)(iface->dev);
    populate_rb_configs_from_rpss_entity(rpss, dev->rb_configs);

    /* Do not unmap ATU entries as they are required for runtime handling */
    return sts;
}

int begin_rpss_pre_rp_ready_init(PDFWK_SYNC_REQUEST_HEADER req)
{
    pcie_sync_request_t* r = (pcie_sync_request_t*)req;
    silibs_status_t sts = SILIBS_SUCCESS;
    pcie_ss_entity_t* rpss = pciess_get_entity(r->rpss_index);
    pcie_phy_fw_t phyfw = {0};

    // Set PCIE PHY Loaded flag
    phyfw.loaded = true;
    phyfw.base = (uintptr_t)SCP_EXP_PCIE_PHY_FW_BASE;

    BUG_ASSERT(rpss != NULL);

    if (plat_get_phy_programming_support() == true)
    {
        sts = pciess_poll_phys_sram_init_done(rpss);
        if (sts == SILIBS_E_TIMEOUT)
        {
            FPFW_DBGPRINT_WARNING("RPSS[%d]: PHY SRAM init not asserted, timeout!\n", rpss->id);
        }

        // There is no PHY Loaded other than the this, so any failure here should be
        // treated as FATAL
        sts = pciess_phys_program_fw(rpss, &phyfw);
        BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, rpss->id, sts);
    }

    sts = pciess_rps_pre_rp_ready_init(rpss);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, rpss->id, sts);

    return sts;
}

int begin_rpss_post_rp_ready_init(PDFWK_SYNC_REQUEST_HEADER req)
{
    pcie_sync_request_t* r = (pcie_sync_request_t*)req;
    silibs_status_t sts = SILIBS_SUCCESS;

    pcie_ss_entity_t* rpss = pciess_get_entity(r->rpss_index);
    BUG_ASSERT(rpss != NULL);

    sts = pciess_rps_ready(rpss);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, rpss->id, sts);

    sts = pciess_rps_post_rp_ready_init(rpss);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, rpss->id, sts);

    sts = pciess_rps_clear_intus(rpss);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, rpss->id, sts);

    /* Enable RPSS VAB ISRs now as the rpss is programmed and ready to go */
    enable_vab_isrs((1 << rpss->id));

    pcie_prod_cfg_workarounds_t* rpss_workarounds = get_workaround_for_rpss(rpss->id);

    for (uint8_t i = 0; i < PCIESS_NUM_PORTS; i++)
    {
        if (rpss_workarounds->prod_rp_cfgs[i].hide_dpc == true)
        {
            sts = oi_pcie_rp_dbi_hide_dpc_cap(&(rpss->rps[i]));
            BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, rpss->id, sts);
        }
        /* TODO: If mscp is going to have complete control over this register set by knobs,
         *   the interface enums for this call must be passed up by silibs knobs headers
         *   to mscp xml
         */

        // Apply force read allocate
        pcie_laattr_ovrd_t overrides_r = {0};
        // Default override values, matches POR values
        overrides_r.tph_override = PCIE_LAATTR_WB_A_OS;
        overrides_r.notph_override = PCIE_LAATTR_WB_A_OS;
        overrides_r.nosnoop_override = PCIE_LAATTR_WB_A_OS;

        // Read tph
        if (rpss_workarounds->prod_rp_cfgs[i].lattr_tph_en_read == true)
        {
            overrides_r.tph_override_op = PCIE_LAATTR_OVERRIDE;
        }
        else
        {
            overrides_r.tph_override_op = PCIE_LAATTR_OVERRIDE_DISABLE;
        }
        // Read notph
        if (rpss_workarounds->prod_rp_cfgs[i].lattr_notph_en_read == true)
        {
            overrides_r.notph_override_op = PCIE_LAATTR_OVERRIDE;
        }
        else
        {
            overrides_r.notph_override_op = PCIE_LAATTR_OVERRIDE_DISABLE;
        }
        // Read no snoop
        if (rpss_workarounds->prod_rp_cfgs[i].lattr_nosnoop_en_read == true)
        {
            overrides_r.nosnoop_override_op = PCIE_LAATTR_OVERRIDE;
        }
        else
        {
            overrides_r.nosnoop_override_op = PCIE_LAATTR_OVERRIDE_DISABLE;
        }
        sts = oi_pcie_ss_set_laattr_rp_overrides(rpss, i, &overrides_r, false);
        BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, rpss->id, sts);

        // Apply force write allocate
        pcie_laattr_ovrd_t overrides_w = {0};
        overrides_w.tph_override = PCIE_LAATTR_WB_A_OS;
        overrides_w.notph_override = PCIE_LAATTR_WB_A_OS;
        overrides_w.nosnoop_override = PCIE_LAATTR_WB_A_OS;

        // Read tph
        if (rpss_workarounds->prod_rp_cfgs[i].lattr_tph_en_write == true)
        {
            overrides_w.tph_override_op = PCIE_LAATTR_OVERRIDE;
        }
        else
        {
            overrides_w.tph_override_op = PCIE_LAATTR_OVERRIDE_DISABLE;
        }
        // Read notph
        if (rpss_workarounds->prod_rp_cfgs[i].lattr_notph_en_write == true)
        {
            overrides_w.notph_override_op = PCIE_LAATTR_OVERRIDE;
        }
        else
        {
            overrides_w.notph_override_op = PCIE_LAATTR_OVERRIDE_DISABLE;
        }
        // Read no snoop
        if (rpss_workarounds->prod_rp_cfgs[i].lattr_nosnoop_en_write == true)
        {
            overrides_w.nosnoop_override_op = PCIE_LAATTR_OVERRIDE;
        }
        else
        {
            overrides_w.nosnoop_override_op = PCIE_LAATTR_OVERRIDE_DISABLE;
        }
        sts = oi_pcie_ss_set_laattr_rp_overrides(rpss, i, &overrides_w, true);
        BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, rpss->id, sts);
    }

    return sts;
}

void begin_link_training(PDFWK_SYNC_REQUEST_HEADER req)
{
    pcie_sync_request_t* r = (pcie_sync_request_t*)req;

    pcie_ss_entity_t* rpss = pciess_get_entity(r->rpss_index);
    BUG_ASSERT(rpss != NULL);

    pciess_rp_initiate_link_training(&(rpss->rps[r->rp_index]));
}

int begin_rp_post_link_up_init(PDFWK_SYNC_REQUEST_HEADER req)
{
    pcie_sync_request_t* r = (pcie_sync_request_t*)req;
    silibs_status_t sts = SILIBS_SUCCESS;

    if (plat_post_link_up_init_needed() == false)
    {
        return sts;
    }

    pcie_ss_entity_t* rpss = pciess_get_entity(r->rpss_index);
    BUG_ASSERT(rpss != NULL);

    sts = pciess_rp_post_link_up_init(&(rpss->rps[r->rp_index]));
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, rpss->id, sts);

    return sts;
}

int get_rp_ready(PDFWK_SYNC_REQUEST_HEADER req)
{
    pcie_sync_request_t* r = (pcie_sync_request_t*)req;
    silibs_status_t sts = SILIBS_SUCCESS;

    pcie_ss_entity_t* rpss = pciess_get_entity(r->rpss_index);
    BUG_ASSERT(rpss != NULL);

    sts = pciess_rp_ready(&(rpss->rps[r->rp_index]));

    return sts;
}

int get_rp_link_status(PDFWK_SYNC_REQUEST_HEADER req)
{
    pcie_sync_request_t* r = (pcie_sync_request_t*)req;
    silibs_status_t sts = SILIBS_SUCCESS;

    pcie_ss_entity_t* rpss = pciess_get_entity(r->rpss_index);
    BUG_ASSERT(rpss != NULL);

    sts = pciess_rp_get_link_train_done(&(rpss->rps[r->rp_index]));

    return sts;
}
