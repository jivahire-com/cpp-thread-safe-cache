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
#include <DfwkDriver.h>
#include <FpFwAssert.h>
#include <ap_top_regs.h>
#include <atu_lib.h>
#include <fpfw_cfg_mgr.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <kng_atu_mappings.h>
#include <kng_soc_constants.h>
#include <pcie_dfwk.h>
#include <pcie_knobs.h>
#include <pcie_platform_overrides_i.h>
#include <pcie_ss_common.h>
#include <pcie_struct_defaults.h>
#include <pciess.h>
#include <silibs_status.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
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

static pcie_cfg_t pcie_cfg_np[NUM_RPSS] = {
    DEFAULT_PCIE_CFG_T,
    DEFAULT_PCIE_CFG_T,
    DEFAULT_PCIE_CFG_T,
    DEFAULT_PCIE_CFG_T,
    DEFAULT_PCIE_CFG_T,
    DEFAULT_PCIE_CFG_T,
    DEFAULT_PCIE_CFG_T,
    DEFAULT_PCIE_CFG_T,
};

/*------------- Functions ----------------*/
static void override_default_pcie_cfg(uint8_t rpss_id)
{
    // generic defaults
    pcie_cfg_t* pcie_cfg = &pcie_cfg_np[rpss_id];
    pcie_prod_cfg_t pcie_cfg_knob;
    switch (rpss_id)
    {
    case RPSS0:
        pcie_cfg_knob = config_get_pcie_rpss0_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss0_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss0_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss0_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss0_rp3_cfg();
        break;
    case RPSS1:
        pcie_cfg_knob = config_get_pcie_rpss1_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss1_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss1_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss1_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss1_rp3_cfg();
        break;
    case RPSS2:
        pcie_cfg_knob = config_get_pcie_rpss2_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss2_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss2_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss2_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss2_rp3_cfg();
        break;
    case RPSS3:
        pcie_cfg_knob = config_get_pcie_rpss3_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss3_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss3_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss3_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss3_rp3_cfg();
        break;
    case RPSS4:
        pcie_cfg_knob = config_get_pcie_rpss4_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss4_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss4_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss4_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss4_rp3_cfg();
        break;
    case RPSS5:
        pcie_cfg_knob = config_get_pcie_rpss5_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss5_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss5_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss5_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss5_rp3_cfg();
        break;
    case RPSS6:
        pcie_cfg_knob = config_get_pcie_rpss6_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss6_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss6_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss6_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss6_rp3_cfg();
        break;
    case RPSS7:
        pcie_cfg_knob = config_get_pcie_rpss7_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss7_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss7_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss7_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss7_rp3_cfg();
        break;
    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
    pcie_cfg->pcie_ss_en = pcie_cfg_knob.pcie_ss_en;
    pcie_cfg->pcie_bifurcation_mode = pcie_cfg_knob.pcie_bifurcation_mode;
    pcie_cfg->pcie_clock_mode = pcie_cfg_knob.pcie_clock_mode;
    pcie_cfg->pcie_stagger_time = pcie_cfg_knob.pcie_stagger_time;
    pcie_cfg->pcie_aspm_support = pcie_cfg_knob.pcie_aspm_support;
    pcie_cfg->pcie_ltr_support = pcie_cfg_knob.pcie_ltr_support;
    pcie_cfg->pcie_l1_exit_latency = pcie_cfg_knob.pcie_l1_exit_latency;
    pcie_cfg->pcie_l0s_exit_latency = pcie_cfg_knob.pcie_l0s_exit_latency;
    pcie_cfg->pcie_loopback_mode = pcie_cfg_knob.pcie_loopback_mode;
    pcie_cfg->pcie_cap_gen_speed = pcie_cfg_knob.pcie_cap_gen_speed;
    pcie_cfg->pcie_cap_mps = pcie_cfg_knob.pcie_cap_mps;
    pcie_cfg->pcie_rcb_mode = pcie_cfg_knob.pcie_rcb_mode;
    pcie_cfg->pcie_ras_mode = pcie_cfg_knob.pcie_ras_mode;
    pcie_cfg->pcie_fast_link_mode = pcie_cfg_knob.pcie_fast_link_mode;
    pcie_cfg->pcie_gen3_eq_disable = pcie_cfg_knob.pcie_gen3_eq_disable;
    pcie_cfg->pcie_gen4_eq_disable = pcie_cfg_knob.pcie_gen4_eq_disable;
    pcie_cfg->pcie_gen5_eq_disable = pcie_cfg_knob.pcie_gen5_eq_disable;
    pcie_cfg->pcie_eq_bypass_support = pcie_cfg_knob.pcie_eq_bypass_support;
    pcie_cfg->pcie_no_eq_support = pcie_cfg_knob.pcie_no_eq_support;
    pcie_cfg->pcie_cxl_support = pcie_cfg_knob.pcie_cxl_support;
    pcie_cfg->pcie_cxl_sync_header_bypass = pcie_cfg_knob.pcie_cxl_sync_header_bypass;
    pcie_cfg->pcie_ide_support = pcie_cfg_knob.pcie_ide_support;
    pcie_cfg->pcie_tee_support = pcie_cfg_knob.pcie_tee_support;
    pcie_cfg->pcie_fips_test = pcie_cfg_knob.pcie_fips_test;
    pcie_cfg->pcie_ide_rekey_support = pcie_cfg_knob.pcie_ide_rekey_support;
    pcie_cfg->pcie_ide_sync_msg_threshold = pcie_cfg_knob.pcie_ide_sync_msg_threshold;
    pcie_cfg->pcie_msi_ext_data_support = pcie_cfg_knob.pcie_msi_ext_data_support;
    pcie_cfg->pcie_msi_64bit_support = pcie_cfg_knob.pcie_msi_64bit_support;
    pcie_cfg->pcie_system_counter_delay = pcie_cfg_knob.pcie_system_counter_delay;
    pcie_cfg->pcie_aer_ecrc_gen_support = pcie_cfg_knob.pcie_aer_ecrc_gen_support;
    pcie_cfg->pcie_aer_ecrc_check_support = pcie_cfg_knob.pcie_aer_ecrc_check_support;
    pcie_cfg->pcie_aer_multiple_header_support = pcie_cfg_knob.pcie_aer_multiple_header_support;

    /* Root Port Defaults not set as in 1x16 Config they are
       not going to be used. If 4x4 bifurcation is used and RP
       are not being used, we have to explicitly set them to false */
}

static void populate_rb_configs_from_rpss_entity(pcie_ss_entity_t* rpss, pcie_root_bridge_config* rb_configs)
{
    pcie_cfg_t* pcie_cfg = &pcie_cfg_np[rpss->id];
    for (uint8_t i = 0; i < PCIESS_NUM_PORTS; i++)
    {
        if (rpss->rps[i].valid == false || rpss->rps[i].enabled == false)
        {
            rb_configs[i].flags.is_enabled = false;
            continue;
        }

        rb_configs[i].flags.is_enabled = rpss->rps[i].enabled;
        rb_configs[i].flags.hot_plug_enabled = pcie_cfg->rp_cfgs[i].pcie_rp_hotplug_capable;
        rb_configs[i].flags.is_cxl = pcie_cfg->pcie_cxl_support && (i == PCIESS_CXL_RP_IDX);
        rb_configs[i].slot_number = pcie_cfg->rp_cfgs[i].pcie_rp_slot_num;

        rb_configs[i].mmiol.base = rpss->ranges[i].mmiol_start;
        rb_configs[i].mmiol.limit = rpss->ranges[i].mmiol_end;

        rb_configs[i].mmioh.base = rpss->ranges[i].mmioh_start;
        rb_configs[i].mmioh.limit = rpss->ranges[i].mmioh_end;

        rb_configs[i].bus.base = rpss->rps[i].bus_cfg.primary_bus;
        rb_configs[i].bus.limit = rpss->rps[i].bus_cfg.subordinate_bus;
    }
}

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

    pcie_ss_entity_t* rpss = pciess_get_entity(rpss_id);
    FPFW_RUNTIME_ASSERT(rpss != NULL);

    override_default_pcie_cfg(rpss_id);

    sts = pciess_config_entity(rpss,
                               resolved_subsystem_config_addr,
                               ap_subsystem_config_addr,
                               &(pcie_cfg_np[rpss_id]),
                               plat_get_phy_programming_support(),
                               true);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    /* Override settings based on the platform we are running on */
    plat_overrides_pre_pciess_config_ss_for_bifur(rpss);

    // TODO: this API fails on SVP, due to ras APIs failing
    // Renable once silibs skips RAS init on SVP
    // ADO: https://dev.azure.com/ms-tsd/Kingsgate/_workitems/edit/797391/
    sts = pciess_config_ss_for_bifur(rpss);
    if (!IS_PLATFORM_SVP())
    {
        FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);
    }

    sts = pciess_deassert_por_reset(rpss);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

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
    FPFW_RUNTIME_ASSERT(rpss != NULL);

    if (plat_get_phy_programming_support() == true)
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

    sts = pciess_rps_ready(rpss);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    sts = pciess_rps_post_rp_ready_init(rpss);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    sts = pciess_rps_clear_intus(rpss);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    /* Enable RPSS VAB ISRs now as the rpss is programmed and ready to go */
    enable_vab_isrs((1 << rpss->id));

    /*
     * Platfrom Override program GIC comparator
     * Task 2013275: Platform Override: add programming of RPSS MSI comparator
     */
    plat_overrides_post_rp_ready(rpss);

    return sts;
}

void begin_link_training(PDFWK_SYNC_REQUEST_HEADER req)
{
    pcie_sync_request_t* r = (pcie_sync_request_t*)req;

    pcie_ss_entity_t* rpss = pciess_get_entity(r->rpss_index);
    FPFW_RUNTIME_ASSERT(rpss != NULL);

    pciess_rp_initiate_link_training(&(rpss->rps[r->rp_index]));
}
