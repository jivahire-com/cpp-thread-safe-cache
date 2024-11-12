//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_runconfig_test.cpp
 * Power service runtime configuration tests
 */

/*------------- Includes -----------------*/
#include "power_test.h" // for POWER_TEST

#include <cstddef> // for NULL
#include <cstdint> // for uintptr_t

extern "C" {

// clang-format off
#include <kng_soc_constants.h> // for NUM_AP_CORES_PER_DIE, NUM_CPU_TILES
// clang-format on
#include <CMockaWrapper.h> // for expect_value, check_expected_ptr, Cmo...
#include <corebits.h>      // for corebits_set_bit
#include <dvfs.h>
#include <dvfs_struct.h>       // for dvfs_vf_slope_t
#include <fpfw_status.h>       // for FPFW_STATUS_SUCCESS
#include <mock_bug_check.h>    // for __wrap_crash_dump_bug_check
#include <power_hw_int_i.h>    // for power_telcfg_t
#include <power_runconfig.h>   // for power_service_config_t
#include <power_runconfig_i.h> // for power_runconfig_t
#include <silibs_common.h>
#include <string.h> // for memcpy_s

/*-- Symbolic Constant Macros (defines) --*/

// defaults for gradient/offset equation for platforms where fuse isn't available
#define DVFS_LDODAC_TO_VOLT_SLOPE  2000
#define DVFS_LDODAC_TO_VOLT_OFFSET 2000
// defaults for dts k/y values for platforms where fuse isn't available
#define PVT_DTS_DEFAULT_K_VAL 286
#define PVT_DTS_DEFAULT_Y_VAL 649
// default for TDP power
#define TDP_DEFAULT_POWER_A 400

// default min plimit
#define TEST_MIN_VALID_PLIMIT 0

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static power_knobs_t s_expected_knobs = {};
static power_fuse_data_t s_expected_fuses = {};
static const corebits_t platform_cores = (corebits_t)COREBITS_INIT_UINT32(0xFFFFFFFF, 0xFFFFFFFF, 0xF);
static power_service_config_t s_test_config = {.platform_cores_in_die = &platform_cores};

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_power_knobs_read(power_knobs_t* p_knobs)
{
    memcpy_s(p_knobs, sizeof(power_knobs_t), mock_ptr_type(power_knobs_t*), sizeof(power_knobs_t));
}

void __wrap_power_fuses_read(power_fuse_data_t* p_fuses)
{
    memcpy_s(p_fuses, sizeof(power_fuse_data_t), mock_ptr_type(power_fuse_data_t*), sizeof(power_fuse_data_t));
}

int __wrap_dvfs_vft_from_fuse_data_per_itd(const dvfs_vf_fused_pairs_t* vf_pairs,
                                           const dvfs_core_memasst_entries_t* core_memasst_entries,
                                           uint8_t* min_valid_plimit,
                                           dvfs_vft_t* vft)
{
    UNUSED(vf_pairs);
    check_expected_ptr(core_memasst_entries);
    *min_valid_plimit = mock_type(uint8_t);
    memcpy_s(vft, sizeof(dvfs_vft_t), mock_ptr_type(dvfs_vft_t*), sizeof(dvfs_vft_t));
    return mock_type(int);
}

} // extern "C"

//
// Helper functions
//

void basic_knobs_setup(power_knobs_t* p_knobs)
{
    // clear data
    memset(p_knobs, 0, sizeof(power_knobs_t));

    // default knobs
}

