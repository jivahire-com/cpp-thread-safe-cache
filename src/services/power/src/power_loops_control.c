//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_loops_control.c
 * Implements the power service control loop
 */

/*------------- Includes -----------------*/
#include "pid_resource.h"
#include "power_distribution_i.h"
#include "power_events.h"
#include "power_hw_int_i.h"
#include "power_i.h"
#include "power_loops_i.h"
#include "power_remote_die_i.h"
#include "power_runconfig_i.h"
#include "power_stub_i.h"

#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <corebits.h>
#include <debug.h>
#include <fpfw_cfg_mgr.h>
#include <idsw_kng.h>
#include <inttypes.h>
#include <scf_power.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/
/*------------- Typedefs -----------------*/
/*-------- Function Prototypes -----------*/
// Following are function prototypes for state handlers (control loop)
static void idle_handler(int event, const void* event_data);
static void warmstart_entry_handler(int event, const void* event_data);
static void collect_inputs_handler(int event, const void* event_data);
static void exchange_inputs_handler(int event, const void* event_data);
static void distribute_available_handler(int event, const void* event_data);
static void set_plimit_before_handler(int event, const void* event_data);
static void set_vr_after_handler(int event, const void* event_data);
static void wait_vr_after_handler(int event, const void* event_data);
static void set_vr_before_handler(int event, const void* event_data);
static void wait_vr_before_handler(int event, const void* event_data);
static void set_plimit_after_handler(int event, const void* event_data);
static void exchange_completion_handler(int event, const void* event_data);
static void error_handler(int event, const void* event_data);

static void hw_get_core_state(power_runconfig_t* p_runconfig, unsigned core, power_core_t* core_state);
static void hw_drain_sensor_fifo();
static void hw_calculate_vcpu(power_runconfig_t* p_runconfig);
static void hw_write_plimits(power_runconfig_t* p_runconfig);

// functions to resolve throttling/boost priorities
static uint8_t throt_pri_for_tracking(power_runconfig_t* p_runconfig, uint8_t priority);
static uint8_t boost_pri_for_tracking(power_runconfig_t* p_runconfig, uint8_t priority);

/*-- Declarations (Statics and globals) --*/
#define DEFAULT_PRIORITY 0
// Default value of PLIMIT_T1/2/3 is copied from silibs/libraries/dvfs/include/dvfs_struct.h
#define DEFAULT_PLIMIT_T1 0x66 /* 40% of maximum threshold */
#define DEFAULT_PLIMIT_T2 0xA7 /* 65% of maximum threshold. Alarm will trigger above this */
#define DEFAULT_PLIMIT_T3 0xE6 /* 90% of maximum threshold. Highest check for cte alarm */

// Table of state handler functions for control loop
static const power_state_handler_t control_loop_handler_table[POWER_CONTROL_STATE_MAX] = {
    [POWER_CONTROL_STATE_IDLE] = idle_handler,
    [POWER_CONTROL_STATE_WARMSTART_ENTRY] = warmstart_entry_handler,
    [POWER_CONTROL_STATE_COLLECT_INPUTS] = collect_inputs_handler,
    [POWER_CONTROL_STATE_EXCHANGE_INPUTS] = exchange_inputs_handler,
    [POWER_CONTROL_STATE_DISTRIBUTE_AVAILABLE] = distribute_available_handler,
    [POWER_CONTROL_STATE_SET_PLIMIT_BEFORE_VR] = set_plimit_before_handler,
    [POWER_CONTROL_STATE_SET_VR_AFTER_PLIMIT] = set_vr_after_handler,
    [POWER_CONTROL_STATE_WAIT_VR_AFTER_PLIMIT] = wait_vr_after_handler,
    [POWER_CONTROL_STATE_SET_VR_BEFORE_PLIMIT] = set_vr_before_handler,
    [POWER_CONTROL_STATE_WAIT_VR_BEFORE_PLIMIT] = wait_vr_before_handler,
    [POWER_CONTROL_STATE_SET_PLIMIT_AFTER_VR] = set_plimit_after_handler,
    [POWER_CONTROL_STATE_EXCHANGE_COMPLETION] = exchange_completion_handler,
    [POWER_CONTROL_STATE_ERROR] = error_handler,
};
static power_loop_residency_t control_loop_handler_residency[POWER_CONTROL_STATE_MAX] = {0};

static power_loop_context_t s_control_loop_context = {.state_count = POWER_CONTROL_STATE_MAX,
                                                      .handlers = control_loop_handler_table,
                                                      .residency = control_loop_handler_residency,
                                                      .id = LOOP_ID_CONTROL};
static power_ctrl_loop_detail_t s_ctrl_loop = {0};

/*------------- Functions ----------------*/
static bool power_control_loop_retry_fail(power_loop_retries_t type)
{
    // call the common function with detail for power control loop
    return power_loops_retry_fail(&s_control_loop_context, type);
}

static void power_control_loop_change_state(power_ctrl_loop_state_t state)
{
    FPFW_RUNTIME_ASSERT(state < POWER_CONTROL_STATE_MAX);
    // update the timestamp used in power logs
    /*
    power_log_update_timestamp(power_timer_get_counter());
    */
    // call the common function with detail for power control loop
    return power_loops_change_state(&s_control_loop_context, (int)state);
}

