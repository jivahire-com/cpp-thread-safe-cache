//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_telemetry_common.h
 * Utilities shared across power telemetry implementations.
 */

#pragma once

/*----------- Nested includes ------------*/

#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef enum power_telemetry_global_rail_id
{
    POWER_TELEM_VR_VCPU0 = 0,
    POWER_TELEM_VR_VPCIE0P75,
    POWER_TELEM_VR_VSYS,
    POWER_TELEM_VR_VDDQ1P1,
    POWER_TELEM_VR_VSOC,
    POWER_TELEM_VR_VD2D0P55,
    POWER_TELEM_VR_VPLL1P2,
    POWER_TELEM_VR_VGPIO1P2,
    POWER_TELEM_VR_VCPU1,
    POWER_TELEM_VR_VD2D0P875,
    POWER_TELEM_VR_COUNT,
} power_telemetry_global_rail_id_t;

/**
 * Identifies the voltage rails associated with die0
 */
typedef enum power_telemetry_rail_id_die0
{
    POWER_TELEM_VR_DIE0_VCPU0 = 0,
    POWER_TELEM_VR_DIE0_VPCIE0P75,
    POWER_TELEM_VR_DIE0_VSYS,
    POWER_TELEM_VR_DIE0_VDDQ1P1,
    POWER_TELEM_VR_DIE0_VSOC,
    POWER_TELEM_VR_DIE0_VD2D0P55,
    POWER_TELEM_VR_DIE0_VPLL1P2,
    POWER_TELEM_VR_DIE0_VGPIO1P2,
    POWER_TELEM_VR_DIE0_COUNT,
} power_telemetry_rail_id_die0_t;

/**
 * Identifies the voltage rails associated with die1
 */
typedef enum power_telemetry_rail_id_die1
{
    POWER_TELEM_VR_DIE1_VCPU1 = 0,
    POWER_TELEM_VR_DIE1_VD2D0P875,
    POWER_TELEM_VR_DIE1_COUNT,
} power_telemetry_rail_id_die1_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * Calculates the loadline loss for a rail using the configured loadline resistance.
 *
 * @param rail_id       Voltage rail identifier.
 * @param current_cA    Measured rail current in centi-amps (0.01 A).
 * @return The loadline loss in milliwatts. Returns 0 when inputs are invalid or the rail
 *         identifier does not map to a configured loadline.
 */
uint32_t power_telemetry_loadline_loss_die0(power_telemetry_rail_id_die0_t rail_id,
                                       uint16_t current_cA);

/**
 * Calculates the loadline loss for a rail using the configured loadline resistance.
 * 
 * @param rail_id       Voltage rail identifier.
 * @param current_cA    Measured rail current in centi-amps (0.01 A).
 * @return The loadline loss in milliwatts. Returns 0 when inputs are invalid or the rail
 *        identifier does not map to a configured loadline.
 */
uint32_t power_telemetry_loadline_loss_die1(power_telemetry_rail_id_die1_t rail_id,
                                       uint16_t current_cA);
