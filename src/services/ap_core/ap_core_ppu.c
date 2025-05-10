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
#include <cluster_ppu_regs.h>
#include <core_cluster_with_pvt_regs.h> // for CORE_CLUSTER_WITH_PVT_VOYAGER_DSU_CLUSTER_ADDRESS
#include <corebits.h>
#include <idhw.h>
#include <kng_error.h>
#include <pik_clock_lib.h>
#include <ppu_v1.h>
#include <silibs_platform.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <utils.h>
#include <voyager_dsu_cluster_regs.h> // for VOYAGER_DSU_CLUSTER_CLUSTER_PPU_ADDRESS, ...

/*-- Symbolic Constant Macros (defines) --*/

#define PPU_V1_PWCR_DEV_REQ_EN    UINT32_C(0x000000FF)
#define PPU_V1_PWCR_DEV_ACTIVE_EN UINT32_C(0x0007FF00)
#define PPU_V1_PWPR_DYNAMIC_EN    UINT32_C(0x00000100)

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

// function to set the power state of a single cluster
static void cluster_set_power_state(ap_core_service_context_t* p_context, unsigned cluster_idx, bool power_state_on, uint32_t timeout_ms)
{
    // TODO: timeout_ms is now being interpreted as us - Need to update and also provide an appropriate timeout in us
    uintptr_t cluster_ppu_addr =
        (p_context->p_config->cluster_pex_base + (cluster_idx * p_context->p_config->cluster_stride) +
         CORE_CLUSTER_WITH_PVT_VOYAGER_DSU_CLUSTER_ADDRESS + VOYAGER_DSU_CLUSTER_CLUSTER_PPU_ADDRESS);

    if (power_state_on)
    {
        // Configure cluster clocks
        for (int clk_dev_id = PIK_DEV_ID_CLUS_CORECLK; clk_dev_id <= PIK_DEV_ID_CLUS_PPUCLK; clk_dev_id++)
        {
            /* Turn on cluster clocks to desired frequency */
            int sts = pik_clock_power_transition(PIK_CLUS_PIK_DEV_ID(cluster_idx, clk_dev_id), MOD_PD_STATE_ON);
            FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);
        }

        // power cluster on

        int32_t status =
            ppu_v1_set_power_mode_with_timeout(cluster_ppu_addr, PPU_V1_MODE_ON, PPU_V1_OPMODE_00, timeout_ms);

        if (KNG_FAILED(status))
        {
            BUG_CHECK(status, cluster_ppu_addr, power_state_on);
        }

        // Dynamic enable cluster PPU to support dynamic INTCLK gating
        // if SVP, use PPU_V1_MODE_ON due to an ARM model error
        if (IS_PLATFORM_SVP())
        {
            ppu_dynamic_enable(cluster_ppu_addr, PPU_V1_MODE_ON);
        }
        else
        {
            ppu_dynamic_enable(cluster_ppu_addr, PPU_V1_MODE_OFF);
        }
    }
    else
    {
        // TO DO : Support disable lock from Off state.
        // https://dev.azure.com/ms-tsd/Base_IP/_workitems/edit/785784

        int status = ppu_v1_set_power_mode_with_timeout(cluster_ppu_addr, PPU_V1_MODE_OFF, PPU_V1_OPMODE_00, timeout_ms);

        if (KNG_FAILED(status))
        {
            BUG_CHECK(status, cluster_ppu_addr, power_state_on);
        }
    }
}

// function to turn all ppu clusters on
void ap_core_ppu_clusters_on(ap_core_service_context_t* p_context, uint32_t timeout_ms)
{
    FPFW_RUNTIME_ASSERT(p_context != NULL);

    for (unsigned int core_idx = 0; core_idx < p_context->p_config->platform_die_core_count; ++core_idx)
    {
        // only initialize cores that are enabled (present/fused)
        if (corebits_is_bit_set(&p_context->enabled_cores, core_idx))
        {
            cluster_set_power_state(p_context, core_idx, true, timeout_ms);
        }
    }
}