void power_loops_control_handle_event(power_ctrl_loop_signal_t event, const void* event_data)
{
    power_loops_handle_event(&s_control_loop_context, (int)event, event_data);
}

/*-- Power control loop state handlers --*/

static void idle_handler(int event, const void* event_data)
{
    UNUSED(event_data);

    switch (event)
    {
    case POWER_LOOP_STATE_SIGNAL_ENTRY:
        // Returning to idle state
        POWER_LOG_TRACE("Control loop idle");
        // reset necessary state
        corebits_clear(&s_ctrl_loop.plimits_pending);
        corebits_clear(&s_ctrl_loop.plimits_successful);
        // clear failure condition if loop iteration successful
        if ((s_ctrl_loop.loop_failure) && (s_control_loop_context.status.last_state != POWER_CONTROL_STATE_ERROR))
        {
            s_ctrl_loop.loop_failure = false;
            // clear HW signal
            power_hw_clear_force_pmin(PM_FW_PMIN_CONTROL);
        }
        // clear remote die status
        power_remote_die_idle_reset();
        break;
    case POWER_CTRL_LOOP_SIGNAL_INTERVAL:
        // Leaving idle state
        power_control_loop_change_state(POWER_CONTROL_STATE_COLLECT_INPUTS);
        break;
    default:
        break;
    }
}

static void get_current_state()
{
    power_runconfig_t* p_runconfig = power_runconfig_get();
    const uint8_t pnominal = p_runconfig->derived.pnominal;

    // clear priority list
    // number of boost rows = value of pnominal
    const size_t size_of_one_row = sizeof(s_ctrl_loop.local.pri_counts.required_for_boost[0]);
    memset((char*)s_ctrl_loop.local.pri_counts.required_for_boost, 0, size_of_one_row * pnominal);

    // now read per core state
    for (unsigned core_idx = 0; core_idx < NUM_AP_CORES_PER_DIE; ++core_idx)
    {
        // only want to engage in state collection for valid cores
        if (corebits_is_bit_set(&p_runconfig->fuses.valid_cores, core_idx))
        {
            power_core_t* core = &s_ctrl_loop.cores.core[core_idx];
            hw_get_core_state(p_runconfig, core_idx, core);

            // get the boost priority for this core
            const unsigned boost_pri_index = core->current_boost_priority;

            // find the minimum (max performance) allowable plimit for this core
            // based on collected data
            uint8_t core_min_plimit = power_distribution_get_minimum_plimit(p_runconfig, &s_ctrl_loop.cores, core_idx);

            // below would be totally unexpected, but since we're using
            // values passed in through config (which should already be
            // doing bounds checks), adding this to be
            if (core_min_plimit > MAX_PLIMIT)
            {
                POWER_ET_WARN(POWER_ET_TYPE_CTRLLOOP_INVALID_MIN_PLIMIT,
                              POWER_ET_ENCODE_DETAIL_CORE(core_min_plimit, core_idx));
                POWER_LOG_ERR("[POWER CTRL LOOP] Invalid min plimit: %d", core_min_plimit);
                core_min_plimit = MAX_PLIMIT;
            }

            // set value per core for eventual adjustment
            core->min_plimit = core_min_plimit;

            // nominal has to be the middle point in our required table, and we only want to increment entries in boost perf levels
            if ((pnominal > 0) && (core_min_plimit < pnominal))
            {
                // increment the count of cores that require this minimum plimit in the current priority level
                s_ctrl_loop.local.pri_counts.required_for_boost[core_min_plimit][boost_pri_index]++;
            }
        }
    }

    // complete core count data in the boost range
    for (int plimit_idx = 1; plimit_idx < pnominal; ++plimit_idx)
    {
        for (int boost_pri_index = 0; boost_pri_index < VM_BOOST_COUNT; ++boost_pri_index)
        {
            // if a plimit is valid in a higher perf, we'll automatically indicate
            // it is valid in a lower perf
            s_ctrl_loop.local.pri_counts.required_for_boost[plimit_idx][boost_pri_index] +=
                s_ctrl_loop.local.pri_counts.required_for_boost[plimit_idx - 1][boost_pri_index];
        }
    }
}

static uint8_t boost_pri_for_tracking(power_runconfig_t* p_runconfig, uint8_t priority)
{
    uint8_t boost_pri_index = 0;
    if (p_runconfig->knobs.power_enable_velocity_boost)
    {
        // as long as OS-requested priority is less than number of
        // priorities we support, use OS priority
        boost_pri_index = (priority < VM_BOOST_COUNT) ? priority : (VM_BOOST_COUNT - 1);
    }
    return boost_pri_index;
}

static uint8_t throt_pri_for_tracking(power_runconfig_t* p_runconfig, uint8_t priority)
{
    unsigned throt_pri_index = 0; // default index for all-core mode
    if (p_runconfig->knobs.capping_mode != power_capping_mode_t_ALL)
    {
        // as long as OS-requested priority is less than number of
        // priorities we support, use OS priority
        throt_pri_index = (priority < VM_THROT_COUNT) ? priority : (VM_THROT_COUNT - 1);
    }
    return throt_pri_index;
}

