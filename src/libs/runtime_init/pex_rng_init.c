//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pex_rng_init.c
 * Config/initialization for the PEX RNG IP
 */


/*------------- Includes -----------------*/

#include <DbgPrint.h>
#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>  
#include <atu_api.h>               // for MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_BASE_ADDR
#include <atu_lib.h>               // for atu_map_entry_t, atu_entry_attr_t
#include <core_cluster_top_regs.h> // for CORE_CLUSTER_TOP_CORE_CLUSTER0_AD...
#include <corebits.h>
#include <fpfw_init.h>          // for fpfw_init_get_handle, FPFW_INIT_S...
#include <idsw_kng.h>        
#include <kng_soc_constants.h>  // for NUM_AP_CORES_PER_DIE
#include <pex_rng.h>
#include <platform_core_config.h>
#define  __NO_CSR_TYPEDEFS__
#include <scp_top_regs.h>
#undef   __NO_CSR_TYPEDEFS__

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(pex_rng, FPFW_INIT_DEPENDENCIES("dfwk", "sysinfo", "mesh", "atu_svc"))
{
    pex_rng_config_t rng_config = {
        .cluster_pex_base = MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_BASE_ADDR,
        .platform_cores_in_die = &platform_cores,
        .core_count = NUM_AP_CORES_PER_DIE,
        .cluster_stride = (CORE_CLUSTER_TOP_CORE_CLUSTER1_ADDRESS - CORE_CLUSTER_TOP_CORE_CLUSTER0_ADDRESS),
    };
    
    switch (idsw_get_platform_sdv())
    {
    case PLATFORM_SVP_SIM:
        rng_config.platform_cores_in_die = &svp_cores;
        break;
    case PLATFORM_SVP_MIN_CONFIG_SIM:
        rng_config.platform_cores_in_die = &svp_min_config_cores;
        break;
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        rng_config.platform_cores_in_die = &fpga_platform_cores;   
        break;
    case PLATFORM_EMU:
    case PLATFORM_EMU_1D:
    case PLATFORM_EMU_2D:
        rng_config.platform_cores_in_die = &platform_cores;   
        break;
    case PLATFORM_EMU_1D_8C:
    case PLATFORM_EMU_2D_8C:
        rng_config.platform_cores_in_die = &zebu_cores_8C_model;  
        break; 
    default:
        break;
    }

    FPFW_DBGPRINT_INFO("RNG init start\n");
    init_pex_rng(&rng_config);
    FPFW_DBGPRINT_INFO("RNG init done\n");
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};

}