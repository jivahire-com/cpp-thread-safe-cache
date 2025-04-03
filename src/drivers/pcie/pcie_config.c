//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_config.c
 * Apply and setup pciess configuration settings retrieved from the
 * configuration manager.
 */

/*------------- Includes -----------------*/
#include <FpFwAssert.h>
#include <fpfw_cfg_mgr.h>
#include <kng_soc_constants.h>
#include <mu_public.h>
#include <pcie_common.h>
#include <pcie_config_i.h>
#include <pcie_knobs.h>
#include <pcie_struct_defaults.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <system_info.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static bool mirror_rpss_configurations();
static void apply_one_to_one_configurations(uint8_t rpss_id,
                                            pcie_cfg_t* pcie_cfg,
                                            pcie_prod_cfg_t* pcie_cfg_knob,
                                            pcie_prod_phy_cfg_t* phy_cfg_knob);
static void apply_mirrored_configurations(uint8_t rpss_id,
                                          pcie_cfg_t* pcie_cfg,
                                          pcie_prod_cfg_t* pcie_cfg_knob,
                                          pcie_prod_phy_cfg_t* phy_cfg_knob);

/*-- Declarations (Statics and globals) --*/
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
static bool mirror_rpss_configurations()
{
    bool config_mirroring = false;
    uint8_t soc_position = BOARD_ID_GET_SOC_POSITION(system_info_get_board_id());

    /* Only mirror PCIe configuration if we are on SoC 1 and the mirroring knob is set to true */
    if ((soc_position == 0x01) && (config_get_pcie_configuration_mirroring() == true))
    {
        config_mirroring = true;
    }

    return config_mirroring;
}