static void collect_inputs_handler(int event, const void* event_data)
{
    UNUSED(event_data);

    switch (event)
    {
    case POWER_LOOP_STATE_SIGNAL_ENTRY:
        // Start state collection
        // Request VR currents - done from telem loop not here

        // Drain Sensor FIFO
        hw_drain_sensor_fifo();
        // Get core state from SCF, update min plimit
        get_current_state();
        break;
    case POWER_CTRL_LOOP_SIGNAL_VR_READ:
        // state collection done?
        FPFW_RUNTIME_ASSERT(event_data != NULL);
        s_ctrl_loop.local.power = *(power_latest_calcs_t*)event_data;
        power_control_loop_change_state(POWER_CONTROL_STATE_EXCHANGE_INPUTS);
        break;
    case POWER_CTRL_LOOP_SIGNAL_INTERVAL:
        if (power_control_loop_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL))
        {
            power_control_loop_change_state(POWER_CONTROL_STATE_ERROR);
        }
        break;
    default:
        // unexpected to receive any other event in this state, but ignore
        break;
    }
}

static void warmstart_entry_handler(int event, const void* event_data)
{
    UNUSED(event);
    UNUSED(event_data);

    // get necessary runtime config data
    power_runconfig_t* p_runconfig = power_runconfig_get();

    // Fixed Distribution
    power_distribution_distribute_warmstart_resources(p_runconfig, &s_ctrl_loop);

    // clear pending bits for any CPUs that are disabled
    corebits_and(&s_ctrl_loop.plimits_pending, &p_runconfig->fuses.valid_cores);

    // Calculate Vcpu - call also used as a hook to modify during test
    hw_calculate_vcpu(p_runconfig);

    // PLIMITs changed; assume VR setpoint increasing
    return power_control_loop_change_state(POWER_CONTROL_STATE_SET_VR_BEFORE_PLIMIT);
}

static void distribute_available_handler(int event, const void* event_data)
{
    UNUSED(event_data);

    bool new_rack_power_cap = false;

    power_runconfig_t* p_runconfig = power_runconfig_get();

    switch (event)
    {
    case POWER_LOOP_STATE_SIGNAL_ENTRY: {
        // Start state distribute available

        // this update should really be a one-time thing, but we'll do it here to simplify synchronization
        if (s_ctrl_loop.max_resources != (s_ctrl_loop.local.max_resources + s_ctrl_loop.remote.max_resources))
        {
            // update the pid config to include local + remote max resources
            pid_config_t pidcfg;
            s_ctrl_loop.max_resources = s_ctrl_loop.local.max_resources + s_ctrl_loop.remote.max_resources;
            pid_get_config(&pidcfg);
            pidcfg.max = s_ctrl_loop.max_resources;
            pid_init(&pidcfg);
        }

        // Calculate CPU power/cap
        float temp =
            power_cap_get_vrcpu_cap(&new_rack_power_cap, &s_ctrl_loop.local.power, &s_ctrl_loop.remote.power);
        pid_update_setpoint(temp);

        // on new rack cap, reset PID integral / error accumulation
        if (new_rack_power_cap)
        {
            pid_reset();
        }

        // PID
        const bool rack_limit = power_hw_gpio_rack_limit_asserted();
        s_ctrl_loop.rack_limit = rack_limit;
        if ((rack_limit) || (s_ctrl_loop.loop_failure))
        {
            // if we're rack limited via gpio or in a loop failure scenario, reset PID and set resources to 0, so that loop selections
            // (all minimum perf) will match forced HW state; has the side-effect of lowering Vcpu

            pid_reset();
            pid_set_resources(0);

            s_ctrl_loop.curr_resources = 0;
        }
        else
        {
            s_ctrl_loop.curr_resources =
                pid_calculate_resources((float)p_runconfig->knobs.control_loop_interval / 1000,
                                        (s_ctrl_loop.local.power.vcpu_power + s_ctrl_loop.remote.power.vcpu_power));
        }

        /* TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1946116
                 enable power log

        power_log_cores_ts(power_timer_get_counter(),
                           &ALLCORES,
                           POWER_LOG_DATA(DIST_AVAIL,
                                          {.soc_pwr  = power_context.soc_power[0],
                                           .vcpu_pwr = power_context.vcpu_power,
                                           .soc_cap  = MIN(power_context.soc_power_cap_watts,
                                                              (float)p_runconfig->soc_maximum_thermal_watts_limit),
                                           .vcpu_cap = temp}));
        */

        // Determine Distribution
        power_distribution_distribute_resources(p_runconfig, &s_ctrl_loop);

        // sets pending bits on a configured minimum number of cores
        power_distribution_minimum_plimit_updates(p_runconfig, &s_ctrl_loop);

        // clear pending bits for any CPUs that are disabled
        corebits_and(&s_ctrl_loop.plimits_pending, &p_runconfig->fuses.valid_cores);

        // Calculate Vcpu
        hw_calculate_vcpu(p_runconfig);

        // Next state based on plimit changes pending, VR increasing/decreasing
        // PLIMITs change
        if (s_ctrl_loop.required_vcpu < s_ctrl_loop.current_vcpu)
        {
            // PLIMITs changed; VR setpoint decreasing
            power_control_loop_change_state(POWER_CONTROL_STATE_SET_PLIMIT_BEFORE_VR);
        }
        else
        {
            // PLIMITs changed; VR setpoint increasing
            power_control_loop_change_state(POWER_CONTROL_STATE_SET_VR_BEFORE_PLIMIT);
        }
    }
    break;
    default:
        // unexpected, as entry event causes state change; ignore
        break;
    }
}

