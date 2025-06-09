//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_config.c
 * Apply and setup pciess configuration settings retrieved from the
 * configuration manager.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
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
#include <system_info.h>

/*-- Symbolic Constant Macros (defines) --*/
// Default value for all workaround knobs
#define DEFAULT_PCIE_PROD_CFG_WORKAROUNDS_T                                                   \
    {                                                                                         \
        .prod_rp_cfgs = {                                                                     \
            {.hide_dpc = false, .force_read_allocate = false, .force_write_allocate = false}, \
            {.hide_dpc = false, .force_read_allocate = false, .force_write_allocate = false}, \
            {.hide_dpc = false, .force_read_allocate = false, .force_write_allocate = false}, \
            {.hide_dpc = false, .force_read_allocate = false, .force_write_allocate = false}, \
        }                                                                                     \
    }

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static bool mirror_rpss_configurations();
static void apply_one_to_one_configurations(uint8_t rpss_id,
                                            pcie_cfg_t* pcie_cfg,
                                            pcie_prod_cfg_t* pcie_cfg_knob,
                                            pcie_prod_phy_cfg_t* phy_cfg_knob,
                                            pcie_prod_cfg_workarounds_t* pcie_cfg_workarounds);
static void apply_mirrored_configurations(uint8_t rpss_id,
                                          pcie_cfg_t* pcie_cfg,
                                          pcie_prod_cfg_t* pcie_cfg_knob,
                                          pcie_prod_phy_cfg_t* phy_cfg_knob,
                                          pcie_prod_cfg_workarounds_t* pcie_cfg_workarounds);

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

