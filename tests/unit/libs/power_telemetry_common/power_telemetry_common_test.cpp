//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_telemetry_common_test.cpp
 * Unit tests for the power telemetry common library.
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <power_telemetry_common.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

uint16_t __wrap_config_get_power_r_loadline_vcpu0(void)
{
    return mock_type(uint16_t);
}

uint16_t __wrap_config_get_power_r_loadline_vcpu1(void)
{
    return mock_type(uint16_t);
}

uint16_t __wrap_config_get_power_vsys_r_loadline(void)
{
    return mock_type(uint16_t);
}
}

TEST_FUNCTION(power_telemetry_loadline_loss_die0_returns_zero_when_resistance_missing, NULL, NULL)
{
    will_return(__wrap_config_get_power_r_loadline_vcpu0, 0U);
    uint32_t loss_mw = power_telemetry_loadline_loss_die0(POWER_TELEM_VR_DIE0_VCPU0, 1500U); // 15 A
    assert_int_equal(loss_mw, 0U);
}

TEST_FUNCTION(power_telemetry_loadline_loss_die0_vcpu0_scales_with_current, NULL, NULL)
{
    will_return(__wrap_config_get_power_r_loadline_vcpu0, 500U);
    uint32_t loss_mw = power_telemetry_loadline_loss_die0(POWER_TELEM_VR_DIE0_VCPU0, 1000U); // 10 A
    assert_int_equal(loss_mw, 50U);
}

TEST_FUNCTION(power_telemetry_loadline_loss_die1_vcpu1_uses_vcpu1_knob, NULL, NULL)
{
    will_return(__wrap_config_get_power_r_loadline_vcpu1, 750U);
    uint32_t loss_mw = power_telemetry_loadline_loss_die1(POWER_TELEM_VR_DIE1_VCPU1, 2000U); // 20 A
    assert_int_equal(loss_mw, 300U);
}

TEST_FUNCTION(power_telemetry_loadline_loss_die0_vsys_uses_vsys_knob, NULL, NULL)
{
    will_return(__wrap_config_get_power_vsys_r_loadline, 1200U);
    uint32_t loss_mw = power_telemetry_loadline_loss_die0(POWER_TELEM_VR_DIE0_VSYS, 500U); // 5 A
    assert_int_equal(loss_mw, 30U);
}

TEST_FUNCTION(power_telemetry_loadline_loss_die0_other_rails_return_zero, NULL, NULL)
{
    uint32_t loss_mw = power_telemetry_loadline_loss_die0(POWER_TELEM_VR_DIE0_VSOC, 1000U);
    assert_int_equal(loss_mw, 0U);
}