// Below is a common handler for several states which write VCPU voltage
static void set_vr_handler(power_ctrl_loop_state_t next_state_set,
                           power_ctrl_loop_state_t next_state_done,
                           power_ctrl_loop_signal_t event,
                           const void* event_data)
{
    UNUSED(event_data);

    switch (event)
    {
    case POWER_LOOP_STATE_SIGNAL_ENTRY:
    case POWER_CTRL_LOOP_SIGNAL_VCPU_SET_FAIL:
        // State entry or failure/retry condition
        // Set VR(Vcpu)
        power_vrs_write_vcpu_voltage(s_ctrl_loop.required_vcpu);
        break;
    case POWER_CTRL_LOOP_SIGNAL_VCPU_DONE:
        power_control_loop_change_state(next_state_done);
        break;
    case POWER_CTRL_LOOP_SIGNAL_VCPU_PENDING:
        // on set acknowledge, we need to read until change is done
        power_control_loop_change_state(next_state_set);
        break;
    case POWER_CTRL_LOOP_SIGNAL_INTERVAL:
        if (power_control_loop_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL))
        {
            power_control_loop_change_state(POWER_CONTROL_STATE_ERROR);
        }
        break;
    default:
        // unexpected to receive any other event in this state, but ignore
        break;
    }
}

// Below is a common handler for several states which write VCPU voltage
static void wait_vr_handler(power_ctrl_loop_state_t next_state_done, power_ctrl_loop_signal_t event, const void* event_data)
{
    UNUSED(event_data);

    switch (event)
    {
    case POWER_LOOP_STATE_SIGNAL_ENTRY:
    case POWER_CTRL_LOOP_SIGNAL_VCPU_SET_FAIL:
    case POWER_CTRL_LOOP_SIGNAL_VCPU_PENDING:
        // State entry
        // Read VR(Vcpu) for done bit
        power_vrs_read_vcpu_voltage();
        break;
    case POWER_CTRL_LOOP_SIGNAL_VCPU_DONE:
        power_control_loop_change_state(next_state_done);
        break;
    case POWER_CTRL_LOOP_SIGNAL_INTERVAL:
        if (power_control_loop_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL))
        {
            power_control_loop_change_state(POWER_CONTROL_STATE_ERROR);
        }
        break;
    default:
        // unexpected to receive any other event in this state, but ignore
        break;
    }
}

static void set_vr_after_handler(int event, const void* event_data)
{
    // call common handler indicating next state after VR change complete
    // if we're setting VR after PLIMIT change, next state is completion
    set_vr_handler(POWER_CONTROL_STATE_WAIT_VR_AFTER_PLIMIT, POWER_CONTROL_STATE_EXCHANGE_COMPLETION, event, event_data);
}

static void wait_vr_after_handler(int event, const void* event_data)
{
    // call common handler indicating next state after VR change complete
    // if we're setting VR after PLIMIT change, next state is completion
    wait_vr_handler(POWER_CONTROL_STATE_EXCHANGE_COMPLETION, event, event_data);
}

static void set_vr_before_handler(int event, const void* event_data)
{
    // call common handler indicating next state after VR change complete
    // if we're setting VR before PLIMIT change, next state is
    // SET_PLIMIT_AFTER_VR
    set_vr_handler(POWER_CONTROL_STATE_WAIT_VR_BEFORE_PLIMIT, POWER_CONTROL_STATE_SET_PLIMIT_AFTER_VR, event, event_data);
}

static void wait_vr_before_handler(int event, const void* event_data)
{
    // call common handler indicating next state after VR change complete
    // if we're setting VR after PLIMIT change, next state is idle
    wait_vr_handler(POWER_CONTROL_STATE_SET_PLIMIT_AFTER_VR, event, event_data);
}

