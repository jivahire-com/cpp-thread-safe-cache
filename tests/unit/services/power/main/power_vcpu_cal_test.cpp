/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 */

/*------------- Includes -----------------*/

#include "power_test.h"

extern "C" {
#include <CMockaWrapper.h>
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <corebits.h>
#include <mock_dvfs.h>
#include <mock_odcm.h>
#include <mock_pvt.h>
#include <pid_resource.h>
#include <power_i.h>
#include <power_loops_i.h>
#include <power_runconfig.h>
#include <power_runconfig_i.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/
/*-------- Function Prototypes -----------*/

/*------------- Functions ----------------*/

} // extern "C

POWER_TEST(vcpu_calc_max_core_voltage_mv, NULL, NULL)
{
#define FIRST_ASSIGNED_PSTATE 10
#define TEST_CORE_COUNT       8
#define TEST_MAX_VFT_VOLTAGE2 800 // should be unused
#define TEST_MAX_VFT_VOLTAGE  700
#define TEST_VFT_VOLTAGE1     650
#define TEST_VFT_VOLTAGE2     675
#define TEST_VFT_VOLTAGE3     500

    unsigned pstate = FIRST_ASSIGNED_PSTATE;
    uint16_t test_mv[TEST_CORE_COUNT / 2] = {TEST_VFT_VOLTAGE1, TEST_MAX_VFT_VOLTAGE, TEST_VFT_VOLTAGE2, TEST_VFT_VOLTAGE3};
    const unsigned int core_count = TEST_CORE_COUNT;

    power_cores_t p_cores;

    power_service_config_t sconfig = {.platform_die_core_count = TEST_CORE_COUNT};

    power_runconfig_t test_runconfig = {.p_sconfig = &sconfig};

    corebits_clear(&test_runconfig.fuses.valid_cores);

    test_runconfig.knobs.force_pstate = NUM_PSTATES; // Disable forced P-State

    for (unsigned bit_idx = 0; bit_idx < core_count; ++bit_idx)
    {
        power_core_t* core = &p_cores.core[bit_idx];
        if (bit_idx % 2 == 0)
        {
            corebits_set_bit(&test_runconfig.fuses.valid_cores, bit_idx);
            core->selected_plimit = pstate++;
        }
        else
        {
            core->selected_plimit = 0;
        }
    }
    for (unsigned vft_idx = 0; vft_idx < TEST_CORE_COUNT / 2; ++vft_idx)
    {
        test_runconfig.derived.vfts[0].vf[FIRST_ASSIGNED_PSTATE + vft_idx].voltage_mv = test_mv[vft_idx];
    }
    test_runconfig.derived.vfts[0].vf[0].voltage_mv = TEST_MAX_VFT_VOLTAGE2;
    expect_value_count(__wrap_FpFwAssert, expression, true, 2);
    assert_int_equal(power_vcpu_calc_max_core_voltage_mv(&test_runconfig, &p_cores), TEST_MAX_VFT_VOLTAGE);
}

