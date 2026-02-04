//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_cap.c
 * Implements the power cap functionality.
 *
 * Power cap is persisted to warm start data via power_ws_save_pwr_cap() which
 * is called from power_cap_finalize() whenever the cap changes. On warm boot,
 * the cap is restored via power_cap_update() called from power_ws_recover_pwr_cap().
 */

/*------------- Includes -----------------*/
#include "power_i.h"           // for power_latest_calcs_t, NO_POWER_CAP
#include "power_runconfig.h"   // for power_knobs_t, power_derived_config_t
#include "power_runconfig_i.h" // for power_runconfig_t, power_runconfig_get
#include "power_warmstart_i.h" // for power_ws_save_pwr_cap

#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>  // for FPFW_MIN, FPFW_MAX
#include <stdbool.h>    // for bool
#include <stddef.h>     // for NULL
#include <stdint.h>     // for uint16_t

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static uint16_t s_local_soc_power_cap_watts = NO_POWER_CAP; /* tracks locally the power cap being used on last loop query */
static uint16_t s_requested_soc_power_cap_watts = NO_POWER_CAP; /* tracks locally the power cap being used on last loop query */
static uint16_t s_previous_power_cap_watts; /* store previous cap when updating for diagnostic purpose */
static power_cap_completed_callback_t s_power_cap_callback = NULL;
static float s_inst_max_electrical_limit0 = 0.0F; /* store instantaneous max electrical limit for diagnostic purpose */
static float s_inst_max_electrical_limit1 = 0.0F; /* store instantaneous max electrical limit for diagnostic purpose */
static float s_inst_vrcpu_cap = 0.0F; /* store instantaneous max electrical limit for diagnostic purpose */
/*------------- Functions ----------------*/

float max_electrical_limit(power_runconfig_t* p_runconfig, power_latest_calcs_t* p_local_power, power_latest_calcs_t* p_remote_power)
{
    const float local_vcpu = AVS_VOLTAGE_FLOAT(p_local_power->vcpu_avs_voltage);
    const float remote_vcpu = AVS_VOLTAGE_FLOAT(p_remote_power->vcpu_avs_voltage);
    const float local_icpu = AVS_CURRENT_FLOAT(p_local_power->vcpu_avs_current);
    const float remote_icpu = AVS_CURRENT_FLOAT(p_remote_power->vcpu_avs_current);
    const float cpuin_i0 = p_runconfig->p_sconfig->is_primary_die ? local_icpu : remote_icpu;
    const float cpuin_i1 = p_runconfig->p_sconfig->is_primary_die ? remote_icpu : local_icpu;

    float cpuin_v0 = p_runconfig->p_sconfig->is_primary_die ? local_vcpu : remote_vcpu;
    float cpuin_v1 = p_runconfig->p_sconfig->is_primary_die ? remote_vcpu : local_vcpu;

    float r_loadline_ohms0 = 0.000001F * p_runconfig->knobs.r_loadline_vcpu0_uohm; // r_loadline knob is a value in uOhms
    float r_loadline_ohms1 = 0.000001F * p_runconfig->knobs.r_loadline_vcpu1_uohm; // r_loadline knob is a value in uOhms

    // V_cpuin = V_cpusetpoint - (I_cpu * R_loadline)
    cpuin_v0 -= (cpuin_i0 * r_loadline_ohms0);
    cpuin_v1 -= (cpuin_i1 * r_loadline_ohms1);

    /*
    P_MEL0 = V_cpu0 * I_MEL + V_cpu1 * I_cpu1
    P_MEL1 = V_cpu0 * I_cpu0 + V_cpu1 * I_MEL
    P_cpu_cap = MIN(P_cpu_cap, P_MEL0, P_MEL1)
    */
    float P_MEL0 = (cpuin_v0 * p_runconfig->knobs.soc_maximum_electrical_current_limit_vcpu0) +
                   (cpuin_v1 * cpuin_i1); // soc_maximum_electrical_current_limit knob is in amps
    float P_MEL1 = (cpuin_v0 * cpuin_i0 + (cpuin_v1 * p_runconfig->knobs.soc_maximum_electrical_current_limit_vcpu1)); // soc_maximum_electrical_current_limit knob is in amps

    // store instantaneous max electrical limit for diagnostic purpose
    s_inst_max_electrical_limit0 = P_MEL0;
    s_inst_max_electrical_limit1 = P_MEL1;

    if (!p_runconfig->p_sconfig->platform_is_multi_die)
    {
        //! if this is single die, return the max electrical limit for die 0
        return P_MEL0;
    }
    //! For dual die, return the minimum of the electrical limits for both dies (for the soc) that will be used to cap the vcpu power.
    return FPFW_MIN(P_MEL0, P_MEL1); // soc_maximum_electrical_current_limit
                                     // knob is in amps
}