// Below is a common handler for several states which update plimits
static void set_plimit_handler(power_ctrl_loop_state_t next_state, power_ctrl_loop_signal_t event, const void* event_data)
{
    UNUSED(event_data);

    power_runconfig_t* p_runconfig = power_runconfig_get();

    const uint32_t retry_ticks = power_timer_get_counter_ticks_us(
        (p_runconfig->knobs.control_loop_interval * 1000 /* interval is in ms*/) / POWER_LOOP_RETRY_COUNT);
    power_loop_plimit_stats_t* const plimit = &s_ctrl_loop.plimit;
    const uint64_t counter = power_timer_get_counter();

    switch (event)
    {
    case POWER_LOOP_STATE_SIGNAL_ENTRY:
        // Start state collection
        // Set PLIMITs
        hw_write_plimits(p_runconfig);
        // Push PLIMIT pending event
        power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_PLIMIT_PENDING, NULL);
        // store start_counter and first plimit send (last retry time)
        plimit->counter_start = counter;
        plimit->counter_last_send = counter;
        break;
    case POWER_CTRL_LOOP_SIGNAL_PLIMIT_PENDING: {
        // Drain Sensor FIFO
        hw_drain_sensor_fifo();
        // all Plimits successful?
        corebits_t successful = s_ctrl_loop.plimits_successful;
        corebits_and(&successful, &s_ctrl_loop.plimits_pending);
        if (corebits_is_equal(&s_ctrl_loop.plimits_pending, &successful))
        {
            // we're done - can indicate power cap in place, store plimit stats
            const uint64_t total_us = power_timer_get_us_from_counter(counter - plimit->counter_start);
            plimit->last_us = total_us;
            plimit->max_us = MAX(total_us, plimit->max_us);
            plimit->min_us = (plimit->min_us == 0) ? total_us : MIN(total_us, plimit->min_us);
            power_cap_finalize();
            // switch to next state
            power_control_loop_change_state(next_state);
        }
        else
        {
            // we're not done - some plimit responses still not received
            // if elapsed time matches our retry time, resend the plimit request to remaining cores
            if (((counter - plimit->counter_last_send) >= retry_ticks) &&
                !power_control_loop_retry_fail(POWER_LOOP_RETRY_TYPE_STATE))
            {
                // update last send time
                plimit->counter_last_send = counter;
                // set remaining plimits
                hw_write_plimits(p_runconfig);
            }
            // Push PLIMIT pending event
            power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_PLIMIT_PENDING, NULL);
        }
    }
    break;
    case POWER_CTRL_LOOP_SIGNAL_INTERVAL:
        // error to receive any other event in this state
        if (power_control_loop_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL))
        {
            POWER_LOG_ERR("Pending PLIMIT changes.");
            POWER_ET_AFFECTED_CORES(POWER_ET_TYPE_CTRLLOOP_PLIMITS_PENDING, s_ctrl_loop.plimits_pending);
            POWER_ET_AFFECTED_CORES(POWER_ET_TYPE_CTRLLOOP_PLIMITS_SUCCESSFUL, s_ctrl_loop.plimits_successful);
            for (unsigned pending_idx = 0; pending_idx < BITTYPE_COUNT; ++pending_idx)
            {
                POWER_LOG_ERR("%d Pending %08" PRIx32 " Successful %08" PRIx32,
                              pending_idx,
                              s_ctrl_loop.plimits_pending.bits[pending_idx],
                              s_ctrl_loop.plimits_successful.bits[pending_idx]);
            }
            power_control_loop_change_state(POWER_CONTROL_STATE_ERROR);
        }
        break;
    default:
        // ignore anything else
        break;
    }
}

static void set_plimit_before_handler(int event, const void* event_data)
{
    // call common handler indicating next state after plimit change complete
    // if we set plimit before VR, next state is set VR
    set_plimit_handler(POWER_CONTROL_STATE_SET_VR_AFTER_PLIMIT, event, event_data);
}
static void set_plimit_after_handler(int event, const void* event_data)
{
    // call common handler indicating next state after plimit change complete
    // if we set plimit after VR, next state is completion
    set_plimit_handler(POWER_CONTROL_STATE_EXCHANGE_COMPLETION, event, event_data);
}

static void exchange_inputs_handler(int event, const void* event_data)
{
    UNUSED(event_data);

    switch (event)
    {
    case POWER_LOOP_STATE_SIGNAL_ENTRY:
        // Start state exchange inputs
        // Exchange inputs
        power_remote_die_exchange_inputs(power_runconfig_get());
        break;
    case POWER_CTRL_LOOP_SIGNAL_EXCHANGE_INPUTS:
        // exchange inputs done
        power_control_loop_change_state(POWER_CONTROL_STATE_DISTRIBUTE_AVAILABLE);
        break;
    case POWER_CTRL_LOOP_SIGNAL_INTERVAL:
        // retry on interval signal
        if (power_control_loop_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL))
        {
            power_control_loop_change_state(POWER_CONTROL_STATE_ERROR);
            break;
        }
        // otherwise, try again
        power_remote_die_exchange_inputs(power_runconfig_get());
        break;
    default:
        // ignore anything else
        break;
    }
}

static void exchange_completion_handler(int event, const void* event_data)
{
    UNUSED(event_data);

    switch (event)
    {
    case POWER_LOOP_STATE_SIGNAL_ENTRY:
        // Start state exchange completion
        // Exchange completion
        power_remote_die_exchange_complete(power_runconfig_get());
        break;
    case POWER_CTRL_LOOP_SIGNAL_EXCHANGE_COMPLETE:
        // exchange completion done
        power_control_loop_change_state(POWER_CONTROL_STATE_IDLE);
        break;
    case POWER_CTRL_LOOP_SIGNAL_INTERVAL:
        // retry on interval signal
        if (power_control_loop_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL))
        {
            power_control_loop_change_state(POWER_CONTROL_STATE_ERROR);
            break;
        }
        // otherwise, try again
        power_remote_die_exchange_complete(power_runconfig_get());
        break;
    default:
        // ignore anything else
        break;
    }
}