POWER_TEST(vcpu_calc_peak_current_A, NULL, NULL)
{
#define FIRST_ASSIGNED_PSTATE      10
#define PEAK_TEST_CORE_COUNT       8
#define PEAK_TEST_MAX_VFT_VOLTAGE2 800  // should be unused
#define PEAK_TEST_MAX_VFT_FREQ2    4000 // should be unused

#define PEAK_TEST_VFT_VOLTAGE1 850
#define PEAK_TEST_VFT_VOLTAGE2 830
#define PEAK_TEST_VFT_VOLTAGE3 810
#define PEAK_TEST_VFT_VOLTAGE4 800
#define PEAK_TEST_VFT_FREQ1    3500
#define PEAK_TEST_VFT_FREQ2    3450
#define PEAK_TEST_VFT_FREQ3    3400
#define PEAK_TEST_VFT_FREQ4    3350
// calculation from python model
#define PEAK_TEST_ASSUMED_ACTIVITY_FACTOR 80
#define PEAK_TEST_C_DYN_PF                2000
#define PEAK_TEST_ADJ_FACTOR              (1.0f / 100000000000.0f)
#define PEAK_TEST_CALCULATED_VALUE        18.036f
#define PEAK_TEST_CALCULATED_VALUE2       19.040f

#define TEST_POLY_CONSTANT 5.0f

// calculation for plimit current threshold
#define CORE0_PLIMIT1 137
#define CORE0_PLIMIT2 144
#define CORE0_PLIMIT3 152
#define CORE2_PLIMIT1 131
#define CORE2_PLIMIT2 139
#define CORE2_PLIMIT3 146
#define CORE4_PLIMIT1 126
#define CORE4_PLIMIT2 133
#define CORE4_PLIMIT3 140
#define CORE6_PLIMIT1 123
#define CORE6_PLIMIT2 130
#define CORE6_PLIMIT3 137

    uint8_t core_plimits[8][3] = {
        {CORE0_PLIMIT1, CORE0_PLIMIT2, CORE0_PLIMIT3},
        {0, 0, 0},
        {CORE2_PLIMIT1, CORE2_PLIMIT2, CORE2_PLIMIT3},
        {0, 0, 0},
        {CORE4_PLIMIT1, CORE4_PLIMIT2, CORE4_PLIMIT3},
        {0, 0, 0},
        {CORE6_PLIMIT1, CORE6_PLIMIT2, CORE6_PLIMIT3},
        {0, 0, 0},
    };

    unsigned pstate = FIRST_ASSIGNED_PSTATE;

    power_service_config_t sconfig = {.platform_die_core_count = PEAK_TEST_CORE_COUNT};

    power_fuse_data_t f_config = {.process_id = (power_fuse_process_id_t)PROCESS_FF};

    power_knobs_t knobcfg = {
        .current_threshold.t1_percent = 90,
        .current_threshold.t2_percent = 95,
        .current_threshold.t3_percent = 100,
        .leakage_temp_scaler.poly_coefficients[PROCESS_FF - 1] =
            (power_leakage_poly_t){.a = 0, .b = 0, .c = 0, .d = TEST_POLY_CONSTANT},
        .force_pstate = 32,
    };

    power_runconfig_t test_runconfig = {.knobs = knobcfg, .fuses = f_config, .p_sconfig = &sconfig};
    power_ctrl_loop_detail_t* test_loop_config;
    const unsigned int core_count = PEAK_TEST_CORE_COUNT;

    power_ctrl_loop_detail_t loop_config = {.current_vcpu = 0};

    for (unsigned bit_idx = 0; bit_idx < core_count; ++bit_idx)
    {
        loop_config.cores.core[bit_idx] = {.temperature_dC = 85};
    }

    test_loop_config = &loop_config;

    uint16_t test_mv[PEAK_TEST_CORE_COUNT / 2] = {PEAK_TEST_VFT_VOLTAGE1, PEAK_TEST_VFT_VOLTAGE2, PEAK_TEST_VFT_VOLTAGE3, PEAK_TEST_VFT_VOLTAGE4};
    uint16_t test_freq[PEAK_TEST_CORE_COUNT / 2] = {PEAK_TEST_VFT_FREQ1, PEAK_TEST_VFT_FREQ2, PEAK_TEST_VFT_FREQ3, PEAK_TEST_VFT_FREQ4};

    corebits_clear(&test_runconfig.fuses.valid_cores);

    for (unsigned bit_idx = 0; bit_idx < core_count; ++bit_idx)
    {

        power_core_t* core = &test_loop_config->cores.core[bit_idx];

        if (bit_idx % 2 == 0)
        {
            corebits_set_bit(&test_runconfig.fuses.valid_cores, bit_idx);
            core->selected_plimit = pstate++;
        }
        else
        {
            core->selected_plimit = 0;
        }
    }

    for (unsigned vft_idx = 0; vft_idx < PEAK_TEST_CORE_COUNT / 2; ++vft_idx)
    {

        test_runconfig.precalculated_current.curveset[0].vf[FIRST_ASSIGNED_PSTATE + vft_idx].dynamic =
            (float)test_mv[vft_idx] * (float)test_freq[vft_idx] * (float)PEAK_TEST_C_DYN_PF *
            (float)PEAK_TEST_ASSUMED_ACTIVITY_FACTOR * PEAK_TEST_ADJ_FACTOR;
        test_runconfig.precalculated_current.curveset[0].vf[FIRST_ASSIGNED_PSTATE + vft_idx].leakage = 0;
    }

    expect_value_count(__wrap_FpFwAssert, expression, true, 5);

    assert_float_equal(power_vcpu_calc_peak_current_A(&test_runconfig, test_loop_config), PEAK_TEST_CALCULATED_VALUE, 0.01);

    for (unsigned bit_idx = 0; bit_idx < core_count; ++bit_idx)
    {

        power_core_t* core = &test_loop_config->cores.core[bit_idx];

        if (corebits_is_bit_set(&test_runconfig.fuses.valid_cores, bit_idx))
        {
            assert_int_equal(core_plimits[bit_idx][0], core->plimit_t1);
            assert_int_equal(core_plimits[bit_idx][1], core->plimit_t2);
            assert_int_equal(core_plimits[bit_idx][2], core->plimit_t3);
            printf("Core %d, PLIMIT T1 = %d, T2 = %d, T3 = %d\n", bit_idx, core->plimit_t1, core->plimit_t2, core->plimit_t3);
        }
    }

    // all cores are forced to the same pstate (previous implementation allowed specific cores to be forced to different pstates)
    test_runconfig.knobs.force_pstate = FIRST_ASSIGNED_PSTATE;

    expect_value_count(__wrap_FpFwAssert, expression, true, 5);
    assert_float_equal(power_vcpu_calc_peak_current_A(&test_runconfig, test_loop_config), PEAK_TEST_CALCULATED_VALUE2, 0.01);
}

