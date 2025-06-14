//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file pex_rng.c
 *  This modules initializes and manages the RNG IP
 */

/*------------- Includes -----------------*/
#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <corebits.h>
#include <idsw.h>              // for idsw_get_platform_sdv,
#include <idsw_kng.h>          // for PLATFORM_FPGA_LARGE
#include <kng_soc_constants.h> // for NUM_DIE
#include <pex_rng.h>
#define __NO_CSR_TYPEDEFS__
#include <scp_top_regs.h>
#undef __NO_CSR_TYPEDEFS__
#include <core_cluster_top_regs.h>
#include <pex_regs.h>
#include <rng.h>
#include <stdio.h>

/*------------- Functions ----------------*/

void init_pex_rng(pex_rng_config_t* rng_config)
{
    FPFW_RUNTIME_ASSERT(rng_config != NULL);
    for (unsigned int core = 0; core < rng_config->core_count; ++core)
    {
        const corebits_t* enabled_cores = rng_config->platform_cores_in_die;
        if (!corebits_is_bit_set(enabled_cores, core))
        {
            continue;
        }
        printf("Enabling RNG for core %d\n", core);

        const uintptr_t cluster_pex_base_addr = (rng_config->cluster_pex_base + (rng_config->cluster_stride * core));
        uint32_t ap_rng_base = cluster_pex_base_addr + PEX_RNG_ADDRESS;

        rng_enable_r(ap_rng_base, 1);
    }
}

void reset_pex_rng(uintptr_t ap_rng_base)
{
    rng_disable_r(ap_rng_base);
    rng_enable_r(ap_rng_base, 1);
}

// Adding dummy implementations for static declarations in rng.h
// TODO: Change the static declarations in rng.h
// ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1917157/?workitem=2063037

static void rng_write_ctrl(uint32_t val)
{
    /* dummy function, do nothing */
    FPFW_UNUSED(val);
}

static uint32_t rng_read_ctrl()
{
    /* dummy function, do nothing */
    return 0;
}

void unused_functions()
{
    FPFW_UNUSED(rng_write_ctrl);
    FPFW_UNUSED(rng_read_ctrl);
}