void basic_fuse_setup(power_fuse_data_t* p_fuses)
{
    // clear data
    memset(p_fuses, 0, sizeof(power_fuse_data_t));

    // set all the valid bits for platform cores; fuses may clear further in
    for (unsigned core_idx = 0; core_idx < NUM_AP_CORES_PER_DIE; ++core_idx)
    {
        corebits_set_bit(&p_fuses->valid_cores, core_idx);
    }

    // default gradient/offset parameters
    const dvfs_vf_slope_t default_ldodac_to_volt = {
        .slope_uvolt = DVFS_LDODAC_TO_VOLT_SLOPE,
        .offset_uvolt = DVFS_LDODAC_TO_VOLT_OFFSET,
    };
    p_fuses->ldodac_to_volt = default_ldodac_to_volt;
    // default dts_coeff parameters
    const dts_coeff_t default_dts_coeff = {
        .k_val = PVT_DTS_DEFAULT_K_VAL,
        .y_val = PVT_DTS_DEFAULT_Y_VAL,
    };
    for (unsigned int idx = 0; idx < ARRAY_SIZE(p_fuses->dts_coeff_tile); ++idx)
    {
        p_fuses->dts_coeff_tile[idx] = default_dts_coeff;
    }
    for (unsigned int idx = 0; idx < ARRAY_SIZE(p_fuses->dts_coeff_soctop); ++idx)
    {
        p_fuses->dts_coeff_soctop[idx] = default_dts_coeff;
    }

    // need to default num cores
    const power_fuse_tdp_t default_tdp_config = {.freq_MHz = DVFS_DEF_NOMINAL_FREQ,

                                                 .power_A = TDP_DEFAULT_POWER_A,
                                                 .num_cores = NUM_AP_CORES_PER_DIE};
    p_fuses->tdp_config = default_tdp_config;
}

static int setup(void** state)
{
    UNUSED(state);

    basic_fuse_setup(&s_expected_fuses);
    basic_knobs_setup(&s_expected_knobs);

    return 0;
}

//
// Tests
//

// shared function for setting a baseline set of runtime config initialization expectations
void set_default_expectations(uint8_t min_plimit)
{

    static const dvfs_vft_t default_vft = DVFS_VFT_DEFAULT_CONFIG;

    will_return(__wrap_power_knobs_read, &s_expected_knobs);
    will_return(__wrap_power_fuses_read, &s_expected_fuses);

    for (unsigned iter = 0; iter < VFT_CURVESET_COUNT * VFT_CURVE_COUNT_PER_CURVESET; ++iter)
    {
        expect_memory(__wrap_dvfs_vft_from_fuse_data_per_itd,
                      core_memasst_entries,
                      &s_expected_fuses.memasst,
                      sizeof(dvfs_core_memasst_entries_t));
        will_return(__wrap_dvfs_vft_from_fuse_data_per_itd, min_plimit);              // min_valid_plimit
        will_return(__wrap_dvfs_vft_from_fuse_data_per_itd, (uintptr_t)&default_vft); // vft
        will_return(__wrap_dvfs_vft_from_fuse_data_per_itd, DVFS_SUCCESS);            // status
    }
}

// ensure static config pointer saved in runconfig init
POWER_TEST(runconfig_sconfig, setup, NULL)
{
    set_default_expectations(TEST_MIN_VALID_PLIMIT);

    power_runconfig_init(&s_test_config);

    assert_int_equal((uintptr_t)&s_test_config, (uintptr_t)power_runconfig_get()->p_sconfig);
}

// ensure happy path for knob reads from runconfig init
POWER_TEST(runconfig_knobs, setup, NULL)
{
    set_default_expectations(TEST_MIN_VALID_PLIMIT);

    power_runconfig_init(&s_test_config);

    assert_memory_equal(&s_expected_knobs, (uintptr_t)&power_runconfig_get()->knobs, sizeof(power_knobs_t));
}

// ensure happy path for fuse reads from runconfig init
POWER_TEST(runconfig_fuses, setup, NULL)
{
    set_default_expectations(TEST_MIN_VALID_PLIMIT);

    power_runconfig_init(&s_test_config);

    assert_memory_equal(&s_expected_fuses, (uintptr_t)&power_runconfig_get()->fuses, sizeof(power_fuse_data_t));
}

// ensure happy path for derived tdp configuration
POWER_TEST(runconfig_derived, setup, NULL)
{
    set_default_expectations(TEST_MIN_VALID_PLIMIT);

    power_runconfig_init(&s_test_config);

    assert_int_equal(power_hw_pstate_from_freq(s_expected_fuses.tdp_config.freq_MHz),
                     power_runconfig_get()->derived.pnominal);
    assert_int_equal(s_expected_fuses.tdp_config.power_A, power_runconfig_get()->derived.soc_maximum_thermal_watts_limit);
}