static void error_handler(int event, const void* event_data)
{
    UNUSED(event_data);

    switch (event)
    {
    case POWER_LOOP_STATE_SIGNAL_ENTRY:
        // if we've entered error state, control loop is not able to influence soc power
        // force pmin with HW signal
        s_ctrl_loop.loop_failure = true;
        s_ctrl_loop.loop_fail_query |= true; // shouldn't be cleared until queried
        power_hw_force_pmin(PM_FW_PMIN_CONTROL);
        POWER_ET_ERROR(POWER_ET_TYPE_CTRLLOOP_ERROR_ENTRY,
                       POWER_ET_ENCODE_RETRIES_STATE(s_control_loop_context.status.retries[POWER_LOOP_RETRY_TYPE_INTERVAL],
                                                     s_control_loop_context.status.last_state));
        POWER_LOG_INFO("Control loop error state entry, retries exhausted in state %d: interval retries %d",
                       s_control_loop_context.status.last_state,
                       s_control_loop_context.status.retries[POWER_LOOP_RETRY_TYPE_INTERVAL]);
        POWER_LOG_INFO("return to idle on next alarm");
        break;
    case POWER_CTRL_LOOP_SIGNAL_INTERVAL:
        power_control_loop_change_state(POWER_CONTROL_STATE_IDLE);
        break;
    default:
        break;
    }
}

static void hw_get_core_state(power_runconfig_t* p_runconfig, unsigned core, power_core_t* core_state)
{
    SCF_CORE_POWER_STATE_T last_state;
    // find tile
    const unsigned int tile_num = core / CORES_PER_TILE;
    // actual implementation would read last pstate from SCF registers
    const int ret = scf_get_core_state(core, p_runconfig->p_sconfig->scf_mhu_base, false, &last_state);
    // only reason for failure is passing in a NULL, which won't happen
    // dynamically
    FPFW_RUNTIME_ASSERT(ret == 0);
    core_state->current_cstate = last_state.cstate;
    core_state->current_pstate = last_state.pstate;
    core_state->current_throt_status = last_state.throt_status;
    // for now storing in 0.1C increments, consistent with AVS
    core_state->temperature_dC =
        power_hw_dts_pvt_raw_to_temp_dC(last_state.temperature, p_runconfig->fuses.dts_coeff_tile[tile_num]);
}

// callback function for dvfs update messages
void message_update_callback(unsigned int core_id, uint8_t desired_pstate, uint8_t base_pstate, uint8_t throttle_pri, uint8_t boost_pri)
{
    FPFW_RUNTIME_ASSERT(core_id < NUM_AP_CORES_PER_DIE);

    power_runconfig_t* p_runconfig = power_runconfig_get();

    // base pstate must be nominal or greater (less perf)
    base_pstate = MAX(base_pstate, p_runconfig->derived.pnominal);
    // qualify priorities with config, etc
    throttle_pri = throt_pri_for_tracking(p_runconfig, throttle_pri);
    boost_pri = boost_pri_for_tracking(p_runconfig, boost_pri);

    // save old base and throttle priority
    const uint8_t current_base_pstate = s_ctrl_loop.cores.core[core_id].current_base_pstate;
    const uint8_t current_throt_priority = s_ctrl_loop.cores.core[core_id].current_throt_priority;
    const uint8_t current_boost_priority = s_ctrl_loop.cores.core[core_id].current_boost_priority;

    // update message packets will be sent when a CPPC register is update in DVFS NON-SEC (OS write)
    s_ctrl_loop.cores.core[core_id].desired_pstate = desired_pstate;
    s_ctrl_loop.cores.core[core_id].current_throt_priority = throttle_pri;
    s_ctrl_loop.cores.core[core_id].current_boost_priority = boost_pri;
    s_ctrl_loop.cores.core[core_id].current_base_pstate = base_pstate;

    // if priority or pstate have changed
    if ((current_throt_priority != throttle_pri) || (current_boost_priority != boost_pri) ||
        (current_base_pstate != base_pstate))
    {
        // update the local count of cores using this perf level for nominal
        s_ctrl_loop.local.pri_counts.required_nominal_for_throttle[current_base_pstate][current_throt_priority]--;
        s_ctrl_loop.local.pri_counts.required_nominal_for_throttle[base_pstate][throttle_pri]++;

        // update the local count of cores using this perf level for nominal with this boost priority
        s_ctrl_loop.local.pri_counts.required_for_boost[current_base_pstate][current_boost_priority]--;
        s_ctrl_loop.local.pri_counts.required_for_boost[base_pstate][boost_pri]++;

        // update core count at priority level
        s_ctrl_loop.local.pri_counts.throt_pri_count[current_throt_priority]--;
        s_ctrl_loop.local.pri_counts.throt_pri_count[throttle_pri]++;

        // update corebits for throttle priority level
        corebits_clear_bit(&s_ctrl_loop.throttle_priority[current_throt_priority], core_id);
        corebits_set_bit(&s_ctrl_loop.throttle_priority[throttle_pri], core_id);

        // update corebits for boost priority level
        corebits_clear_bit(&s_ctrl_loop.boost_priority[current_boost_priority], core_id);
        corebits_set_bit(&s_ctrl_loop.boost_priority[boost_pri], core_id);
    }

    POWER_LOG_TRACE("update message, core %d", core_id);
}

