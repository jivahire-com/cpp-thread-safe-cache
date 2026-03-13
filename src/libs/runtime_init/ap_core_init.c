//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ap_core_init.c
 * This file contains the config/initialization for the APcore service
 */

/*------------- Includes -----------------*/

#include <FpFwUtils.h> // for knowing how big 1k is
#include <ap_core_init.h>
#include <atu_api.h> // for MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_BASE_ADDR
#include <atu_lib.h> // for atu_map_entry_t, atu_entry_attr_t
#include <boot_status.h>
#include <bug_check.h>
#include <core_cluster_top_regs.h> // for CORE_CLUSTER_TOP_CORE_CLUSTER0_AD...
#include <core_info.h>
#include <corebits.h>
#include <fpfw_init.h>          // for fpfw_init_get_handle, FPFW_INIT_S...
#include <idsw.h>               // for idsw_get_die_id
#include <idsw_kng.h>           // for NUM_DIE
#include <silibs_ap_top_regs.h> // for AP_TOP_D0_CORE_CLUSTER_SIZE, AP_T...
#include <startup_shutdown.h>
#include <stdint.h> // for uint32_t
#include <system_info.h>
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/
#define DEFAULT_BOOT_CORE_RVBARADDR (0x00010000ull)

/*-------------- Functions ---------------*/
static void setup_ap_loop_to_self(uint64_t rvbaraddr)
{
#define ARM64_LOOP 0x14000000
    // setup loop to self
    // find SCP address for the rvbar
    uint32_t translated_rvbar;
    // expect that there is a global translation for rvbar
    BUG_ASSERT(!atu_translate_address(ATU_ID_MSCP, rvbaraddr, &translated_rvbar));
    // write a loop to self instruction at the entry point to enable side-load of FW when HSP not present, etc
    *(volatile uint32_t*)translated_rvbar = ARM64_LOOP;
}

PLACED_CODE FPFW_INIT_COMPONENT(
    ap_core_svc,
    FPFW_INIT_DEPENDENCIES("dfwk", "mesh_stg_2", "icc_hspmbx", "atu_svc", "sysinfo", "pex_rng", "core_info", "hm_post_init", "ift"))
{
    // only call idsw_get_platform_sdv() once
    KNG_PLAT_ID platform_sdv = idsw_get_platform_sdv();
    corebits_t* sys_cores_in_die1 = core_info_get_enable_cores_result();
    static ap_core_service_t ap_core_service;
    static ap_core_service_config_t ap_core_config = {
        .boot_core_rvbaraddr = DEFAULT_BOOT_CORE_RVBARADDR,
        .cluster_stride = (CORE_CLUSTER_TOP_CORE_CLUSTER1_ADDRESS - CORE_CLUSTER_TOP_CORE_CLUSTER0_ADDRESS),
    };

    // platform defaults
    ap_core_config.cluster_pex_base = MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_BASE_ADDR;
    ap_core_config.platform_cores_in_die = sys_cores_in_die1;
    ap_core_config.platform_die_core_count = NUM_AP_CORES_PER_DIE;
    ap_core_config.primary_boot_die = (idsw_get_die_id() == DIE_0);

    // platform overrides — only one call to idsw_get_platform_sdv above
    if ((platform_sdv == PLATFORM_FPGA_LARGE_RVP) || (platform_sdv == PLATFORM_FPGA_LARGE))
    {
        if (ap_core_config.primary_boot_die)
        {
            setup_ap_loop_to_self(ap_core_config.boot_core_rvbaraddr);
        }
    }

    // finally hook up the service
    fpfw_icc_base_ctx_t* icc_hspmbx_ctx = fpfw_init_get_handle("icc_hspmbx");
    ap_core_init(&ap_core_service, fpfw_init_get_handle("dfwk"), icc_hspmbx_ctx, &ap_core_config);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &ap_core_service};
}

PLACED_CODE FPFW_INIT_COMPONENT(ap_core_int, FPFW_INIT_DEPENDENCIES("ap_core_svc", "sos_int", "mesh_stg_2", "boot_stat"))
{
    static ap_core_interface_t ap_core_interface;
    ap_core_interface_init(fpfw_init_get_handle("ap_core_svc"), &ap_core_interface);

    /*========= Begin code for SSI registration =========*/
    // create an interface specifically for SSI and register it
    static ap_core_interface_t ap_core_ssi_interface;
    ap_core_interface_init(fpfw_init_get_handle("ap_core_svc"), &ap_core_ssi_interface);
    // static data for SSI registration
    static startup_ssi_registration_t ssi_registration = {
        .registration_id = SOS_SSI_ID_AP_CORE,
    };
    int32_t status =
        sos_register_ssi(fpfw_init_get_handle("sos_int"), &ssi_registration, &ap_core_ssi_interface.header);
    BUG_ASSERT_PARAM(status == FPFW_INIT_STATUS_SUCCESS, status, 0);
    /*=========== End code for SSI registration ==========*/

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_APCORE_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_APCORE_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &ap_core_interface};
}
