//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

#include "power_telemetry_common.h"

/*------------- Includes -----------------*/

#include <fpfw_cfg_mgr.h>
#include <limits.h>

/*-- Symbolic Constant Macros (defines) --*/

/* Divides I^2 * R to get milliwatts:
 * I in centi-amps -> (1/0.01)^2 = 100 * 100
 * R in micro-ohms -> 1e-6
 * W in mW -> 1000
 * (cA^2 * uΩ) / 10,000,000 => mW
 */
#define LOADLINE_POWER_DIVISOR (10000000ULL)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

static uint16_t get_loadline_resistance_uohm(power_telemetry_global_rail_id_t rail_id)
{
    switch (rail_id)
    {
    case POWER_TELEM_VR_VCPU0:
        return config_get_power_r_loadline_vcpu0();
    case POWER_TELEM_VR_VCPU1:
        return config_get_power_r_loadline_vcpu1();
    case POWER_TELEM_VR_VSYS:
        return config_get_power_vsys_r_loadline();
    case POWER_TELEM_VR_VPCIE0P75:
    case POWER_TELEM_VR_VDDQ1P1:
    case POWER_TELEM_VR_VSOC:
    case POWER_TELEM_VR_VD2D0P55:
    case POWER_TELEM_VR_VPLL1P2:
    case POWER_TELEM_VR_VGPIO1P2:
    case POWER_TELEM_VR_VD2D0P875:
        return 0U;
    default:
        return 0U;
    }
}

static power_telemetry_global_rail_id_t map_die0_to_global(power_telemetry_rail_id_die0_t rail_id)
{
    switch (rail_id)
    {
    case POWER_TELEM_VR_DIE0_VCPU0:
        return POWER_TELEM_VR_VCPU0;
    case POWER_TELEM_VR_DIE0_VPCIE0P75:
        return POWER_TELEM_VR_VPCIE0P75;
    case POWER_TELEM_VR_DIE0_VSYS:
        return POWER_TELEM_VR_VSYS;
    case POWER_TELEM_VR_DIE0_VDDQ1P1:
        return POWER_TELEM_VR_VDDQ1P1;
    case POWER_TELEM_VR_DIE0_VSOC:
        return POWER_TELEM_VR_VSOC;
    case POWER_TELEM_VR_DIE0_VD2D0P55:
        return POWER_TELEM_VR_VD2D0P55;
    case POWER_TELEM_VR_DIE0_VPLL1P2:
        return POWER_TELEM_VR_VPLL1P2;
    case POWER_TELEM_VR_DIE0_VGPIO1P2:
        return POWER_TELEM_VR_VGPIO1P2;
    default:
        return POWER_TELEM_VR_COUNT;
    }
}

static power_telemetry_global_rail_id_t map_die1_to_global(power_telemetry_rail_id_die1_t rail_id)
{
    switch (rail_id)
    {
    case POWER_TELEM_VR_DIE1_VCPU1:
        return POWER_TELEM_VR_VCPU1;
    case POWER_TELEM_VR_DIE1_VD2D0P875:
        return POWER_TELEM_VR_VD2D0P875;
    default:
        return POWER_TELEM_VR_COUNT;
    }
}

static uint32_t calculate_loadline_loss_mw(uint16_t resistance_uohm, uint16_t current_cA)
{
    if ((resistance_uohm == 0U) || (current_cA == 0U))
    {
        return 0U;
    }

    // Compute I^2 * R using 64-bit arithmetic to avoid overflow
    const uint64_t current_sq = (uint64_t)current_cA * (uint64_t)current_cA;
    const uint64_t numerator = current_sq * (uint64_t)resistance_uohm;
    const uint64_t result = (numerator + (LOADLINE_POWER_DIVISOR / 2ULL)) / LOADLINE_POWER_DIVISOR;

    return (result > UINT32_MAX) ? UINT32_MAX : (uint32_t)result;
}

uint32_t power_telemetry_loadline_loss_die0(power_telemetry_rail_id_die0_t rail_id, uint16_t current_cA)
{
    const power_telemetry_global_rail_id_t global_rail_id = map_die0_to_global(rail_id);
    const uint16_t resistance_uohm = get_loadline_resistance_uohm(global_rail_id);

    return calculate_loadline_loss_mw(resistance_uohm, current_cA);
}

uint32_t power_telemetry_loadline_loss_die1(power_telemetry_rail_id_die1_t rail_id, uint16_t current_cA)
{
    const power_telemetry_global_rail_id_t global_rail_id = map_die1_to_global(rail_id);
    const uint16_t resistance_uohm = get_loadline_resistance_uohm(global_rail_id);

    return calculate_loadline_loss_mw(resistance_uohm, current_cA);
}