POWER_TEST(power_vcpu_calc_core_leakage_scaler, NULL, NULL)
{
#define TEST_POLY_CONSTANT 5.0f

    // Set process_id
    power_fuse_data_t sconfig = {.process_id = (power_fuse_process_id_t)PROCESS_FF};

    // then provide a constant for the scaler

    power_knobs_t knobcfg = {.leakage_temp_scaler.poly_coefficients[PROCESS_FF - 1] =
                                 (power_leakage_poly_t){.a = 0, .b = 0, .c = 0, .d = TEST_POLY_CONSTANT}};

    power_runconfig_t test_runconfig = {.knobs = knobcfg, .fuses = sconfig};

    expect_value(__wrap_FpFwAssert, expression, true);
    // verify correct scaler is used
    assert_float_equal(power_vcpu_calc_core_leakage_scaler(&test_runconfig, 85), TEST_POLY_CONSTANT, 0.01);
}

/* provide detail for next test*/
static const power_vcpu_interp_t default_leakage[] = {
    {.current = {.ldo_dac = 340, .current_ma = 100, .vcpu_mv = 1050, .temp_offset = 213}},
    {.current = {.ldo_dac = 375, .current_ma = 150, .vcpu_mv = 1050, .temp_offset = 213}},
    {.current = {.ldo_dac = 400, .current_ma = 200, .vcpu_mv = 1050, .temp_offset = 213}},
    {.current = {.ldo_dac = 425, .current_ma = 250, .vcpu_mv = 1050, .temp_offset = 213}},
    {.current = {.ldo_dac = 450, .current_ma = 300, .vcpu_mv = 1050, .temp_offset = 213}},
    {.current = {.ldo_dac = 460, .current_ma = 400, .vcpu_mv = 1050, .temp_offset = 213}},
    {.current = {.ldo_dac = 480, .current_ma = 500, .vcpu_mv = 1050, .temp_offset = 213}},
};
static const power_vcpu_interp_t default_core_cdyn[] = {
    {.cdyn =
         {
             .ldo_dac = 340,
             .cdyn_pf = 1300,
         }},
    {.cdyn =
         {
             .ldo_dac = 410,
             .cdyn_pf = 1200,
         }},
    {.cdyn =
         {
             .ldo_dac = 480,
             .cdyn_pf = 1100,
         }},
};

POWER_TEST(power_vcpu_interpolate_from_points, NULL, NULL)
{
// above the topmost point, slope should be from two previous points
#define INTERPOLATE_TEST1_INPUT  490
#define INTERPOLATE_TEST1_OUTPUT 550

// below the bottom point, value should stay constant
#define INTERPOLATE_TEST2_INPUT  335 // bottom point is 340
#define INTERPOLATE_TEST2_OUTPUT 100 // matches bottom point

// between two points, with rounding up
#define INTERPOLATE_TEST3_INPUT  351 // between 375 and 340, slope will be 50/35
#define INTERPOLATE_TEST3_OUTPUT 116 // (100 + 50/35 * 11)

#define INTERPOLATE_TEST4_INPUT 445
#define INTERPOLATE_TEST4_OUTPUT \
    1150 // not sure if value can be decreasing (negative slope), but most robust would be to allow it if that's what tester fuses

    expect_value_count(__wrap_FpFwAssert, expression, true, 9);

    assert_int_equal(INTERPOLATE_TEST1_OUTPUT,
                     power_vcpu_interpolate_from_points(default_leakage, DIMOF(default_leakage), INTERPOLATE_TEST1_INPUT));

    expect_value_count(__wrap_FpFwAssert, expression, true, 9);

    assert_int_equal(INTERPOLATE_TEST2_OUTPUT,
                     power_vcpu_interpolate_from_points(default_leakage, DIMOF(default_leakage), INTERPOLATE_TEST2_INPUT));

    expect_value_count(__wrap_FpFwAssert, expression, true, 7);

    assert_int_equal(INTERPOLATE_TEST3_OUTPUT,
                     power_vcpu_interpolate_from_points(default_leakage, DIMOF(default_leakage), INTERPOLATE_TEST3_INPUT));

    expect_value_count(__wrap_FpFwAssert, expression, true, 7);

    assert_int_equal(INTERPOLATE_TEST4_OUTPUT,
                     power_vcpu_interpolate_from_points(default_core_cdyn, DIMOF(default_core_cdyn), INTERPOLATE_TEST4_INPUT));
}
