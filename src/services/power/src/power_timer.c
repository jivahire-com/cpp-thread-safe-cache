//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_timer.c
 * Implements the timer/counter requirements of the power service
 */

/*------------- Includes -----------------*/
#include "power_hw_int_i.h"
#include "power_i.h"
#include "power_loops_i.h"

#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <debug.h>
#include <gtimer_prodfw.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define PWR_CTRL_NAME "Pwr Ctrl Loop Timer"
/*------------- Typedefs -----------------*/
typedef struct _power_timer_context
{
    fpfw_tmr_entry_t control_loop_timer;
    fpfw_tmr_entry_t pvt_telem_loop_timer;
    fpfw_tmr_entry_t adclk_telem_timer;
} power_timer_context_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static power_timer_context_t s_power_timer_ctx;

/*------------- Functions ----------------*/

// TODO:  replace current implementation of this file with gtimer integration
//        https://azurecsi.visualstudio.com/Dev/_workitems/edit/1973153

uint64_t power_timer_get_counter()
{
    return gtimer_prodfw_get_counter();
}

uint64_t power_timer_get_counter_ticks_us(uint16_t time_in_us)
{
    //! 125MHz timer, 1 tick = 8 ns
    //! 1 tick = 0.008 us
    //! 1 us = 125 ticks
    uint64_t ticks_per_us = (uint64_t)gtimer_prodfw_get_frequency() / 1000000UL;
    uint64_t ticks_in_us = 0;
    if (time_in_us <= UINT64_MAX / ticks_per_us)
    {
        ticks_in_us = (uint64_t)time_in_us * ticks_per_us;
    }
    else
    {
        // Handle overflow case, e.g., log an error or return a safe value
        FPFW_RUNTIME_ASSERT(0 && "Integer overflow detected in power_timer_get_counter_ticks_us");
    }
    return ticks_in_us;
}

uint64_t power_timer_get_us_from_counter(uint32_t ticks)
{
    //! 125MHz timer
    //! 1 tick = 8 ns = 0.008 us
    //! 1 us = 125 ticks
    uint32_t time_per_tick_ns = (1000000000ULL) / (uint64_t)gtimer_prodfw_get_frequency(); //! If freq = 125MHz, this is 8ns
    if (ticks > UINT64_MAX / time_per_tick_ns)
    {
        // Handle overflow case, e.g., log an error or return a safe value
        FPFW_RUNTIME_ASSERT(0 && "Integer overflow detected in power_timer_get_us_from_counter");
        return 0; // Return a safe value in case of overflow
    }
    uint64_t time_in_us = (ticks * time_per_tick_ns) / 1000ULL; // convert to us
    return time_in_us;
}

void ctrl_loop_timer_cb(void* ctx, uint64_t exp_tick, uint64_t now_tick)
{
    FPFW_RUNTIME_ASSERT(ctx != 0);
    UNUSED(exp_tick);
    UNUSED(now_tick);

    power_runconfig_t* p_runconfig = (power_runconfig_t*)ctx;
    /* VR telemetry and the power control loop are driven by the
     * interval alarm; provide to VR telem first, since it triggers
     * AVS requests */
    if ((p_runconfig->knobs.loops_disable & (power_loops_disable_t_CTRL_LOOP | power_loops_disable_t_VR_TELEM_LOOP)) !=
        (power_loops_disable_t_CTRL_LOOP | power_loops_disable_t_VR_TELEM_LOOP))
    {
        /* telemetry event is sent if either loop is enabled, since control loop depends on VR current collection */
        power_loops_vr_telem_handle_event(POWER_VR_TELEM_SIGNAL_INTERVAL, NULL);
    }
    if ((p_runconfig->knobs.loops_disable & power_loops_disable_t_CTRL_LOOP) == 0)
    {
        power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_INTERVAL, NULL);
    }
}

void pvt_telem_loop_timer_cb(void* ctx, uint64_t exp_tick, uint64_t now_tick)
{
    FPFW_RUNTIME_ASSERT(ctx != 0);
    UNUSED(exp_tick);
    UNUSED(now_tick);

    power_runconfig_t* p_runconfig = (power_runconfig_t*)ctx;
    /* PVT telemetry driven by this pvt interval event */
    if ((p_runconfig->knobs.loops_disable & power_loops_disable_t_PVT_TELEM_LOOP) == 0)
    {
        power_loops_pvt_telem_handle_event(POWER_PVT_TELEM_SIGNAL_INTERVAL, NULL);
    }
}

void adclk_telem_timer_cb(void* ctx, uint64_t exp_tick, uint64_t now_tick)
{
    /* The adclk telemetry timer callback function, managed within the power
     * module, accumulates the adclk droop count at shorter intervals
     * (adclk_throt.telemetry_interval) to prevent overflow. The telemetry
     * service is expected to retrieve the accumulated results over a longer
     * period, such as 24 hours. Once retrieved, the droop count should be
     * reset to zero. */
    /// @todo: Need to revisit this once Task 2338766 is completed.
    ///        Need to review where telemetry service to call power_get_adclk_telem and power_reset_adclk_telem.
    FPFW_RUNTIME_ASSERT(ctx != 0);
    UNUSED(exp_tick);
    UNUSED(now_tick);

    power_runconfig_t* p_runconfig = (power_runconfig_t*)ctx;
    if ((p_runconfig->knobs.adclk_throt.enable) && (p_runconfig->knobs.adclk_throt.telemetry_interval > 0))
    {
        power_loops_adclk_telem_handle_event(POWER_ADCLK_TELEM_SIGNAL_INTERVAL, NULL);
    }
}

void power_timer_start_loop_timers()
{
    power_runconfig_t* p_runconfig = power_runconfig_get();

    const uint32_t ticks_per_ms = gtimer_prodfw_get_frequency() / 1000;
    gtimer_add_periodic(&s_power_timer_ctx.control_loop_timer,
                        p_runconfig->knobs.control_loop_interval * ticks_per_ms,
                        ctrl_loop_timer_cb,
                        p_runconfig);

    gtimer_add_periodic(&s_power_timer_ctx.pvt_telem_loop_timer,
                        p_runconfig->knobs.pvt_loop_interval * ticks_per_ms,
                        pvt_telem_loop_timer_cb,
                        p_runconfig);

    if ((p_runconfig->knobs.adclk_throt.enable) && (p_runconfig->knobs.adclk_throt.telemetry_interval > 0))
    {
        gtimer_add_periodic(&s_power_timer_ctx.adclk_telem_timer,
                            p_runconfig->knobs.adclk_throt.telemetry_interval * ticks_per_ms,
                            adclk_telem_timer_cb,
                            p_runconfig);
    }
}
