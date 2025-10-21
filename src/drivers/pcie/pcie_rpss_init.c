//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_rpss_init.c
 * Instantiates PCIe root port devices using SCP driver framework.
 */

/*------------- Includes -----------------*/
#include <health_monitor.h>
#define __NO_CSR_TYPEDEFS__     // Needed to avoid huge buffers in ap_top_regs.h
#define __NO_ADDRMAP_TYPEDEFS__ // Needed to avoid huge buffers in ap_top_regs.h
#include <DbgPrint.h>
#include <DfwkDriver.h>
#include <DfwkPtrTypes.h>
#include <ap_top_regs.h>
#include <bug_check.h>
#include <fpfw_cfg_mgr.h>
#include <idsw_kng.h>
#include <ift_fw.h>
#include <kng_soc_constants.h>
#include <mscp_exp_rmss_memory_map.h>
#include <oi_pcie.h>
#include <pcie_bdat_i.h>
#include <pcie_config_i.h>
#include <pcie_dfwk.h>
#include <pcie_platform_overrides_i.h>
#include <pcie_rp_common.h>
#include <pcie_rp_sii.h>
#include <pcie_rpss_init_i.h>
#include <pcie_ss_common.h>
#include <pciess.h>
#include <silibs_status.h>
#include <stdbool.h>
#include <stdint.h>
#include <vab.h>
#include <vab_atu_mappings.h>
#include <vab_rpss_top_regs.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
#define LT_RETRIES_MAX (3)
/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
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

static acpi_cper_section_t cper_section = {0};

/*------------- Functions ----------------*/
int begin_rpss_init(PDFWK_SYNC_REQUEST_HEADER req)
{
    pcie_sync_request_t* r = (pcie_sync_request_t*)req;
    uint8_t rpss_id = r->rpss_index;
    silibs_status_t sts = SILIBS_SUCCESS;

    /* Setup resolved addresses */
    uint64_t ap_subsystem_config_addr = rpss_addrs[rpss_id] + VAB_RPSS_TOP_RPSS_ADDRESS;
    uint64_t resolved_subsystem_config_addr = get_rpss_resolved_base(rpss_id);

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

    if (!ift_is_enabled())
    {
        sts = pciess_config_ss_for_bifur(rpss);
        BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, rpss_id, sts);

        sts = pciess_deassert_por_reset(rpss);
        BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, rpss_id, sts);

        sts = pciess_phys_toggle_clocks(rpss);
        BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, rpss_id, sts);

        pciess_device_interface_t* iface = (pciess_device_interface_t*)(req->OwningInterface);
        pciess_device_t* dev = (pciess_device_t*)(iface->dev);
        populate_rb_configs_from_rpss_entity(rpss, dev->rb_configs);
    }
    else
    {
        sts = pciess_config_ss_for_ift(rpss);
        BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, rpss_id, sts);
    }

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

    BUG_ASSERT_PARAM(rpss != NULL, rpss, 0);

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

int get_rpss_ready(PDFWK_SYNC_REQUEST_HEADER req)
{
    pcie_sync_request_t* r = (pcie_sync_request_t*)req;
    silibs_status_t sts = SILIBS_SUCCESS;

    pcie_ss_entity_t* rpss = pciess_get_entity(r->rpss_index);
    BUG_ASSERT_PARAM(rpss != NULL, rpss, 0);

    sts = pciess_rps_ready(rpss);

    return sts;
}

int begin_rpss_post_rp_ready_init(PDFWK_SYNC_REQUEST_HEADER req)
{
    pcie_sync_request_t* r = (pcie_sync_request_t*)req;
    silibs_status_t sts = SILIBS_SUCCESS;

    pcie_ss_entity_t* rpss = pciess_get_entity(r->rpss_index);
    BUG_ASSERT_PARAM(rpss != NULL, rpss, 0);

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
    BUG_ASSERT_PARAM(rpss != NULL, rpss, 0);

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
    BUG_ASSERT_PARAM(rpss != NULL, rpss, 0);

    sts = pciess_rp_post_link_up_init(&(rpss->rps[r->rp_index]));
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, rpss->id, sts);

    /* Do not fail boot in case we fail to add BDAT info, as this is non-fatal */
    if (publish_pcie_bdat_info_for_this_rp(rpss, r->rp_index) != SILIBS_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("RPSS[%d] RP[%d]: Publishing BDAT info returned error status: %d\n", rpss->id, r->rp_index, (int8_t)sts);
    }

    return sts;
}

