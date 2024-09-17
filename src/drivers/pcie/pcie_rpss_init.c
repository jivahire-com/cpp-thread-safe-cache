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
#include <idsw.h>
#include <idsw_kng.h>
#include <kng_atu_mappings.h>
#include <kng_soc_constants.h>
#include <pcie_dfwk.h>
#include <pcie_knobs.h>
#include <pcie_platform_overrides_i.h>
#include <pcie_ss_common.h>
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
static void populate_rb_configs_from_rpss_entity(pcie_ss_entity_t* rpss, pcie_root_bridge_config* rb_configs)
{
    for (uint8_t i = 0; i < PCIESS_NUM_PORTS; i++)
    {
        if (rpss->rps[i].valid == false || rpss->rps[i].enabled == false)
        {
            rb_configs[i].flags.is_enabled = false;
            continue;
        }

        rb_configs[i].flags.is_enabled = rpss->rps[i].enabled;
        rb_configs[i].flags.hot_plug_enabled = pcie_cfg_np.rp_cfgs[i].pcie_rp_hotplug_capable;
        rb_configs[i].slot_number = pcie_cfg_np.rp_cfgs[i].pcie_rp_slot_num;

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

    sts = pciess_config_entity(rpss, resolved_subsystem_config_addr, ap_subsystem_config_addr, &pcie_cfg_np, true, true);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    /* Override settings based on the platform we are running on */
    plat_overrides_pre_pciess_config_ss_for_bifur(rpss);

    // TODO: this API fails on SVP, due to ras APIs failing
    // Renable once silibs skips RAS init on SVP
    // ADO: https://dev.azure.com/ms-tsd/Kingsgate/_workitems/edit/797391/
    sts = pciess_config_ss_for_bifur(rpss);
    if (idsw_get_platform_sdv() != PLATFORM_SVP_SIM)
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

    if (!(IS_PLATFORM_FPGA()))
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