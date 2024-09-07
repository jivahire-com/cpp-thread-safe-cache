//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_temp_interrupts.c
 * Implements DDR Manager Temperature Interrupt API implementations
 */

/*------------- Includes -----------------*/
#include "ddr_manager_temp_interrupts.h"

#include "ddr_manager.h"
#include "ddr_manager_bwl.h"
#include "ddr_manager_i.h"

#include <bug_check.h>
#include <ddrss.h>
#include <ddrss_lib.h>
#include <ddrss_runtime_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static uint8_t config_manager_get_ddr_mc_temp_changed_range()
{
    // Get the DDR MC temperature range from the config manager
    // This is a stub implementation
    // Replace with config manager API / DDR knob API
    // ADO: 1831724
    return DDR_MR4_TEMP_85_90;
}

void ddr_manager_temp_interrupts_init()
{
    ddrss_register_temp_interrupts(ddr_manager_temp_interrupts_critical_cb, ddr_manager_temp_interrupts_changed_cb);
}

void ddr_manager_temp_interrupts_critical_cb(uint32_t mc)
{
    // Trigger the thermal trip
    ddr_manager_set_thermal_trip_gpio();

    // Enqueue temp update event
    ddr_manager_message_t work_queue_msg = {.message_type = DDR_TEMP_CHANGED_EVENT, .message_data = mc};
    ddr_manager_enqueue_event(&work_queue_msg);
}

void ddr_manager_temp_interrupts_changed_cb(uint32_t mc)
{
    // Enqueue temp update event
    ddr_manager_message_t work_queue_msg = {.message_type = DDR_TEMP_CHANGED_EVENT, .message_data = mc};
    ddr_manager_enqueue_event(&work_queue_msg);
}

void ddr_manager_temp_interrupts_process_changed_event(uint32_t mc)
{
    ddrss_refresh_rate_sts_t refresh_rate_sts;
    int read_status = ddrss_refresh_rate_read_status(mc, &refresh_rate_sts);
    BUG_ASSERT(read_status == SILIBS_SUCCESS);

    // Get the max temperature range from the DDR MC
    uint8_t max_temp_range = 0;
    for (uint8_t dimm_rank = 0; dimm_rank < DDRSS_MAX_DIMM_RANK; dimm_rank++)
    {
        for (uint8_t sdram_dev = 0; sdram_dev < DDRSS_MAX_DIMM_SDRAM_DEV_PER_SUB_CH; sdram_dev++)
        {
            if (refresh_rate_sts.dram_temp[dimm_rank][sdram_dev] > max_temp_range)
            {
                max_temp_range = refresh_rate_sts.dram_temp[dimm_rank][sdram_dev];
            }
        }
    }

    // Engage BWL if temperature range at or above high threshold
    if (max_temp_range >= config_manager_get_ddr_mc_temp_changed_range())
    {
        ddr_manager_enable_bwl_mr4();
    }
    else
    {
        ddr_manager_disable_bwl_mr4();
    }
}