int get_rp_ready(PDFWK_SYNC_REQUEST_HEADER req)
{
    pcie_sync_request_t* r = (pcie_sync_request_t*)req;
    silibs_status_t sts = SILIBS_SUCCESS;

    pcie_ss_entity_t* rpss = pciess_get_entity(r->rpss_index);
    BUG_ASSERT_PARAM(rpss != NULL, rpss, 0);

    sts = pciess_rp_ready(&(rpss->rps[r->rp_index]));

    return sts;
}

static void log_link_training_failure_cper(pcie_ss_entity_t* rpss, uint8_t rp_index)
{
    memset(&cper_section, 0, sizeof(acpi_cper_section_t));
    acpi_err_sec_pcie_vendor_t* pcie_cper = &cper_section.sec_pcie_vendor;
    acpi_error_severity_t severity = ACPI_ERROR_SEVERITY_INFORMATIONAL;

    pcie_rp_populate_cper(&(rpss->rps[rp_index]), pcie_cper, sizeof(acpi_err_sec_pcie_vendor_t));

    /* Fill in data specific to link training errors */
    pcie_cper->error_type = PCIE_LINK_TRAINING_ERROR;
    pcie_cper->link_training_info.ltssm_state = pcie_rp_sii_get_link_state(&(rpss->rps[rp_index]));
    pcie_cper->link_training_info.link_width = rpss->rps[rp_index].current_state.lanecount;
    pcie_cper->link_training_info.link_speed = rpss->rps[rp_index].current_state.linkrate;

    hm_submit_cper(ACPI_ERROR_DOMAIN_PCIE, severity, &cper_section, sizeof(cper_section));
}

int get_rp_link_status(PDFWK_SYNC_REQUEST_HEADER req)
{
    pcie_sync_request_t* r = (pcie_sync_request_t*)req;
    silibs_status_t sts = SILIBS_SUCCESS;
    /*
     *  When LT Time out happens, The retry count is NOT send, a NULL pointer is passed
     *  in which case we anyway have to log a CPER  to indicate Link triaining failed.
     *  So we default the value of the lt_retry_count to MAX value.
     */
    uint8_t lt_retry_count = LT_RETRIES_MAX;

    // Retry Count comes from the Request orginator
    // IF Device is not attached [ aka Link Timer Expires ], this will be set to NULL
    if ((r->p_sent_data) != NULL)
    {
        lt_retry_count = *((uint8_t*)(r->p_sent_data));
        FPFW_DBGPRINT_VERBOSE("RPSS[%d] RP[%d]: Retry Count %d\n", r->rpss_index, r->rp_index, (int8_t)lt_retry_count);
    }

    pcie_ss_entity_t* rpss = pciess_get_entity(r->rpss_index);
    BUG_ASSERT_PARAM(rpss != NULL, rpss, 0);

    /*
     * Log a link training non-FATAL CPER here only if :
     *          - if retry count is >= LT_RETRIES_MAX   AND
     *          - Status is SILIB_E_BUSY or SILIBS_E_OVERWRITTEN
     *
     * SILIBS_E_BUSY - this indicates that link training hasn't completed or the DLL_ACTIVE is not set
     * SILIBS_E_OVERWRITTEN - this indicates that link training completed but the link width or speed is not as expected
     */
    sts = pciess_rp_get_link_train_done(&(rpss->rps[r->rp_index]));
    if (((sts & (SILIBS_E_BUSY | SILIBS_E_OVERWRITTEN)) != 0) && lt_retry_count == LT_RETRIES_MAX)
    {
        FPFW_DBGPRINT_WARNING("RPSS[%d] RP[%d]: Retry Count %d\n", r->rpss_index, r->rp_index, (int8_t)lt_retry_count);
        log_link_training_failure_cper(rpss, r->rp_index);
    }

    return sts;
}