// callback function for dvfs success messages
void message_success_callback(unsigned int core_id, uint8_t desired_pstate, uint8_t current_pstate)
{
    FPFW_RUNTIME_ASSERT(core_id < NUM_AP_CORES_PER_DIE);

    // a success message packet will be sent after evert PLIMIT register write
    // success messages will always include current CPPC desired perf value, so save it here
    s_ctrl_loop.cores.core[core_id].desired_pstate = desired_pstate;
    if (s_ctrl_loop.cores.core[core_id].selected_plimit == current_pstate)
    {
        // on success, we update the current plimit and set the core success bit
        s_ctrl_loop.cores.core[core_id].current_plimit = current_pstate;
        corebits_set_bit(&s_ctrl_loop.plimits_successful, core_id);
        POWER_LOG_TRACE("success message, core %d selected %d", core_id, current_pstate);
    }
    else
    {
        // TODO: this is fails constantly and spams uart,  enable once resolved
        // task https://azurecsi.visualstudio.com/Dev/_workitems/edit/2199252

        // this is unexpected, but keep the print for early HW issues
        // if it does happen, control loop will retry due to success bit not being set
        // POWER_LOG_WARN("plimit success core%d selected %d != success %d",
        //                core_id,
        //                s_ctrl_loop.cores.core[core_id].selected_plimit,
        //                current_pstate);
    }
}

static void hw_drain_sensor_fifo()
{
    // call poll function, process success/fails
    power_telemetry_message_poll(message_update_callback, message_success_callback);
}

static void hw_calculate_vcpu(power_runconfig_t* p_runconfig)
{
    // calculate the necessary LDO input voltage
    // TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1505723/

    // reminder that vcpu_calc needs to provide per-core current limit data used in loadline calculation (we need to save that calculation for setting of plimit later)
    // TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/create/Build%20Task

    uint16_t ldo_in_mv =
        power_vcpu_calc_max_core_voltage_mv(p_runconfig, &s_ctrl_loop.cores) + p_runconfig->fuses.v_ldo_dropout_mv;

    float r_loadline_uohm = (float)p_runconfig->knobs.r_loadline_vcpu0_uohm;
    if (idsw_get_die_id() == DIE_1)
    {
        r_loadline_uohm = (float)p_runconfig->knobs.r_loadline_vcpu1_uohm;
    }

    float loadline_drop_mv = power_vcpu_calc_peak_current_A(p_runconfig, &s_ctrl_loop) * r_loadline_uohm /
                             1000.0F; // r_loadline is uOhm - /1000 makes result mV

    const uint16_t possible_loadline_drop_mV = (uint16_t)FLOAT_TO_UNSIGNED(loadline_drop_mv);
    const uint16_t vcpu_required_mV = ldo_in_mv + p_runconfig->fuses.vcpu_guardband_mv + p_runconfig->knobs.vcpu_offset_mv;

    s_ctrl_loop.required_vcpu = vcpu_required_mV + possible_loadline_drop_mV;

    // sanity check the voltage request
    if (s_ctrl_loop.required_vcpu < vcpu_required_mV)
    {
        POWER_ET_ERROR(POWER_ET_TYPE_VCPU_UNEXPECED_LL_LOSS_CALCULATION, possible_loadline_drop_mV);
        POWER_LOG_WARN("Unexpected ll_loss calc: %04x (+%04x=%04x)",
                       possible_loadline_drop_mV,
                       vcpu_required_mV,
                       s_ctrl_loop.required_vcpu);
        s_ctrl_loop.required_vcpu = vcpu_required_mV;
    }

    POWER_LOG_TRACE("ldo_in_mv - %d loadline_drop_mv - %f required_vcpu - %d",
                    ldo_in_mv,
                    loadline_drop_mv,
                    s_ctrl_loop.required_vcpu);
}

static void hw_write_plimits(power_runconfig_t* p_runconfig)
{
    // local for rack_limited pstate
    uint8_t plimited = power_cap_is_capped() ? p_runconfig->derived.pnominal : MAX_PLIMIT;

    // changes should be the set of cores with pending and not yet successful plimit updates
    corebits_t changes = s_ctrl_loop.plimits_successful;
    corebits_inv(&changes);
    corebits_and(&changes, &s_ctrl_loop.plimits_pending);

    int core = corebits_first(&changes);
    while (core != -1)
    {
        // if the selected limit is greater than the established plimit for rack throttling, rack_limit_throttle should be true
        bool rack_limit_throttle = (s_ctrl_loop.cores.core[core].selected_plimit > plimited);
        // clear the core from changes; we'll iterate until this is empty
        corebits_clear_bit(&changes, core);

        dvfs_plimit plimit_req = {
            .vf_index = s_ctrl_loop.cores.core[core].selected_plimit,
            .power_cap = rack_limit_throttle,
            .currthresh_1 = s_ctrl_loop.cores.core[core].plimit_t1,
            .currthresh_2 = s_ctrl_loop.cores.core[core].plimit_t2,
            .currthresh_3 = s_ctrl_loop.cores.core[core].plimit_t3,
        };

        power_set_plimit(p_runconfig, core, plimit_req);

        // log the plimit selection
        // TODO: https://dev.azure.com/AzureCSI/Dev/_queries/edit/1811056
        // power_log_core(core, POWER_LOG_DATA(PLIMIT, {.plimit = plimit, .rack_cap = rack_power_cap}));

        core = corebits_first(&changes);
    }
}

