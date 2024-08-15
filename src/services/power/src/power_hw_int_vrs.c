//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_hw_int_vrs.c
 * Implements the voltage rail interface of the power service
 */

/*------------- Includes -----------------*/
#include "power_events.h"
#include "power_i.h"
#include "power_loops_i.h"
#include "power_stub_i.h"

#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <debug.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

#define PWR_MW 1000

#define MAX_VR_PER_DIE 8
#define VR_CPU_IDX     0

#define DO_ONLY_ONCE(...)                    \
    do                                       \
    {                                        \
        static bool has_not_occurred = true; \
        if (has_not_occurred)                \
        {                                    \
            __VA_ARGS__;                     \
            has_not_occurred = false;        \
        }                                    \
    } while (0)

/*------------- Typedefs -----------------*/
// context structure for VRs
typedef struct _power_vrs_context
{
    // latest current/temp/voltage here
    power_vrs_avs_latest_t vr_inputs[MAX_VR_PER_DIE];
    power_latest_calcs_t latest_power;

    // store recent power calculations
    float soc_power[SOC_POWER_AVG_COUNT]; /* most recent x soc power measurements,
                                             most recent in 0 index */
    float vcpu_power_log[SOC_POWER_AVG_COUNT];
    float vcpu_current_log[SOC_POWER_AVG_COUNT];
} power_vrs_context_t;

/*-------- Function Prototypes -----------*/
static void calculate_soc_power();

/*-- Declarations (Statics and globals) --*/
static power_vrs_context_t s_power_vrs_ctx = {0};

/*------------- Functions ----------------*/

// TODO: Below is temp implementation ahead of AVS integration (https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491023/)
void power_vrs_initiate_vr_reads()
{
    // temp, somewhat valid-looking data
    s_power_vrs_ctx.vr_inputs[VR_CPU_IDX].voltage = 1000;
    s_power_vrs_ctx.vr_inputs[VR_CPU_IDX].current = 20000;

    calculate_soc_power();

    // store for control loop
    s_power_vrs_ctx.latest_power.soc_power = s_power_vrs_ctx.soc_power[0]; // most recent soc power measurement
    s_power_vrs_ctx.latest_power.vcpu_power = s_power_vrs_ctx.vcpu_power_log[0]; // most recent vcpu power measurement
    s_power_vrs_ctx.latest_power.vcpu_avs_voltage = s_power_vrs_ctx.vr_inputs[VR_CPU_IDX].voltage;
    s_power_vrs_ctx.latest_power.vcpu_avs_current = s_power_vrs_ctx.vr_inputs[VR_CPU_IDX].current;

    power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VR_READ, &s_power_vrs_ctx.latest_power);
    power_loops_vr_telem_handle_event(POWER_VR_TELEM_SIGNAL_VR_CURRENT, s_power_vrs_ctx.vr_inputs);
}

void power_vrs_write_vcpu_voltage(uint16_t voltage_mv)
{
    power_runconfig_t* p_runconfig = power_runconfig_get();

    // any boot VR request changes should have completed

    // TODO: update cpu_idx, since forced index will be difference on die1
    // (https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491058/) skip updating VCPU if forced VCPU was set
    if (p_runconfig->knobs.forced_vrs.vr[VR_CPU_IDX] != 0)
    {
        power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VCPU_DONE, NULL);
        return;
    }

    // want to warn/adjust if calculated voltage is over a maximum, under minimum
    if (voltage_mv > VR_VCPU_MAX_VOLTAGE_MV)
    {
        DO_ONLY_ONCE(POWER_ET_ERROR(POWER_ET_TYPE_VCPU_CALCULATED_OVER_MAX, voltage_mv));
        DO_ONLY_ONCE(POWER_LOG_WARN(" vcpu %dmV > %dmV", voltage_mv, VR_VCPU_MAX_VOLTAGE_MV));
        voltage_mv = VR_VCPU_MAX_VOLTAGE_MV;
    }
    else if (voltage_mv < VR_VCPU_MIN_VOLTAGE_MV)
    {
        DO_ONLY_ONCE(POWER_ET_ERROR(POWER_ET_TYPE_VCPU_CALCULATED_UNDER_MIN, voltage_mv));
        DO_ONLY_ONCE(POWER_LOG_WARN(" vcpu %dmV < %dmV", voltage_mv, VR_VCPU_MIN_VOLTAGE_MV));
        voltage_mv = VR_VCPU_MIN_VOLTAGE_MV;
    }

    // send voltage.. temporarily just finish this
    FPFW_UNUSED(voltage_mv);
    // can signal FAIL, PENDING or DONE based on AVS response
    power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VCPU_PENDING, NULL);
}