float power_cap_get_vrcpu_cap(bool* p_new_cap, power_latest_calcs_t* p_local_power, power_latest_calcs_t* p_remote_power)
{
    FPFW_RUNTIME_ASSERT(p_local_power != NULL);
    FPFW_RUNTIME_ASSERT(p_remote_power != NULL);

    power_runconfig_t* p_runconfig = power_runconfig_get();

    float MEL_W = max_electrical_limit(p_runconfig, p_local_power, p_remote_power);
    if (p_new_cap)
    {
        // set new cap to true if previous local cache of cap is none, and there
        // is a new cap
        *p_new_cap = ((s_local_soc_power_cap_watts == NO_POWER_CAP) && (s_requested_soc_power_cap_watts != NO_POWER_CAP));
    }
    // need to track, locally, the last power_cap we used towards cpu cap
    s_local_soc_power_cap_watts = s_requested_soc_power_cap_watts;
    // P_cap = MIN(P_MTL, P_power_cap)
    uint16_t power_cap = FPFW_MIN(s_local_soc_power_cap_watts, p_runconfig->derived.soc_maximum_thermal_watts_limit);

    // calculate CPU portion of power cap, ensuring value isn't less than 0.
    float soc_power_no_cpu = (p_local_power->soc_power + p_remote_power->soc_power) -
                             (p_local_power->vcpu_power + p_remote_power->vcpu_power);
    float cpu_power_cap = FPFW_MAX(0.0F, (float)power_cap - soc_power_no_cpu);

    // now get the min of thermal/rack cap and vcpu-based electrical limit
    cpu_power_cap = FPFW_MIN(cpu_power_cap, MEL_W);
    // store inst vrcpu cap for diagnostic purpose
    s_inst_vrcpu_cap = cpu_power_cap;
    return cpu_power_cap;
}

void power_cap_finalize()
{
    /* if we have a callback to provide success message to, provide that if the
     * current cap matches what the ctrl loop last queried */
    if ((s_power_cap_callback) && (s_requested_soc_power_cap_watts == s_local_soc_power_cap_watts))
    {
        s_power_cap_callback(MP_POWER_CAP_SUCCESS, s_local_soc_power_cap_watts, s_previous_power_cap_watts);
        s_power_cap_callback = NULL;

        // Save to warm start if cap changed (keeps warm start data always up-to-date)
        if (s_previous_power_cap_watts != s_local_soc_power_cap_watts)
        {
            power_ws_save_pwr_cap();
        }
    }
}

int power_cap_update(power_cap_completed_callback_t callback, uint16_t new_power_cap, bool source_is_cli)
{
    // if callback is pending, it means current request still in flight
    if (s_power_cap_callback)
    {
        if (source_is_cli)
        {
            // if new request is from CLI, just fail
            return MP_POWER_CAP_FAIL_CLI_NOT_ALLOWED;
        }
        // if new request is not from CLI, fail in flight and update to new
        // cap
        s_power_cap_callback(MP_POWER_CAP_FAIL_PREVIOUS_UPDATED, new_power_cap, s_requested_soc_power_cap_watts);
    }
    // store off details related to this power cap request
    s_previous_power_cap_watts = s_requested_soc_power_cap_watts;
    s_requested_soc_power_cap_watts = new_power_cap;
    s_power_cap_callback = callback;
    return MP_POWER_CAP_PENDING;
}

int power_cap_cancel(power_cap_completed_callback_t callback, bool source_is_cli)
{
    // internally, just need to set the current cap to none
    return power_cap_update(callback, NO_POWER_CAP, source_is_cli);
}

void power_cap_init()
{
    // initialize global and local power cap detail to none
    s_requested_soc_power_cap_watts = NO_POWER_CAP;
    s_local_soc_power_cap_watts = NO_POWER_CAP;
    s_power_cap_callback = NULL;
}

bool power_cap_is_capped()
{
    return (s_local_soc_power_cap_watts != NO_POWER_CAP);
}

uint16_t get_current_soc_power_cap()
{
    return s_local_soc_power_cap_watts;
}

float get_current_vrcpu_cap()
{
    return s_inst_vrcpu_cap;
}

float get_inst_max_electrical_limit(uint8_t die_num)
{
    if (die_num == 0)
    {
        return s_inst_max_electrical_limit0;
    }
    return s_inst_max_electrical_limit1;
}
