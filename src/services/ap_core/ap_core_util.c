//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ap_core_util.c
 * Implements the APcore utility functions.
 */

/*------------- Includes -----------------*/
#include "ap_core_i.h"

#include <FpFwAssert.h>
#include <core_cluster_with_pvt_regs.h> // for CORE_CLUSTER_WITH_PVT_VOYAGER_DSU_CLUSTER_ADDRESS
#include <core_manager_and_clock_control_registers_regs.h>
#include <corebits.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

// utility function to find the first core to boot
unsigned int ap_core_util_boot_core(ap_core_service_context_t* p_context)
{
    // TODO: proper implementation
    //       https://azurecsi.visualstudio.com/Dev/_workitems/edit/1877167
    FPFW_RUNTIME_ASSERT(p_context != NULL);
    // for now, just return the first core that is enabled
    for (unsigned int core_idx = 0; core_idx < p_context->p_config->platform_die_core_count; ++core_idx)
    {
        if (corebits_is_bit_set(&p_context->enabled_cores, core_idx))
        {
            return core_idx;
        }
    }
    return 0;
}

// utility function to set the RVBARADDR for a single core
void ap_core_util_set_rvbaraddr(ap_core_service_context_t* p_context, unsigned core_idx, uint64_t rvbaraddr)
{
    FPFW_RUNTIME_ASSERT(p_context != NULL);
    vptr_core_manager_and_clock_control_registers_reg vp_cm_ccr_reg =
        (vptr_core_manager_and_clock_control_registers_reg)(p_context->p_config->cluster_pex_base +
                                                            (core_idx * p_context->p_config->cluster_stride) +
                                                            CORE_CLUSTER_WITH_PVT_CORE_MANAGER_ADDRESS);
    // set the RVBARADDR
    vp_cm_ccr_reg->pe_rvbaraddr_lw.as_uint32 = (uint32_t)rvbaraddr;
    vp_cm_ccr_reg->pe_rvbaraddr_up.as_uint32 = (uint32_t)(rvbaraddr >> 32);
}

// utility function to set the RVBARADDR for all enabled cores
void ap_core_util_set_all_rvbaraddr(ap_core_service_context_t* p_context, uint64_t rvbaraddr)
{
    FPFW_RUNTIME_ASSERT(p_context != NULL);
    for (unsigned int core_idx = 0; core_idx < p_context->p_config->platform_die_core_count; ++core_idx)
    {
        if (!corebits_is_bit_set(&p_context->enabled_cores, core_idx))
        {
            continue;
        }
        ap_core_util_set_rvbaraddr(p_context, core_idx, rvbaraddr);
    }
}

// utility function to get a bitfield of enabled cores based on fuse disables
void ap_core_util_get_fuse_enabled_cores(corebits_t* enabled_cores)
{
    // TODO: integrate with fuse service
    //       https://azurecsi.visualstudio.com/Dev/_workitems/edit/1877171
    FPFW_RUNTIME_ASSERT(enabled_cores != NULL);
    // get the enabled cores from the fuses
    // temporary until we can read real fuses
    *enabled_cores = (corebits_t)COREBITS_INIT_UINT32(0xFFFFFFFF, 0xFFFFFFFF, 0xF);
}