void power_vrs_read_vcpu_voltage()
{
    // read voltage.. temporarily just finish this
    // we can update the vcpu voltage once we integrate with AVS, but the primary purpose of this function is to signal DONE or PENDING
    power_loops_control_handle_event(POWER_CTRL_LOOP_SIGNAL_VCPU_DONE, NULL);
}

static void calculate_soc_power()
{
    power_runconfig_t* p_runconfig = power_runconfig_get();

    // initialize with static rail power
    // TODO: Only include static rail on die 0 (https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491058/)
    float soc_power = p_runconfig->knobs.static_rail_power_watts;
    float vcpu_power = 0;
    // TODO: pick right loadline knob for die (https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491058/)
    float r_loadline_ohms = 0.000001F * p_runconfig->knobs.r_loadline_vcpu0_uohm; // loadline knobs in uOhms
    // float vsys_r_loadline_ohms = 0.000001F * p_runconfig->knobs.vsys_r_loadline_uohm;
    float rail_vcpu_current = 0;

    // sum the power of all the VRs; save off VCPU separately
    for (int vr_idx = 0; vr_idx < MAX_VR_PER_DIE; ++vr_idx)
    {
        const float rail_current = AVS_CURRENT_FLOAT(s_power_vrs_ctx.vr_inputs[vr_idx].current);
        float rail_power = rail_current * AVS_VOLTAGE_FLOAT(s_power_vrs_ctx.vr_inputs[vr_idx].voltage);

        // Adjust for loadline need rail/die info
        switch (vr_idx)
        {
        case VR_CPU_IDX:
            rail_power -= (rail_current * rail_current * r_loadline_ohms); // subtract i^2 * r for loadline
            rail_vcpu_current = rail_current;
            break;
        /* TODO: Need config data for AVS rails (https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491023/)
        case MPCL_VR_VSYS:
            rail_power -= (rail_current * rail_current * vsys_r_loadline_ohms);  // subtract i^2 * r for
        loadline break;
        */
        default:
            break;
        }
        soc_power += rail_power;
    }
    // move 0-(x-1) to 1-x  -- leaving index 0 open for new measurement
    memmove(&s_power_vrs_ctx.soc_power[1],
            &s_power_vrs_ctx.soc_power[0],
            sizeof(s_power_vrs_ctx.soc_power[0]) * (SOC_POWER_AVG_COUNT - 1));
    // save most recent measurement to index 0
    s_power_vrs_ctx.soc_power[0] = soc_power;

    // move 0-(x-1) to 1-x  -- leaving index 0 open for new measurement
    memmove(&s_power_vrs_ctx.vcpu_power_log[1],
            &s_power_vrs_ctx.vcpu_power_log[0],
            sizeof(s_power_vrs_ctx.vcpu_power_log[0]) * (SOC_POWER_AVG_COUNT - 1));
    // save most recent measurement to index 0
    s_power_vrs_ctx.vcpu_power_log[0] = vcpu_power;

    memmove(&s_power_vrs_ctx.vcpu_current_log[1],
            &s_power_vrs_ctx.vcpu_current_log[0],
            sizeof(s_power_vrs_ctx.vcpu_current_log[0]) * (SOC_POWER_AVG_COUNT - 1));
    // save most recent measurement to index 0
    s_power_vrs_ctx.vcpu_current_log[0] = rail_vcpu_current;
}

uint32_t power_vrs_get_recent_power_mw()
{
    // TODO: updates for remote die (https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491058/)
    // likely want an API that can be used to pass in the remote power to also log and include in this calculation
    float total = 0.0F;
    for (int meas_idx = 0; meas_idx < SOC_POWER_AVG_COUNT; meas_idx++)
    {
        total += s_power_vrs_ctx.soc_power[meas_idx];
    }
    return (uint32_t)((total / SOC_POWER_AVG_COUNT) * PWR_MW);
}