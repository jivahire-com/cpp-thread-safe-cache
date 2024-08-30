//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ap_core_ppu.c
 * Implements the APcore PPU interface.
 */

/*------------- Includes -----------------*/
#include "ap_core_i.h"

#include <FpFwAssert.h>
#include <bug_check.h>
#include <core_cluster_with_pvt_regs.h> // for CORE_CLUSTER_WITH_PVT_VOYAGER_DSU_CLUSTER_ADDRESS
#include <corebits.h>
#include <kng_error.h>
#include <ppu_v1.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <utils.h>
#include <voyager_dsu_cluster_regs.h> // for VOYAGER_DSU_CLUSTER_CLUSTER_PPU_ADDRESS, ...

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

// TODO : Once below task completed and how it implemented in the code, we will finalize ppu design.
// for now, prevent system hang case by adding timeout to ppu_v1_set_power_mode_with_timeout.
// https://dev.azure.com/ms-tsd/Base_IP/_workitems/edit/781336

// function to initialize the PPU for all cores and clusters
void ap_core_ppu_init(ap_core_service_context_t* p_context)
{
    FPFW_RUNTIME_ASSERT(p_context != NULL);
    for (unsigned int core_idx = 0; core_idx < p_context->p_config->platform_die_core_count; ++core_idx)
    {
        // only initialize cores that are enabled (present/fused)
        if (!corebits_is_bit_set(&p_context->enabled_cores, core_idx))
        {
            continue;
        }
        uintptr_t cluster_ppu_addr =
            (p_context->p_config->cluster_pex_base + (core_idx * p_context->p_config->cluster_stride) +
             CORE_CLUSTER_WITH_PVT_VOYAGER_DSU_CLUSTER_ADDRESS + VOYAGER_DSU_CLUSTER_CLUSTER_PPU_ADDRESS);
        uintptr_t core_ppu_addr =
            (p_context->p_config->cluster_pex_base + (core_idx * p_context->p_config->cluster_stride) +
             CORE_CLUSTER_WITH_PVT_VOYAGER_DSU_CLUSTER_ADDRESS + VOYAGER_DSU_CLUSTER_CORE0_PPU_ADDRESS);

        ppu_v1_init(cluster_ppu_addr);
        ppu_v1_init(core_ppu_addr);
    }
}

static int32_t ppu_v1_set_power_mode_with_timeout(uintptr_t ppu_v1_base_addr, PPU_V1_MODE ppu_mode, PPU_V1_OPMODE op_policy, uint32_t timeout_ms)
{
    int32_t status = KNG_SUCCESS;

    if (timeout_ms == 0)
    {
        ppu_v1_set_power_mode(ppu_v1_base_addr, ppu_mode, op_policy);
    }
    else
    {
        ppu_v1_request_power_mode(ppu_v1_base_addr, ppu_mode, op_policy);

        uint32_t elapsed_time = 0;
        uint32_t interval = 3;

        while (ppu_v1_get_power_mode(ppu_v1_base_addr) != ppu_mode)
        {
            if (elapsed_time >= timeout_ms)
            {
                status = KNG_E_TIMEOUT;
                break;
            }

            sleep_ms(interval);
            elapsed_time += interval;
        }
    }

    return status;
}

// function to set the power state of a single cluster
static void cluster_set_power_state(ap_core_service_context_t* p_context, unsigned cluster_idx, bool power_state_on, uint32_t timeout_ms)
{
    uintptr_t cluster_ppu_addr =
        (p_context->p_config->cluster_pex_base + (cluster_idx * p_context->p_config->cluster_stride) +
         CORE_CLUSTER_WITH_PVT_VOYAGER_DSU_CLUSTER_ADDRESS + VOYAGER_DSU_CLUSTER_CLUSTER_PPU_ADDRESS);

    if (power_state_on)
    {
        // power cluster on
        int32_t status =
            ppu_v1_set_power_mode_with_timeout(cluster_ppu_addr, PPU_V1_MODE_ON, PPU_V1_OPMODE_00, timeout_ms);

        if (KNG_FAILED(status))
        {
            BUG_CHECK(status, cluster_ppu_addr, power_state_on);
        }

        // Dynamic enable cluster PPU to support dynamic INTCLK gating
        ppu_dynamic_enable(cluster_ppu_addr, PPU_V1_MODE_OFF);
    }
    else
    {
        // power cluster off
        // determine proper cluster off sequence
        FPFW_RUNTIME_ASSERT(false);
    }
}

// function to turn all ppu clusters on
void ap_core_ppu_clusters_on(ap_core_service_context_t* p_context, uint32_t timeout_ms)
{
    FPFW_RUNTIME_ASSERT(p_context != NULL);

    for (unsigned int core_idx = 0; core_idx < p_context->p_config->platform_die_core_count; ++core_idx)
    {
        // only initialize cores that are enabled (present/fused)
        if (!corebits_is_bit_set(&p_context->enabled_cores, core_idx))
        {
            continue;
        }

        cluster_set_power_state(p_context, core_idx, true, timeout_ms);
    }
}

// function to set the core power state of a single core
void ap_core_ppu_core_set_power_state(ap_core_service_context_t* p_context, unsigned core_idx, bool power_state_on, uint32_t timeout_ms)
{
    FPFW_RUNTIME_ASSERT(p_context != NULL);
    // only act on cores that are enabled (present/fused)
    if (!corebits_is_bit_set(&p_context->enabled_cores, core_idx))
    {
        // unexpected core index
        // TODO: should likely return error to dfwk client (SCMI)
        //       https://azurecsi.visualstudio.com/Dev/_workitems/edit/1877178
        FPFW_RUNTIME_ASSERT(false);
    }

    uintptr_t core_ppu_addr =
        (p_context->p_config->cluster_pex_base + (core_idx * p_context->p_config->cluster_stride) +
         CORE_CLUSTER_WITH_PVT_VOYAGER_DSU_CLUSTER_ADDRESS + VOYAGER_DSU_CLUSTER_CORE0_PPU_ADDRESS);

    if (power_state_on)
    {
        // power core on
        int status = ppu_v1_set_power_mode_with_timeout(core_ppu_addr, PPU_V1_MODE_ON, PPU_V1_OPMODE_00, timeout_ms);

        if (KNG_FAILED(status))
        {
            BUG_CHECK(status, core_ppu_addr, power_state_on);
        }

        // Dynamic enable core PPU to support dynamic C state entry
        ppu_dynamic_enable(core_ppu_addr, PPU_V1_MODE_OFF);
    }
    else
    {
        // power core off
        // TODO : determine proper core off sequence
        // https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1824038/
    
        APCORE_LOG_INFO("AP Core Power off Requested, For now NOP to unblock SBSA testing");
    }
}