static void apply_one_to_one_configurations(uint8_t rpss_id, pcie_cfg_t* pcie_cfg, pcie_prod_cfg_t* pcie_cfg_knob, pcie_prod_phy_cfg_t* phy_cfg_knob)
{
    switch (rpss_id)
    {
    case RPSS0:
        *pcie_cfg_knob = config_get_pcie_rpss0_cfg();
        *phy_cfg_knob = config_get_pcie_rpss0_phy_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss0_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss0_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss0_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss0_rp3_cfg();
        break;
    case RPSS1:
        *pcie_cfg_knob = config_get_pcie_rpss1_cfg();
        *phy_cfg_knob = config_get_pcie_rpss1_phy_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss1_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss1_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss1_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss1_rp3_cfg();
        break;
    case RPSS2:
        *pcie_cfg_knob = config_get_pcie_rpss2_cfg();
        *phy_cfg_knob = config_get_pcie_rpss2_phy_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss2_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss2_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss2_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss2_rp3_cfg();
        break;
    case RPSS3:
        *pcie_cfg_knob = config_get_pcie_rpss3_cfg();
        *phy_cfg_knob = config_get_pcie_rpss3_phy_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss3_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss3_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss3_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss3_rp3_cfg();
        break;
    case RPSS4:
        *pcie_cfg_knob = config_get_pcie_rpss4_cfg();
        *phy_cfg_knob = config_get_pcie_rpss4_phy_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss4_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss4_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss4_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss4_rp3_cfg();
        break;
    case RPSS5:
        *pcie_cfg_knob = config_get_pcie_rpss5_cfg();
        *phy_cfg_knob = config_get_pcie_rpss5_phy_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss5_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss5_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss5_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss5_rp3_cfg();
        break;
    case RPSS6:
        *pcie_cfg_knob = config_get_pcie_rpss6_cfg();
        *phy_cfg_knob = config_get_pcie_rpss6_phy_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss6_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss6_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss6_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss6_rp3_cfg();
        break;
    case RPSS7:
        *pcie_cfg_knob = config_get_pcie_rpss7_cfg();
        *phy_cfg_knob = config_get_pcie_rpss7_phy_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss7_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss7_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss7_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss7_rp3_cfg();
        break;
    default:
        /* Invalid RPSS ID */
        /* This should never happen */
        printf("[PCIe Configuration] Critical error! Invalid RPSS ID: %d\n", rpss_id);
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
}

static void apply_mirrored_configurations(uint8_t rpss_id, pcie_cfg_t* pcie_cfg, pcie_prod_cfg_t* pcie_cfg_knob, pcie_prod_phy_cfg_t* phy_cfg_knob)
{
    /*
     * Apply PCIe configurations for mirrored 2 socket boards.
     *
     * The pattern followed is as follows:
     * ----- Die 0 Configuration -----
     * RPSS0 <- inherits configuration for -> RPSS3
     * RPSS1 <- inherits configuration for -> RPSS2
     * RPSS2 <- inherits configuration for -> RPSS1
     * RPSS3 <- inherits configuration for -> RPSS0
     * ----- Die 1 Configuration -----
     * RPSS4 <- inherits configuration for -> RPSS7
     * RPSS5 <- inherits configuration for -> RPSS6
     * RPSS6 <- inherits configuration for -> RPSS5
     * RPSS7 <- inherits configuration for -> RPSS4
     */
    switch (rpss_id)
    {
    case RPSS0:
        *pcie_cfg_knob = config_get_pcie_rpss3_cfg();
        *phy_cfg_knob = config_get_pcie_rpss3_phy_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss3_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss3_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss3_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss3_rp3_cfg();
        break;
    case RPSS1:
        *pcie_cfg_knob = config_get_pcie_rpss2_cfg();
        *phy_cfg_knob = config_get_pcie_rpss2_phy_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss2_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss2_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss2_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss2_rp3_cfg();
        break;
    case RPSS2:
        *pcie_cfg_knob = config_get_pcie_rpss1_cfg();
        *phy_cfg_knob = config_get_pcie_rpss1_phy_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss1_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss1_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss1_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss1_rp3_cfg();
        break;
    case RPSS3:
        *pcie_cfg_knob = config_get_pcie_rpss0_cfg();
        *phy_cfg_knob = config_get_pcie_rpss0_phy_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss0_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss0_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss0_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss0_rp3_cfg();
        break;
    case RPSS4:
        *pcie_cfg_knob = config_get_pcie_rpss7_cfg();
        *phy_cfg_knob = config_get_pcie_rpss7_phy_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss7_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss7_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss7_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss7_rp3_cfg();
        break;
    case RPSS5:
        *pcie_cfg_knob = config_get_pcie_rpss6_cfg();
        *phy_cfg_knob = config_get_pcie_rpss6_phy_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss6_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss6_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss6_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss6_rp3_cfg();
        break;
    case RPSS6:
        *pcie_cfg_knob = config_get_pcie_rpss5_cfg();
        *phy_cfg_knob = config_get_pcie_rpss5_phy_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss5_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss5_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss5_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss5_rp3_cfg();
        break;
    case RPSS7:
        *pcie_cfg_knob = config_get_pcie_rpss4_cfg();
        *phy_cfg_knob = config_get_pcie_rpss4_phy_cfg();
        pcie_cfg->rp_cfgs[0] = config_get_pcie_rpss4_rp0_cfg();
        pcie_cfg->rp_cfgs[1] = config_get_pcie_rpss4_rp1_cfg();
        pcie_cfg->rp_cfgs[2] = config_get_pcie_rpss4_rp2_cfg();
        pcie_cfg->rp_cfgs[3] = config_get_pcie_rpss4_rp3_cfg();
        break;
    default:
        /* Invalid RPSS ID */
        /* This should never happen */
        printf("[PCIe Configuration] Critical error! Invalid RPSS ID: %d\n", rpss_id);
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
}

void override_default_pcie_cfg(uint8_t rpss_id)
{
    pcie_cfg_t* pcie_cfg = &pcie_cfg_np[rpss_id];
    pcie_prod_cfg_t pcie_cfg_knob;
    pcie_prod_phy_cfg_t phy_cfg_knob;

    if (mirror_rpss_configurations() == true)
    {
        apply_mirrored_configurations(rpss_id, pcie_cfg, &pcie_cfg_knob, &phy_cfg_knob);
    }
    else
    {
        apply_one_to_one_configurations(rpss_id, pcie_cfg, &pcie_cfg_knob, &phy_cfg_knob);
    }

    pcie_cfg->pcie_ss_override = false; // Disable so that individual RPSS settings are used
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

    /* pcie_cfg->pcie_cxl_support set below */
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

    /* Apply PHY knobs per lane*/
    for (uint8_t i = 0; i < PCIE_LANE_COUNT; i++)
    {
        pcie_cfg->phy_lane_cfgs[i] = phy_cfg_knob.phy_lane_cfgs[i];
    }

    /* Determine if CXL is supported on this RPSS */
    cxl_region_params_t cxl_region_params =
        (idsw_get_die_id() == 0) ? config_get_cxl_params_die0() : config_get_cxl_params_die1();

    if (cxl_region_params.valid == false)
    {
        return;
    }

    /*
     * Loop through CXL ports and check whether this RPSS is one of them.
     *
     * If interleaving is disabled, we only need to check the first port
     * If interleaving is enabled, we need to check all ports up to interleave_ways
     */
    for (uint8_t i = 0; i < cxl_region_params.interleave_ways ||
                        (i == PCIESS_CXL_RP_IDX && cxl_region_params.interleave_ways == INTERLEAVE_NONE);
         i++)
    {
        if (cxl_region_params.ports[i] == (CXL_PORT)rpss_id)
        {
            pcie_cfg->pcie_cxl_support = true;
        }
    }

    /*
     * Root Port defaults aren't set as in 1x16 configurations they are unused.
     * If 4x4 bifurcation is used and root ports are not being used, we have to
     * explicitly set them to false.
     */
}

void populate_rb_configs_from_rpss_entity(pcie_ss_entity_t* rpss, pcie_root_bridge_config* rb_configs)
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
        rb_configs[i].flags.is_secondary_soc =
            (BOARD_ID_GET_SOC_POSITION(system_info_get_board_id()) == 0x01) ? 0b1 : 0b0;

        rb_configs[i].mmiol.base = rpss->ranges[i].mmiol_start;
        rb_configs[i].mmiol.limit = rpss->ranges[i].mmiol_end;

        rb_configs[i].mmioh.base = rpss->ranges[i].mmioh_start;
        rb_configs[i].mmioh.limit = rpss->ranges[i].mmioh_end;

        rb_configs[i].bus.base = rpss->rps[i].bus_cfg.primary_bus;
        rb_configs[i].bus.limit = rpss->rps[i].bus_cfg.subordinate_bus;
    }
}

pcie_cfg_t* get_configuration_for_rpss(uint8_t rpss_id)
{
    pcie_cfg_t* pcie_cfg = NULL;

    if (rpss_id >= NUM_RPSS)
    {
        /* Invalid RPSS ID */
        /* This should never happen */
        printf("[PCIe Configuration] Critical error! Invalid RPSS ID: %d\n", rpss_id);
        FPFW_RUNTIME_ASSERT(false);
    }

    pcie_cfg = &pcie_cfg_np[rpss_id];

    return pcie_cfg;
}