// function to turn all ppu clusters on if needed
void ap_core_ppu_clusters_on_if_needed(ap_core_service_context_t* p_context, uint32_t timeout_ms)
{
    FPFW_RUNTIME_ASSERT(p_context != NULL);

    for (unsigned int core_idx = 0; core_idx < p_context->p_config->platform_die_core_count; ++core_idx)
    {
        // only initialize cores that are enabled (present/fused)
        if (corebits_is_bit_set(&p_context->enabled_cores, core_idx))
        {
            uintptr_t cluster_ppu_addr =
                (p_context->p_config->cluster_pex_base + (core_idx * p_context->p_config->cluster_stride) +
                 CORE_CLUSTER_WITH_PVT_VOYAGER_DSU_CLUSTER_ADDRESS + VOYAGER_DSU_CLUSTER_CLUSTER_PPU_ADDRESS);

            if (ppu_v1_get_power_mode(cluster_ppu_addr) != PPU_V1_MODE_ON)
            {
                cluster_set_power_state(p_context, core_idx, true, timeout_ms);
            }
        }
    }
}

// function to turn all ppu clusters off
void ap_core_ppu_clusters_off(ap_core_service_context_t* p_context, uint32_t timeout_ms)
{
    FPFW_RUNTIME_ASSERT(p_context != NULL);

    for (unsigned int core_idx = 0; core_idx < p_context->p_config->platform_die_core_count; ++core_idx)
    {
        // only initialize cores that are enabled (present/fused)
        if (corebits_is_bit_set(&p_context->enabled_cores, core_idx))
        {
            cluster_set_power_state(p_context, core_idx, false, timeout_ms);
        }
    }
}

// function to turn all core off
void ap_core_ppu_cores_off(ap_core_service_context_t* p_context, uint32_t timeout_ms)
{
    FPFW_RUNTIME_ASSERT(p_context != NULL);

    for (unsigned int core_idx = 0; core_idx < p_context->p_config->platform_die_core_count; ++core_idx)
    {
        // only initialize cores that are enabled (present/fused)
        if (corebits_is_bit_set(&p_context->enabled_cores, core_idx))
        {
            ap_core_ppu_core_set_power_state(p_context, core_idx, false, timeout_ms);
        }
    }
}

// function to set the core power state of a single core
void ap_core_ppu_core_set_power_state(ap_core_service_context_t* p_context, unsigned core_idx, bool power_state_on, uint32_t timeout_ms)
{
    FPFW_RUNTIME_ASSERT(p_context != NULL);
    // only act on cores that are enabled (present/fused)
    if (!corebits_is_bit_set(&p_context->enabled_cores, core_idx))
    {
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
        // TO DO : Support disable lock from Off state.
        // https://dev.azure.com/ms-tsd/Base_IP/_workitems/edit/785784

        int status = ppu_v1_set_power_mode_with_timeout(core_ppu_addr, PPU_V1_MODE_OFF, PPU_V1_OPMODE_00, timeout_ms);

        if (KNG_FAILED(status))
        {
            BUG_CHECK(status, core_ppu_addr, power_state_on);
        }
    }
}

// future silibs API for disabling PPU handshaking
void ppu_v1_disable_handshake(uintptr_t ppu_v1_base_addr)
{
    vptr_cluster_ppu_reg ppu = (vptr_cluster_ppu_reg)ppu_v1_base_addr;

    // Disable dynamic transitions (per TRM: needed before disabling handshaking)
    ppu->ppu_pwpr.as_uint32 = PPU_V1_MODE_ON;

    // Disable PPU handshaking
    ppu->ppu_pwcr.as_uint32 &= ~(PPU_V1_PWCR_DEV_REQ_EN | PPU_V1_PWCR_DEV_ACTIVE_EN);
}

// function to disable PPU handshaking
void ap_core_ppu_disable_handshaking(ap_core_service_context_t* p_context)
{
    FPFW_RUNTIME_ASSERT(p_context != NULL);

    for (unsigned int core_idx = 0; core_idx < p_context->p_config->platform_die_core_count; ++core_idx)
    {
        // only initialize cores that are enabled (present/fused)
        if (corebits_is_bit_set(&p_context->enabled_cores, core_idx))
        {
            uintptr_t cluster_ppu_addr =
                (p_context->p_config->cluster_pex_base + (core_idx * p_context->p_config->cluster_stride) +
                 CORE_CLUSTER_WITH_PVT_VOYAGER_DSU_CLUSTER_ADDRESS + VOYAGER_DSU_CLUSTER_CLUSTER_PPU_ADDRESS);
            uintptr_t core_ppu_addr =
                (p_context->p_config->cluster_pex_base + (core_idx * p_context->p_config->cluster_stride) +
                 CORE_CLUSTER_WITH_PVT_VOYAGER_DSU_CLUSTER_ADDRESS + VOYAGER_DSU_CLUSTER_CORE0_PPU_ADDRESS);

            ppu_v1_disable_handshake(core_ppu_addr);
            ppu_v1_disable_handshake(cluster_ppu_addr);
        }
    }
}
