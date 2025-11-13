//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pex_rng_init.c
 * Config/initialization for the PEX RNG IP
 */

/*------------- Includes -----------------*/

#include <DbgPrint.h>
#include <FpFwUtils.h>
#include <atu_api.h>               // for MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_BASE_ADDR
#include <atu_lib.h>               // for atu_map_entry_t, atu_entry_attr_t
#include <core_cluster_top_regs.h> // for CORE_CLUSTER_TOP_CORE_CLUSTER0_AD...
#include <core_info.h>
#include <corebits.h>
#include <fpfw_init.h> // for fpfw_init_get_handle, FPFW_INIT_S...
#include <idsw_kng.h>
#include <ift_fw.h>
#include <pex_rng.h>
#define __NO_CSR_TYPEDEFS__
#include <scp_top_regs.h>
#undef __NO_CSR_TYPEDEFS__
#include <system_info.h>

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(pex_rng, FPFW_INIT_DEPENDENCIES("dfwk", "sysinfo", "mesh_stg_2", "atu_svc", "core_info"))
{
    if (ift_is_enabled())
    {
        FPFW_DBGPRINT_INFO("IFT enabled. Skip PEX RNG\n");
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    corebits_t* sys_cores_in_die1 = core_info_get_enable_cores_result();
    static pex_rng_config_t rng_config;

    rng_config.cluster_pex_base = MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_BASE_ADDR;
    rng_config.platform_cores_in_die = sys_cores_in_die1;
    rng_config.core_count = NUM_AP_CORES_PER_DIE;
    rng_config.cluster_stride = (CORE_CLUSTER_TOP_CORE_CLUSTER1_ADDRESS - CORE_CLUSTER_TOP_CORE_CLUSTER0_ADDRESS);

    if (!system_info_is_warm_start())
    {
        FPFW_DBGPRINT_INFO("RNG init start\n");
        init_pex_rng(&rng_config);
        FPFW_DBGPRINT_INFO("RNG init done\n");
    }
    else
    {
        FPFW_DBGPRINT_INFO("RNG init skipping on warm start\n");
    }
    register_pex_error_domain(&rng_config);
    FPFW_DBGPRINT_INFO("PEX error domain registration done\n");

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &rng_config};
}
