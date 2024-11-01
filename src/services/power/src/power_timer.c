//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_timer.c
 * Implements the timer/counter requirements of the power service
 */

/*------------- Includes -----------------*/
#include "power_i.h"
#include "power_loops_i.h"
#include "power_stub_i.h"

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
} power_timer_context_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static power_timer_context_t s_power_timer_ctx;

/*------------- Functions ----------------*/

// TODO:  replace current implementation of this file with gtimer integration
//        https://azurecsi.visualstudio.com/Dev/_workitems/edit/1973153

uint64_t power_timer_get_counter()
{
    return (uint64_t)tx_time_get();
}

uint64_t power_timer_get_counter_ticks_us(uint16_t time_in_us)
{
    FPFW_UNUSED(time_in_us);
    return 1000ULL; // some non-zero value for now
}

uint64_t power_timer_get_us_from_counter(uint32_t ticks)
{
    FPFW_UNUSED(ticks);
    return 0ULL;
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
}