static void power_loops_priority_init(power_runconfig_t* p_runconfig, power_loop_pri_counts_t* counts)
{
    int core_count = corebits_count(&p_runconfig->fuses.valid_cores);
    counts->throt_pri_count[DEFAULT_PRIORITY] = core_count;
    counts->required_nominal_for_throttle[p_runconfig->derived.pnominal][DEFAULT_PRIORITY] = (uint8_t)core_count;
    counts->required_for_boost[p_runconfig->derived.pnominal][DEFAULT_PRIORITY] = (uint8_t)core_count;
}

void power_loops_control_init()
{
    const unsigned core_count = NUM_AP_CORES_PER_DIE;
    // get the runtime configuration
    power_runconfig_t* p_runconfig = power_runconfig_get();

    // initialize the loop context
    power_loops_init_loop(&s_control_loop_context);

    // since we're distributing perf levels, max resources are the total number
    // of perf levels we'd need to raise every core to highest/max perf
    s_ctrl_loop.local.max_resources = PLIMIT_TO_RESOURCES(MIN_PLIMIT) * corebits_count(&p_runconfig->fuses.valid_cores);
    s_ctrl_loop.max_resources = s_ctrl_loop.local.max_resources;

    // similarly, the number of levels we have to raise total to get all cores to nominal
    s_ctrl_loop.curr_resources =
        PLIMIT_TO_RESOURCES(p_runconfig->derived.pnominal) * corebits_count(&p_runconfig->fuses.valid_cores);

    POWER_ET_STATUS_PARAM(POWER_ET_TYPE_INIT_MAX_RESOURCES, s_ctrl_loop.max_resources);
    POWER_LOG_INFO("Power loop.  Max resources: %d  Initial: %d", s_ctrl_loop.max_resources, s_ctrl_loop.curr_resources);

    // initialize the pid resource management
    const pid_config_t pid_config = {
        .kp = (float)p_runconfig->knobs.pid.kpt / 1000,
        .ki = (float)p_runconfig->knobs.pid.kit / 1000,
        .kd = (float)p_runconfig->knobs.pid.kdt / 1000,
        .setpoint_offset = (float)p_runconfig->knobs.pid.setpoint_offset / 1000,
        /* temporary setpoint, will be updated to cpu portion of limit before
           use */
        .setpoint = p_runconfig->derived.soc_maximum_thermal_watts_limit,
        .max = s_ctrl_loop.max_resources,
    };

    // forced pstate init
    if (p_runconfig->knobs.force_pstate < NUM_PSTATES)
    {
        for (unsigned int core = 0; core < core_count; ++core)
        {
            // if pstate is forced, we'll always select max plimit
            s_ctrl_loop.cores.core[core].selected_plimit = MAX_PLIMIT;
        }
    }

    // general per-core initialization
    for (unsigned int core = 0; core < core_count; ++core)
    {
        // initialize core base perf and priorities
        s_ctrl_loop.cores.core[core].current_base_pstate = p_runconfig->derived.pnominal;
        s_ctrl_loop.cores.core[core].current_throt_priority = DEFAULT_PRIORITY;
        s_ctrl_loop.cores.core[core].current_boost_priority = DEFAULT_PRIORITY;
        s_ctrl_loop.cores.core[core].plimit_t1 = DEFAULT_PLIMIT_T1;
        s_ctrl_loop.cores.core[core].plimit_t2 = DEFAULT_PLIMIT_T2;
        s_ctrl_loop.cores.core[core].plimit_t3 = DEFAULT_PLIMIT_T3;
        // if core is valid
        if (corebits_is_bit_set(&p_runconfig->fuses.valid_cores, core))
        {
            // set core bit only for valid cores
            corebits_set_bit(&s_ctrl_loop.throttle_priority[DEFAULT_PRIORITY], core);
            corebits_set_bit(&s_ctrl_loop.boost_priority[DEFAULT_PRIORITY], core);
        }
    }

    // initialize the local priority counts
    power_loops_priority_init(p_runconfig, &s_ctrl_loop.local.pri_counts);

    pid_init(&pid_config);
    // set an initial resource count
    pid_set_resources(s_ctrl_loop.curr_resources);

    // initialize remote die sync
    power_remote_die_init(p_runconfig);
}

// function should be called after core initialization in either cold or warm boot
void power_loops_control_post_core_init()
{
    // we need to sync CPPC state from cores to control loop
    power_hw_capture_cppc_state(message_update_callback);
}

power_ctrl_loop_detail_t* power_ctrl_loop_get()
{
    return &s_ctrl_loop;
}

power_loop_context_t* get_s_loop_context(void)
{
    return &s_control_loop_context;
}

power_ctrl_loop_detail_t* get_s_ctrl_loop(void)
{
    return &s_ctrl_loop;
}