static pcie_prod_cfg_workarounds_t pcie_cfg_workarounds_np[NUM_RPSS] = {
    DEFAULT_PCIE_PROD_CFG_WORKAROUNDS_T, // RPSS0
    DEFAULT_PCIE_PROD_CFG_WORKAROUNDS_T, // RPSS1
    DEFAULT_PCIE_PROD_CFG_WORKAROUNDS_T, // RPSS2
    DEFAULT_PCIE_PROD_CFG_WORKAROUNDS_T, // RPSS3
    DEFAULT_PCIE_PROD_CFG_WORKAROUNDS_T, // RPSS4
    DEFAULT_PCIE_PROD_CFG_WORKAROUNDS_T, // RPSS5
    DEFAULT_PCIE_PROD_CFG_WORKAROUNDS_T, // RPSS6
    DEFAULT_PCIE_PROD_CFG_WORKAROUNDS_T  // RPSS7
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

static void apply_one_to_one_configurations(uint8_t rpss_id,
                                            pcie_cfg_t* pcie_cfg,
                                            pcie_prod_cfg_t* pcie_cfg_knob,
                                            pcie_prod_phy_cfg_t* phy_cfg_knob,
                                            pcie_prod_cfg_workarounds_t* pcie_cfg_workarounds)
{
    pcie_prod_rp_cfg_t rp_knobs[4];
    switch (rpss_id)
    {
    case RPSS0:
        *pcie_cfg_knob = config_get_pcie_rpss0_cfg();
        *phy_cfg_knob = config_get_pcie_rpss0_phy_cfg();
        rp_knobs[0] = config_get_pcie_rpss0_rp0_cfg();
        rp_knobs[1] = config_get_pcie_rpss0_rp1_cfg();
        rp_knobs[2] = config_get_pcie_rpss0_rp2_cfg();
        rp_knobs[3] = config_get_pcie_rpss0_rp3_cfg();
        break;
    case RPSS1:
        *pcie_cfg_knob = config_get_pcie_rpss1_cfg();
        *phy_cfg_knob = config_get_pcie_rpss1_phy_cfg();
        rp_knobs[0] = config_get_pcie_rpss1_rp0_cfg();
        rp_knobs[1] = config_get_pcie_rpss1_rp1_cfg();
        rp_knobs[2] = config_get_pcie_rpss1_rp2_cfg();
        rp_knobs[3] = config_get_pcie_rpss1_rp3_cfg();
        break;
    case RPSS2:
        *pcie_cfg_knob = config_get_pcie_rpss2_cfg();
        *phy_cfg_knob = config_get_pcie_rpss2_phy_cfg();
        rp_knobs[0] = config_get_pcie_rpss2_rp0_cfg();
        rp_knobs[1] = config_get_pcie_rpss2_rp1_cfg();
        rp_knobs[2] = config_get_pcie_rpss2_rp2_cfg();
        rp_knobs[3] = config_get_pcie_rpss2_rp3_cfg();
        break;
    case RPSS3:
        *pcie_cfg_knob = config_get_pcie_rpss3_cfg();
        *phy_cfg_knob = config_get_pcie_rpss3_phy_cfg();
        rp_knobs[0] = config_get_pcie_rpss3_rp0_cfg();
        rp_knobs[1] = config_get_pcie_rpss3_rp1_cfg();
        rp_knobs[2] = config_get_pcie_rpss3_rp2_cfg();
        rp_knobs[3] = config_get_pcie_rpss3_rp3_cfg();
        break;
    case RPSS4:
        *pcie_cfg_knob = config_get_pcie_rpss4_cfg();
        *phy_cfg_knob = config_get_pcie_rpss4_phy_cfg();
        rp_knobs[0] = config_get_pcie_rpss4_rp0_cfg();
        rp_knobs[1] = config_get_pcie_rpss4_rp1_cfg();
        rp_knobs[2] = config_get_pcie_rpss4_rp2_cfg();
        rp_knobs[3] = config_get_pcie_rpss4_rp3_cfg();
        break;
    case RPSS5:
        *pcie_cfg_knob = config_get_pcie_rpss5_cfg();
        *phy_cfg_knob = config_get_pcie_rpss5_phy_cfg();
        rp_knobs[0] = config_get_pcie_rpss5_rp0_cfg();
        rp_knobs[1] = config_get_pcie_rpss5_rp1_cfg();
        rp_knobs[2] = config_get_pcie_rpss5_rp2_cfg();
        rp_knobs[3] = config_get_pcie_rpss5_rp3_cfg();
        break;
    case RPSS6:
        *pcie_cfg_knob = config_get_pcie_rpss6_cfg();
        *phy_cfg_knob = config_get_pcie_rpss6_phy_cfg();
        rp_knobs[0] = config_get_pcie_rpss6_rp0_cfg();
        rp_knobs[1] = config_get_pcie_rpss6_rp1_cfg();
        rp_knobs[2] = config_get_pcie_rpss6_rp2_cfg();
        rp_knobs[3] = config_get_pcie_rpss6_rp3_cfg();
        break;
    case RPSS7:
        *pcie_cfg_knob = config_get_pcie_rpss7_cfg();
        *phy_cfg_knob = config_get_pcie_rpss7_phy_cfg();
        rp_knobs[0] = config_get_pcie_rpss7_rp0_cfg();
        rp_knobs[1] = config_get_pcie_rpss7_rp1_cfg();
        rp_knobs[2] = config_get_pcie_rpss7_rp2_cfg();
        rp_knobs[3] = config_get_pcie_rpss7_rp3_cfg();
        break;
    default:
        /* Invalid RPSS ID */
        /* This should never happen */
        FPFW_DBGPRINT_ERROR("[PCIe Configuration] Critical error! Invalid RPSS ID: %d\n", rpss_id);
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
    pcie_cfg->rp_cfgs[0] = rp_knobs[0].silibs_rp_cfg;
    pcie_cfg->rp_cfgs[1] = rp_knobs[1].silibs_rp_cfg;
    pcie_cfg->rp_cfgs[2] = rp_knobs[2].silibs_rp_cfg;
    pcie_cfg->rp_cfgs[3] = rp_knobs[3].silibs_rp_cfg;
    pcie_cfg_workarounds->prod_rp_cfgs[0] = rp_knobs[0].prod_rp_cfg;
    pcie_cfg_workarounds->prod_rp_cfgs[1] = rp_knobs[1].prod_rp_cfg;
    pcie_cfg_workarounds->prod_rp_cfgs[2] = rp_knobs[2].prod_rp_cfg;
    pcie_cfg_workarounds->prod_rp_cfgs[3] = rp_knobs[3].prod_rp_cfg;
}

static void apply_mirrored_configurations(uint8_t rpss_id,
                                          pcie_cfg_t* pcie_cfg,
                                          pcie_prod_cfg_t* pcie_cfg_knob,
                                          pcie_prod_phy_cfg_t* phy_cfg_knob,
                                          pcie_prod_cfg_workarounds_t* pcie_cfg_workarounds)
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
    pcie_prod_rp_cfg_t rp_knobs[4];
    switch (rpss_id)
    {
    case RPSS0:
        *pcie_cfg_knob = config_get_pcie_rpss3_cfg();
        *phy_cfg_knob = config_get_pcie_rpss3_phy_cfg();
        rp_knobs[0] = config_get_pcie_rpss3_rp0_cfg();
        rp_knobs[1] = config_get_pcie_rpss3_rp1_cfg();
        rp_knobs[2] = config_get_pcie_rpss3_rp2_cfg();
        rp_knobs[3] = config_get_pcie_rpss3_rp3_cfg();
        break;
    case RPSS1:
        *pcie_cfg_knob = config_get_pcie_rpss2_cfg();
        *phy_cfg_knob = config_get_pcie_rpss2_phy_cfg();
        rp_knobs[0] = config_get_pcie_rpss2_rp0_cfg();
        rp_knobs[1] = config_get_pcie_rpss2_rp1_cfg();
        rp_knobs[2] = config_get_pcie_rpss2_rp2_cfg();
        rp_knobs[3] = config_get_pcie_rpss2_rp3_cfg();
        break;
    case RPSS2:
        *pcie_cfg_knob = config_get_pcie_rpss1_cfg();
        *phy_cfg_knob = config_get_pcie_rpss1_phy_cfg();
        rp_knobs[0] = config_get_pcie_rpss1_rp0_cfg();
        rp_knobs[1] = config_get_pcie_rpss1_rp1_cfg();
        rp_knobs[2] = config_get_pcie_rpss1_rp2_cfg();
        rp_knobs[3] = config_get_pcie_rpss1_rp3_cfg();
        break;
    case RPSS3:
        *pcie_cfg_knob = config_get_pcie_rpss0_cfg();
        *phy_cfg_knob = config_get_pcie_rpss0_phy_cfg();
        rp_knobs[0] = config_get_pcie_rpss0_rp0_cfg();
        rp_knobs[1] = config_get_pcie_rpss0_rp1_cfg();
        rp_knobs[2] = config_get_pcie_rpss0_rp2_cfg();
        rp_knobs[3] = config_get_pcie_rpss0_rp3_cfg();
        break;
    case RPSS4:
        *pcie_cfg_knob = config_get_pcie_rpss7_cfg();
        *phy_cfg_knob = config_get_pcie_rpss7_phy_cfg();
        rp_knobs[0] = config_get_pcie_rpss7_rp0_cfg();
        rp_knobs[1] = config_get_pcie_rpss7_rp1_cfg();
        rp_knobs[2] = config_get_pcie_rpss7_rp2_cfg();
        rp_knobs[3] = config_get_pcie_rpss7_rp3_cfg();
        break;
    case RPSS5:
        *pcie_cfg_knob = config_get_pcie_rpss6_cfg();
        *phy_cfg_knob = config_get_pcie_rpss6_phy_cfg();
        rp_knobs[0] = config_get_pcie_rpss6_rp0_cfg();
        rp_knobs[1] = config_get_pcie_rpss6_rp1_cfg();
        rp_knobs[2] = config_get_pcie_rpss6_rp2_cfg();
        rp_knobs[3] = config_get_pcie_rpss6_rp3_cfg();
        break;
    case RPSS6:
        *pcie_cfg_knob = config_get_pcie_rpss5_cfg();
        *phy_cfg_knob = config_get_pcie_rpss5_phy_cfg();
        rp_knobs[0] = config_get_pcie_rpss5_rp0_cfg();
        rp_knobs[1] = config_get_pcie_rpss5_rp1_cfg();
        rp_knobs[2] = config_get_pcie_rpss5_rp2_cfg();
        rp_knobs[3] = config_get_pcie_rpss5_rp3_cfg();
        break;
    case RPSS7:
        *pcie_cfg_knob = config_get_pcie_rpss4_cfg();
        *phy_cfg_knob = config_get_pcie_rpss4_phy_cfg();
        rp_knobs[0] = config_get_pcie_rpss4_rp0_cfg();
        rp_knobs[1] = config_get_pcie_rpss4_rp1_cfg();
        rp_knobs[2] = config_get_pcie_rpss4_rp2_cfg();
        rp_knobs[3] = config_get_pcie_rpss4_rp3_cfg();
        break;
    default:
        /* Invalid RPSS ID */
        /* This should never happen */
        FPFW_DBGPRINT_ERROR("[PCIe Configuration] Critical error! Invalid RPSS ID: %d\n", rpss_id);
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
    pcie_cfg->rp_cfgs[0] = rp_knobs[0].silibs_rp_cfg;
    pcie_cfg->rp_cfgs[1] = rp_knobs[1].silibs_rp_cfg;
    pcie_cfg->rp_cfgs[2] = rp_knobs[2].silibs_rp_cfg;
    pcie_cfg->rp_cfgs[3] = rp_knobs[3].silibs_rp_cfg;
    pcie_cfg_workarounds->prod_rp_cfgs[0] = rp_knobs[0].prod_rp_cfg;
    pcie_cfg_workarounds->prod_rp_cfgs[1] = rp_knobs[1].prod_rp_cfg;
    pcie_cfg_workarounds->prod_rp_cfgs[2] = rp_knobs[2].prod_rp_cfg;
    pcie_cfg_workarounds->prod_rp_cfgs[3] = rp_knobs[3].prod_rp_cfg;
}

void override_default_pcie_cfg(uint8_t rpss_id)
{
    pcie_cfg_t* pcie_cfg = &pcie_cfg_np[rpss_id];
    pcie_prod_cfg_t pcie_cfg_knob;
    pcie_prod_phy_cfg_t phy_cfg_knob;
    pcie_prod_cfg_workarounds_t* pcie_cfg_workarounds = &pcie_cfg_workarounds_np[rpss_id];

    if (mirror_rpss_configurations() == true)
    {
        apply_mirrored_configurations(rpss_id, pcie_cfg, &pcie_cfg_knob, &phy_cfg_knob, pcie_cfg_workarounds);
    }
    else
    {
        apply_one_to_one_configurations(rpss_id, pcie_cfg, &pcie_cfg_knob, &phy_cfg_knob, pcie_cfg_workarounds);
    }

    pcie_cfg->pcie_ss_override = false; /* Always disable so that individual RPSS settings are used */
    pcie_cfg->pcie_ss_en = pcie_cfg_knob.pcie_ss_en;
    pcie_cfg->pcie_bifurcation_mode = pcie_cfg_knob.pcie_bifurcation_mode;
    pcie_cfg->pcie_system_counter_delay = pcie_cfg_knob.pcie_system_counter_delay;

    /* pcie_cfg->pcie_cxl_support set below */
    pcie_cfg->pcie_cxl_sync_header_bypass = pcie_cfg_knob.pcie_cxl_sync_header_bypass;

    /* Apply PHY knobs per lane*/
    for (uint8_t i = 0; i < PCIE_LANE_COUNT; i++)
    {
        pcie_cfg->phy_lane_cfgs[i] = phy_cfg_knob.phy_lane_cfgs[i];
    }

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
        rb_configs[i].flags.is_slot = pcie_cfg->rp_cfgs[i].pcie_rp_is_slot;
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
        FPFW_DBGPRINT_ERROR("[PCIe Configuration] Critical error! Invalid RPSS ID: %d\n", rpss_id);
        FPFW_RUNTIME_ASSERT(false);
    }

    pcie_cfg = &pcie_cfg_np[rpss_id];

    return pcie_cfg;
}

pcie_prod_cfg_workarounds_t* get_workaround_for_rpss(uint8_t rpss_id)
{
    if (rpss_id >= NUM_RPSS)
    {
        /* Invalid RPSS ID */
        /* This should never happen */
        FPFW_DBGPRINT_ERROR("[PCIe Workaround] Critical error! Invalid RPSS ID: %d\n", rpss_id);
        FPFW_RUNTIME_ASSERT(false);
    }

    return &pcie_cfg_workarounds_np[rpss_id];
}
