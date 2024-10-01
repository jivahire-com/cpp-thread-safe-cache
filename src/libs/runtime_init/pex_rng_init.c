//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pex_rng_init.c
 * Config/initialization for the PEX RNG IP
 */


/*------------- Includes -----------------*/

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
#include <silibs_ap_top_regs.h> // for AP_TOP_D0_CORE_CLUSTER_SIZE, AP_T...
#include <stdint.h> // for uint32_t
#define  __NO_CSR_TYPEDEFS__
#include <scp_top_regs.h>
#undef   __NO_CSR_TYPEDEFS__

#define SVP_NUM_CORES_PER_DIE 4
#define FPGA_NUM_CORES_PER_DIE 8

static const corebits_t fpga_platform_cores = (corebits_t)COREBITS_INIT_UINT32(0x000c0300, 0x00c03000, 0);
static const corebits_t platform_cores = (corebits_t)COREBITS_INIT_UINT32(0xFFFFFFFF, 0xFFFFFFFF, 0xF);

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
        rng_config.core_count = SVP_NUM_CORES_PER_DIE;
        break;
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        rng_config.platform_cores_in_die = &fpga_platform_cores;   
        break;
    default:
        break;
    }

    init_pex_rng(&rng_config);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};

}