//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_bwl.c
 * This file contains the implementation for the DDR Manager BWL (bandwidth limiter) APIs
 *
 */

/*------------- Includes -----------------*/
#include "ddr_manager_bwl.h"

#include "ddr_manager_i.h"

#include <ddr_manager_events.h>
#include <fpfw_cfg_mgr.h>
#include <gtimer_prodfw.h>
#include <idsw_kng.h>
#include <sensor_fifo_service.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static dimm_throttle_source_t bwl_state = DIMM_THROTTLE_SOURCE_NONE;
static bool s_bwlEngaged = false;

static const bool START = true;
static const bool STOP = false;

/*------------- Functions ----------------*/
static void ddr_manager_control_bwl(int action)
{
    uint32_t DDR_BWL_MAX_ACC_COST = config_get_ddrmanager_bwl_max_acc_cost(); // Maximum accumulated cost value
    uint32_t DDR_BWL_RD_WR_COST = config_get_ddrmanager_bwl_rd_wr_cost(); // Cost of issuing a read or write to the media

    // Get Die ID
    KNG_DIE_ID die_id = idsw_get_die_id();
    int mc_start = die_id == DIE_0 ? 0 : DDRSS_MAX_SS_NUM;

    if (action == START)
    {
        // Engage the DDR BWL
        if (s_bwlEngaged)
        {
            return;
        }

        // TODO: add config knob to throttle single DIMM or all DIMMs on local die
        for (int mc = mc_start; mc < (mc_start + DDRSS_MAX_SS_NUM); mc++)
        {
            // Configure the bandwidth limiter for this MC
            int sts = ddrss_bandwidth_limiter_config(mc, START, DDR_BWL_MAX_ACC_COST, DDR_BWL_RD_WR_COST);
            if (sts != SILIBS_SUCCESS)
            {
                printf("Failed to configure BWL for MC %d: %d\n", mc, sts);
            }
        }

        // Set the MEMHOT GPIO to indicate to the platform that memory is hot and throttling is active
        ddr_manager_set_memhot_gpio();
        s_bwlEngaged = true;

        // Record the time of engagement for residency tracking & increment throttle count
        ddr_telemetry_increment_throttle_count();
        set_last_gtimer_count(gtimer_prodfw_get_counter());

        printf("DDR BWL+\n");
    }
    else if (action == STOP)
    {
        // Disengage the DDR BWL
        if (!s_bwlEngaged)
        {
            return;
        }

        for (int mc = mc_start; mc < (mc_start + DDRSS_MAX_SS_NUM); mc++)
        {
            // Configure the bandwidth limiter for this MC
            int sts = ddrss_bandwidth_limiter_config(mc, STOP, DDR_BWL_MAX_ACC_COST, DDR_BWL_RD_WR_COST);
            if (sts != SILIBS_SUCCESS)
            {
                printf("Failed to configure BWL for MC %d: %d\n", mc, sts);
            }
        }

        // Add the time since last engagement to the residency counter
        uint64_t current_gtimer_count = gtimer_prodfw_get_counter();
        if (current_gtimer_count < get_last_gtimer_count())
        {
            // Handle potential timer wrap-around
            set_last_gtimer_count(current_gtimer_count);
        }

        uint32_t delta_ticks = (uint32_t)(current_gtimer_count - get_last_gtimer_count());
        ddr_bwl_residency_add_ticks(delta_ticks);
        set_last_gtimer_count(0); // Reset the last_gtimer_count for next engagement

        ddr_manager_clear_memhot_gpio();
        s_bwlEngaged = false;
        printf("DDR BWL-\n");
    }
}

bool ddr_manager_get_bwl_engaged()
{
    return s_bwlEngaged;
}

uint8_t ddr_manager_get_bwl_state()
{
    return (uint8_t)bwl_state;
}

void ddr_manager_enable_bwl_i3c()
{
    if (bwl_state == DIMM_THROTTLE_SOURCE_NONE)
    {
        ddr_manager_control_bwl(START);
        printf("DDR BWL enabled by I3C\n");
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_BWL_ENABLED_BY_I3C);
    }

    bwl_state |= DIMM_THROTTLE_SOURCE_EXT_TEMP_SENSOR;
}

void ddr_manager_disable_bwl_i3c()
{
    dimm_throttle_source_t previous_bwl_state = bwl_state;

    bwl_state &= ~(DIMM_THROTTLE_SOURCE_EXT_TEMP_SENSOR);
    if ((bwl_state == DIMM_THROTTLE_SOURCE_NONE) && (previous_bwl_state == DIMM_THROTTLE_SOURCE_EXT_TEMP_SENSOR))
    {
        ddr_manager_control_bwl(STOP);
        printf("DDR BWL disabled by I3C\n");
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_BWL_DISABLED_BY_I3C);
    }
}

void ddr_manager_enable_bwl_mr4()
{
    if (bwl_state == DIMM_THROTTLE_SOURCE_NONE)
    {
        ddr_manager_control_bwl(START);
        printf("DDR BWL enabled by MR4\n");
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_BWL_ENABLED_BY_MR4);
    }

    bwl_state |= DIMM_THROTTLE_SOURCE_MR4;
}

void ddr_manager_disable_bwl_mr4()
{
    dimm_throttle_source_t previous_bwl_state = bwl_state;

    bwl_state &= ~(DIMM_THROTTLE_SOURCE_MR4);
    if ((bwl_state == DIMM_THROTTLE_SOURCE_NONE) && (previous_bwl_state == DIMM_THROTTLE_SOURCE_MR4))
    {
        ddr_manager_control_bwl(STOP);
        printf("DDR BWL disabled by MR4\n");
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_BWL_DISABLED_BY_MR4);
    }
}

void ddr_manager_enable_bwl_force()
{
    if (bwl_state == DIMM_THROTTLE_SOURCE_NONE)
    {
        ddr_manager_control_bwl(START);
        printf("DDR BWL forced enabled\n");
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_BWL_FORCED_ENABLE);
    }

    bwl_state |= DIMM_THROTTLE_SOURCE_FORCED_CLI;
}

void ddr_manager_disable_bwl_force()
{
    dimm_throttle_source_t previous_bwl_state = bwl_state;

    bwl_state &= ~(DIMM_THROTTLE_SOURCE_FORCED_CLI);
    if ((bwl_state == DIMM_THROTTLE_SOURCE_NONE) && (previous_bwl_state == DIMM_THROTTLE_SOURCE_FORCED_CLI))
    {
        ddr_manager_control_bwl(STOP);
        printf("DDR BWL forced disabled\n");
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_BWL_FORCED_DISABLE);
    }
}