// ensure knob override of tfp fuses as expected
POWER_TEST(runconfig_derived_tdp_knob_override, setup, NULL)
{
#define TEST_PSTATE_OVERRIDE        2
#define TEST_THERMAL_LIMIT_OVERRIDE 500

    set_default_expectations(TEST_MIN_VALID_PLIMIT);

    s_expected_knobs.nominal_pstate = TEST_PSTATE_OVERRIDE;
    s_expected_knobs.soc_maximum_thermal_watts_limit = TEST_THERMAL_LIMIT_OVERRIDE;

    power_runconfig_init(&s_test_config);

    assert_int_equal(TEST_PSTATE_OVERRIDE, power_runconfig_get()->derived.pnominal);
    assert_int_equal(TEST_THERMAL_LIMIT_OVERRIDE, power_runconfig_get()->derived.soc_maximum_thermal_watts_limit);
}

// ensure core assigned to curve when enabled
POWER_TEST(runconfig_derived_curve_assignment, setup, NULL)
{
    set_default_expectations(TEST_MIN_VALID_PLIMIT);

    power_runconfig_init(&s_test_config);

    for (unsigned core_idx = 0; core_idx < NUM_AP_CORES_PER_DIE; ++core_idx)
    {
        bool found = false;
        for (unsigned curveset_idx = 0; curveset_idx < VFT_CURVESET_COUNT; ++curveset_idx)
        {
            bool core_in_curve =
                corebits_is_bit_set(&power_runconfig_get()->derived.vfts[curveset_idx].assigned_cores, core_idx);
            // ensure core not already found in curve assignment
            assert_false(found && core_in_curve);
            found |= core_in_curve;
        }
        assert_true(found);
    }
}

// ensure core not assigned to any curve if disabled
POWER_TEST(runconfig_derived_curve_assignment_core_disabled, setup, NULL)
{
#define TEST_DISABLED_CORE 5
    set_default_expectations(TEST_MIN_VALID_PLIMIT);

    // disable a core
    corebits_clear_bit(&s_expected_fuses.valid_cores, TEST_DISABLED_CORE);

    power_runconfig_init(&s_test_config);

    for (unsigned core_idx = 0; core_idx < NUM_AP_CORES_PER_DIE; ++core_idx)
    {
        bool found = false;
        for (unsigned curveset_idx = 0; curveset_idx < VFT_CURVESET_COUNT; ++curveset_idx)
        {
            bool core_in_curve =
                corebits_is_bit_set(&power_runconfig_get()->derived.vfts[curveset_idx].assigned_cores, core_idx);
            // ensure core not already found in curve assignment
            assert_false(found && core_in_curve);
            found |= core_in_curve;
        }
        if (core_idx == TEST_DISABLED_CORE)
        {
            assert_false(found);
        }
        else
        {
            assert_true(found);
        }
    }
}

// ensure indexed curve assignment matches fuses
POWER_TEST(runconfig_derived_curve_assignment_curve_assignment, setup, NULL)
{
    set_default_expectations(TEST_MIN_VALID_PLIMIT);

    power_runconfig_init(&s_test_config);

    for (unsigned core_idx = 0; core_idx < NUM_AP_CORES_PER_DIE; ++core_idx)
    {
        unsigned curve_assignment = 0;
        int status = power_fuses_get_curve_assignment(core_idx, &curve_assignment);
        assert_int_equal(curve_assignment, power_runconfig_get()->derived.assigned_vft[core_idx]);
        assert_int_equal(status, FPFW_STATUS_SUCCESS);
    }
}

// ensure default VFT if no valid curve
POWER_TEST(runconfig_default_if_no_valid_curve, setup, NULL)
{
    // min_plimit of NUM_PSTATES indicates fuses didn't result in a valid curve
    set_default_expectations(NUM_PSTATES);

    power_runconfig_init(&s_test_config);

    assert_int_not_equal(power_runconfig_get()->derived.vfts[0].min_plimit, NUM_PSTATES);
}
