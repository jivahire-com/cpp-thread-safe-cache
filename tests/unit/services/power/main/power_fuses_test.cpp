//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_fuses_test.cpp
 * Power service fuse tests
 */

/*------------- Includes -----------------*/
#include "power_test.h" // for POWER_TEST

#include <cstddef> // for NULL

extern "C" {
#include "corebits.h"   // for corebits_t
#include "power_test.h" // for POWER_TEST, UNUSED

#include <CMockaWrapper.h> // for expect_any_always, CmockaWrapperTest
#include <assert.h>        // for assert
#include <cstddef>         // for NULL, size_t
#include <fpfw_status.h>   // for FPFW_STATUS_NULL_POINTER, FPFW_STATUS...
#include <idsw_kng.h>
#include <mock_bug_check.h>    // for __wrap_crash_dump_bug_check
#include <power_i.h>           // for power_fuses_get_dts_coeff, power_fuse...
#include <power_runconfig.h>   // for power_fuse_data_t, dts_coeff_t, power...
#include <power_runconfig_i.h> // for power_fuses_read, power_fuses_get_cur...
#include <stdint.h>            // for int8_t, uint64_t, uint8_t, uintptr_t
#include <string.h>            // for memcpy

/*-- Symbolic Constant Macros (defines) --*/

#define CUSTOMER_SAMPLE_MILESTONE_UNKNOWN 0
#define CUSTOMER_SAMPLE_MILESTONE_ES      1
#define CUSTOMER_SAMPLE_MILESTONE_PC      2
#define CUSTOMER_SAMPLE_MILESTONE_PR      3

#define FUSE_MOCK_VALUE_FF   0xFF
#define FUSE_MOCK_VALUE_ZERO 0x0
#define FUSE_MOCK_VALUE_ONE  0x1

#define PC_SAMPLE_MAX_FREQ 3400

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

static power_runconfig_t test_power_runconfig = {};
static uint64_t fuse_data = FUSE_MOCK_VALUE_FF;

/*------------- Functions ----------------*/
//
// Mocks
//

uint64_t __wrap_platform_read_fuse(const uintptr_t target_addr, const unsigned fuse_bit_offset, const unsigned fuse_bit_size)
{
    check_expected_ptr(target_addr);
    check_expected(fuse_bit_offset);
    check_expected(fuse_bit_size);

    size_t fuse_size = ((fuse_bit_size + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE);
    memcpy((void*)target_addr, (void*)&fuse_data, fuse_size);

    return mock_type(uint64_t);
}

} // extern "C"

//
// Setup functions
//

static int set_fuse_data_to_zero(void** state)
{
    UNUSED(state);

    fuse_data = FUSE_MOCK_VALUE_ZERO;

    return 0;
}

static int set_fuse_data_to_one(void** state)
{
    UNUSED(state);

    fuse_data = FUSE_MOCK_VALUE_ONE;

    return 0;
}

static int set_fuse_data_to_default(void** state)
{
    UNUSED(state);

    fuse_data = FUSE_MOCK_VALUE_FF;

    return 0;
}

static int set_run_config_core_enabled(void** state)
{
    UNUSED(state);

    test_power_runconfig.fuses.valid_cores.bits[0] = 0x1;
    test_power_runconfig.fuses.valid_cores.bits[1] = 0x1;
    test_power_runconfig.fuses.valid_cores.bits[2] = 0x1;
    set_fuse_data_to_one(state);

    return 0;
}

static int reset_run_config_to_empty(void** state)
{
    UNUSED(state);

    test_power_runconfig = {};
    set_fuse_data_to_default(state);

    return 0;
}

//
// Tests
//

POWER_TEST(read_fuse_null_fuse_ptr, set_fuse_data_to_default, NULL)
{
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    will_return(__wrap_power_runconfig_get, &test_power_runconfig);

    if (!bugcheck_mock_return())
    {
        power_fuses_read(NULL);
    }
}

POWER_TEST(read_fuse_power_hw_supported, set_fuse_data_to_default, NULL)
{
    power_fuse_data_t test_fuses = {};

    will_return_always(__wrap_idsw_get_platform_sdv,
                       PLATFORM_RVP_EVT_SILICON); // Required since there is a workaround that checks if it SVP

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return(__wrap_power_runconfig_get, &test_power_runconfig);
    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    if (!bugcheck_mock_return())
    {
        power_fuses_read(&test_fuses);
    }
}

POWER_TEST(read_fuse_power_hw_supported_core_enabled, set_run_config_core_enabled, reset_run_config_to_empty)
{
    power_fuse_data_t test_fuses = {};
    will_return_always(__wrap_idsw_get_platform_sdv,
                       PLATFORM_RVP_EVT_SILICON); // Required since there is a workaround that checks if it SVP
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return(__wrap_power_runconfig_get, &test_power_runconfig);
    will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS, -1);

    power_fuses_read(&test_fuses);
}

POWER_TEST(read_fuse_func_failures, set_run_config_core_enabled, reset_run_config_to_empty)
{
    power_fuse_data_t test_fuses = {};
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_power_runconfig_get, &test_power_runconfig);

    if (!bugcheck_mock_return())
    {
        // 1) read_vf -> fail on second call (ldo)
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS, 6);
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER, 1);
        power_fuses_read(&test_fuses);
    }

    if (!bugcheck_mock_return())
    {
        // 2) read_memasst -> fail in first entry (9 reads)
        // reach memasst: read_vf success = 392
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS, 393);
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER, 9);
        power_fuses_read(&test_fuses);
    }

    if (!bugcheck_mock_return())
    {
        // 3) get_ldodac_to_voltage -> fail on first read
        // reach ldo: read_vf 392 + memasst 45 = 437
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS, 437);
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER, 1);
        power_fuses_read(&test_fuses);
    }

    if (!bugcheck_mock_return())
    {
        // 4) get_dts_coeff_tile -> fail on first Y read
        // reach tile Y: 392 + 45 + 2 (ldo) + 1 (pmm_rev) = 440
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS, 440);
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER, 1);
        power_fuses_read(&test_fuses);
    }

    if (!bugcheck_mock_return())
    {
        // 5) get_dts_coeff_soctop -> fail on first Y read
        // reach soctop Y: 440 + 4 (finish tile Y/K for 2 coeffs) + 1 (soctop pmm_rev) = 445
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS, 445);
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER, 1);
        power_fuses_read(&test_fuses);
    }

    if (!bugcheck_mock_return())
    {
        // 6) get_ldo_headroom -> fail on first read
        // soctop full success adds 3 (soctop Y for 2 coeffs; pmm_rev already counted) → 445 + 3 = 448,
        // BUT we fail *before* the ldo_headroom read, so successes before failure = 447.
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS, 449);
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER, 1);
        power_fuses_read(&test_fuses);
    }

    if (!bugcheck_mock_return())
    {
        // 7) get_vcpu_guardband -> fail on first read
        // ldo_headroom success +1 → 448
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS, 450);
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER, 1);
        power_fuses_read(&test_fuses);
    }

    if (!bugcheck_mock_return())
    {
        // 8) vcpu_leakage -> fail in first entry (simulate 4 failing reads)
        // guardband success +1 → 449
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS, 451);
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER, 1);
        power_fuses_read(&test_fuses);
    }

    if (!bugcheck_mock_return())
    {
        // 9) ldo_dyn -> fail in first entry (4 reads)
        // vcpu_leakage full success adds 28 → 449 + 28 = 477
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS, 479);
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER, 1);
        power_fuses_read(&test_fuses);
    }

    if (!bugcheck_mock_return())
    {
        // 10) core_cdyn -> fail in first entry (2 reads)
        // ldo_dyn full success adds 8 → 477 + 8 = 485
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS, 487);
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER, 1);
        power_fuses_read(&test_fuses);
    }

    if (!bugcheck_mock_return())
    {
        // 11) process_id -> fail on first read
        // core_cdyn full success adds 6 → 485 + 6 = 491
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS, 493);
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER, 1);
        power_fuses_read(&test_fuses);
    }

    if (!bugcheck_mock_return())
    {
        // 12) get_tdp_config -> fail on first read
        // process_id success +1 → 492
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS, 494);
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER, 1);
        power_fuses_read(&test_fuses);
    }

    if (!bugcheck_mock_return())
    {
        // 13) get_curve_temp -> fail on first read
        // tdp_config two reads success +2 → 494
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS, 496);
        will_return_count(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER, 1);
        power_fuses_read(&test_fuses);
    }
}

POWER_TEST(load_cap_to_max_no_cap, NULL, NULL)
{
    int32_t ret_val = 0xFFFFFFFF;
    assert_true((ldo_cap_to_max(0x100, 0xFFFFFFFF, 0x0) != ret_val));
}

POWER_TEST(get_pmm_rev_rev_0, set_fuse_data_to_zero, set_fuse_data_to_default)
{
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    if (!bugcheck_mock_return())
    {
        power_fuses_get_pmm_rev();
    }
}

POWER_TEST(get_pmm_rev_fuse_read_fail, set_fuse_data_to_zero, set_fuse_data_to_default)
{
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER);

    if (!bugcheck_mock_return())
    {
        power_fuses_get_pmm_rev();
    }
}

POWER_TEST(get_dts_coeff_null_arg, set_fuse_data_to_zero, set_fuse_data_to_default)
{
    unsigned k_offset = 0x1;
    unsigned k_width = 0x2;
    unsigned y_offset = 0x3;
    unsigned y_width = 0x4;
    unsigned fuse_elements = 0x5;
    unsigned coeff_count = 0x1;
    unsigned coeff_spacing = 0x2;

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    if (!bugcheck_mock_return())
    {
        power_fuses_get_dts_coeff(k_offset, k_width, y_offset, y_width, 0x1, coeff_count, coeff_spacing, NULL);
    }

    if (!bugcheck_mock_return())
    {
        power_fuses_get_dts_coeff(k_offset, 0, y_offset, y_width, fuse_elements, coeff_count, coeff_spacing, NULL);
    }

    if (!bugcheck_mock_return())
    {
        power_fuses_get_dts_coeff(k_offset, k_width, y_offset, 0, fuse_elements, coeff_count, coeff_spacing, NULL);
    }

    if (!bugcheck_mock_return())
    {
        power_fuses_get_dts_coeff(k_offset, k_width, y_offset, y_width, 0, 0, 0, NULL);
    }

    if (!bugcheck_mock_return())
    {
        power_fuses_get_dts_coeff(k_offset, k_width, y_offset, y_width, fuse_elements, coeff_count, coeff_spacing, NULL);
    }
}

POWER_TEST(get_dts_coeff_y_fuse_read_fail, set_fuse_data_to_one, set_fuse_data_to_default)
{
    unsigned k_offset = 0x1;
    unsigned k_width = 0x2;
    unsigned y_offset = 0x3;
    unsigned y_width = 0x4;
    unsigned fuse_elements = 0x5;
    unsigned coeff_count = 0x1;
    unsigned coeff_spacing = 0x2;
    dts_coeff_t dts_coeff = {};

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    will_return(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER);

    assert_true((power_fuses_get_dts_coeff(k_offset, k_width, y_offset, y_width, fuse_elements, coeff_count, coeff_spacing, &dts_coeff) ==
                 FPFW_STATUS_NULL_POINTER));
}

POWER_TEST(get_dts_coeff_y_fuse_read_zero, set_fuse_data_to_zero, set_fuse_data_to_default)
{
    unsigned k_offset = 0x1;
    unsigned k_width = 0x2;
    unsigned y_offset = 0x3;
    unsigned y_width = 0x4;
    unsigned fuse_elements = 0x5;
    unsigned coeff_count = 0x1;
    unsigned coeff_spacing = 0x2;
    dts_coeff_t dts_coeff = {};

    will_return_always(__wrap_idsw_get_platform_sdv,
                       PLATFORM_RVP_EVT_SILICON); // Required since there is a workaround that checks if it SVP

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    if (!bugcheck_mock_return())
    {
        power_fuses_get_dts_coeff(k_offset, k_width, y_offset, y_width, fuse_elements, coeff_count, coeff_spacing, &dts_coeff);
    }
}

POWER_TEST(get_dts_coeff_k_fuse_read_fail, set_fuse_data_to_one, set_fuse_data_to_default)
{
    unsigned k_offset = 0x1;
    unsigned k_width = 0x2;
    unsigned y_offset = 0x3;
    unsigned y_width = 0x4;
    unsigned fuse_elements = 0x5;
    unsigned coeff_count = 0x1;
    unsigned coeff_spacing = 0x2;
    dts_coeff_t dts_coeff = {};

    will_return_always(__wrap_idsw_get_platform_sdv,
                       PLATFORM_RVP_EVT_SILICON); // Required since there is a workaround that checks if it SVP

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    will_return(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER);

    assert_true((power_fuses_get_dts_coeff(k_offset, k_width, y_offset, y_width, fuse_elements, coeff_count, coeff_spacing, &dts_coeff) ==
                 FPFW_STATUS_NULL_POINTER));
}

POWER_TEST(read_memasst_null_arg, NULL, NULL)
{
    assert_true((power_fuses_read_memasst(NULL) == (FPFW_STATUS_NULL_POINTER)));
}

POWER_TEST(read_memasst_fuse_read_fail, NULL, NULL)
{
    dvfs_core_memasst_entries_t entries = {};

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER);

    assert_true((power_fuses_read_memasst(&entries) == FPFW_STATUS_FAIL));
}

POWER_TEST(clear_core_valid_bits_null_args, NULL, NULL)
{
    assert_true((power_fuses_clear_core_valid_bits(NULL) == FPFW_STATUS_NULL_POINTER));
}

POWER_TEST(clear_core_valid_bits_fuse_read_fail, NULL, NULL)
{
    corebits_t valid_bits = {};

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER);

    assert_true((power_fuses_clear_core_valid_bits(&valid_bits) == FPFW_STATUS_NULL_POINTER));
}

POWER_TEST(loadac_to_volage_null_args, NULL, NULL)
{
    assert_true((power_fuses_get_ldodac_to_voltage(NULL) == FPFW_STATUS_NULL_POINTER));
}

POWER_TEST(loadac_to_volage_zero_fuse_data, set_fuse_data_to_zero, set_fuse_data_to_default)
{
    dvfs_vf_slope_t slope_offset = {};

    will_return_always(__wrap_idsw_get_platform_sdv,
                       PLATFORM_RVP_EVT_SILICON); // Required since there is a workaround that checks if it SVP

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    if (!bugcheck_mock_return())
    {
        power_fuses_get_ldodac_to_voltage(&slope_offset); // This is expected to assert
    }
}

POWER_TEST(loadac_to_volage_fuse_read_fail, NULL, NULL)
{
    dvfs_vf_slope_t slope_offset = {};

    will_return_always(__wrap_idsw_get_platform_sdv,
                       PLATFORM_RVP_EVT_SILICON); // Required since there is a workaround that checks if it SVP

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER);

    if (!bugcheck_mock_return())
    {
        power_fuses_get_ldodac_to_voltage(&slope_offset);
    }
}

POWER_TEST(loadac_to_volage_second_fuse_read_fail, NULL, NULL)
{
    dvfs_vf_slope_t slope_offset = {};

    will_return_always(__wrap_idsw_get_platform_sdv,
                       PLATFORM_RVP_EVT_SILICON); // Required since there is a workaround that checks if it SVP

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    will_return(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER);

    assert_true(power_fuses_get_ldodac_to_voltage(&slope_offset) == FPFW_STATUS_NULL_POINTER); // This called is expected to call assert
}

POWER_TEST(loadac_to_volage_valid_fuse_data, set_fuse_data_to_default, NULL)
{
    dvfs_vf_slope_t slope_offset = {};

    will_return_always(__wrap_idsw_get_platform_sdv,
                       PLATFORM_RVP_EVT_SILICON); // Required since there is a workaround that checks if it SVP

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    assert_true((power_fuses_get_ldodac_to_voltage(&slope_offset) == FPFW_STATUS_SUCCESS));
}

POWER_TEST(get_ldo_headroom_null_args, NULL, NULL)
{
    assert_true((power_fuses_get_ldo_headroom(NULL) == FPFW_STATUS_NULL_POINTER));
}

POWER_TEST(get_ldo_headroom_fuse_read_fail, NULL, NULL)
{
    uint8_t ldo_headroom;

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER);

    assert_true((power_fuses_get_ldo_headroom(&ldo_headroom) == FPFW_STATUS_NULL_POINTER));
}

POWER_TEST(vcpu_guardband_null_args, NULL, NULL)
{
    assert_true((power_fuses_get_vcpu_guardband(NULL) == FPFW_STATUS_NULL_POINTER));
}

POWER_TEST(vcpu_guardband_fuse_read_fail, NULL, NULL)
{
    uint8_t vcpu_guardband;

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER);

    assert_true((power_fuses_get_vcpu_guardband(&vcpu_guardband) == FPFW_STATUS_NULL_POINTER));
}

POWER_TEST(ldo_dyn_count_assert, NULL, NULL)
{
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    if (!bugcheck_mock_return())
    {
        power_fuses_ldo_dyn(NULL, 0x1);
    }
}

POWER_TEST(vcpu_leakage_assert_and_null_args, NULL, NULL)
{
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    assert_true((power_fuses_vcpu_leakage(NULL, 0x7) == FPFW_STATUS_NULL_POINTER));
    if (!bugcheck_mock_return())
    {
        power_fuses_vcpu_leakage(NULL, 0x1);
    }
}

POWER_TEST(vcpu_leakage_fuse_read_fail, NULL, NULL)
{
    power_vcpu_interp_t vcpu_leakage[7];
    unsigned count = 0x7;

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER);

    assert_true((power_fuses_vcpu_leakage(vcpu_leakage, count) == FPFW_STATUS_FAIL));
}

POWER_TEST(ldo_dyn_null_args, NULL, NULL)
{
    assert_true((power_fuses_ldo_dyn(NULL, 0x7) == FPFW_STATUS_NULL_POINTER));
}

POWER_TEST(ldo_dyn_fuse_read_fail, NULL, NULL)
{
    power_vcpu_interp_t vcpu_intercept[7];
    unsigned count = 0x2;

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER);

    assert_true((power_fuses_ldo_dyn(vcpu_intercept, count) == FPFW_STATUS_FAIL));
}

POWER_TEST(core_cdyn_null_args, NULL, NULL)
{
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    if (!bugcheck_mock_return())
    {
        power_fuses_core_cdyn(NULL, 0x1);
    }

    if (!bugcheck_mock_return())
    {
        power_fuses_core_cdyn(NULL, 0x7);
    }
}

POWER_TEST(core_cdyn_fuse_read_fail, NULL, NULL)
{
    power_vcpu_interp_t core_cdyn_intercept[7];
    unsigned count = 0x3;

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER);

    assert_true((power_fuses_core_cdyn(core_cdyn_intercept, count) == FPFW_STATUS_FAIL));
}

POWER_TEST(process_id_null_args, NULL, NULL)
{
    assert_true((power_fuses_process_id(NULL) == FPFW_STATUS_NULL_POINTER));
}

POWER_TEST(process_id_fuse_read_fail, NULL, NULL)
{
    power_fuse_process_id_t process_id;

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER);

    assert_true((power_fuses_process_id(&process_id) == FPFW_STATUS_NULL_POINTER));
}

POWER_TEST(get_tdp_config_null_args, NULL, NULL)
{
    assert_true((power_fuses_get_tdp_config(NULL) == FPFW_STATUS_NULL_POINTER));
}

POWER_TEST(get_tdp_config_first_fuse_read_fail, set_fuse_data_to_zero, set_fuse_data_to_default)
{
    power_fuse_tdp_t tdp_config;

    will_return_always(__wrap_idsw_get_platform_sdv,
                       PLATFORM_RVP_EVT_SILICON); // Required since there is a workaround that checks if it SVP

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER);

    if (!bugcheck_mock_return())
    {
        // status check will fail
        power_fuses_get_tdp_config(&tdp_config);
    }
}
POWER_TEST(get_tdp_config_first_fuse_read_fail_success, set_fuse_data_to_zero, set_fuse_data_to_default)
{
    power_fuse_tdp_t tdp_config;

    will_return_always(__wrap_idsw_get_platform_sdv,
                       PLATFORM_RVP_EVT_SILICON); // Required since there is a workaround that checks if it SVP

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    if (!bugcheck_mock_return())
    {
        // fuse value non zero check will fail
        power_fuses_get_tdp_config(&tdp_config);
    }
}

POWER_TEST(get_tdp_config_second_fuse_read_fail, set_fuse_data_to_default, NULL)
{
    power_fuse_tdp_t tdp_config;
    will_return_always(__wrap_idsw_get_platform_sdv,
                       PLATFORM_RVP_EVT_SILICON); // Required since there is a workaround that checks if it SVP

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    will_return(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER);

    assert_true((power_fuses_get_tdp_config(&tdp_config) == FPFW_STATUS_NULL_POINTER));
}

POWER_TEST(get_tdp_config_third_fuse_read_fail, set_fuse_data_to_default, NULL)
{
    power_fuse_tdp_t tdp_config;

    will_return_always(__wrap_idsw_get_platform_sdv,
                       PLATFORM_RVP_EVT_SILICON); // Required since there is a workaround that checks if it SVP

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    assert_true((power_fuses_get_tdp_config(&tdp_config) == FPFW_STATUS_SUCCESS));
}

POWER_TEST(get_tdp_config_fuse_read_success, set_fuse_data_to_one, set_fuse_data_to_default)
{
    power_fuse_tdp_t tdp_config;

    will_return_always(__wrap_idsw_get_platform_sdv,
                       PLATFORM_RVP_EVT_SILICON); // Required since there is a workaround that checks if it SVP

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    assert_true((power_fuses_get_tdp_config(&tdp_config) == FPFW_STATUS_SUCCESS));
}

POWER_TEST(get_tdp_config_fuse_success, NULL, NULL)
{
    power_fuse_tdp_t tdp_config;

    will_return_always(__wrap_idsw_get_platform_sdv,
                       PLATFORM_RVP_EVT_SILICON); // Required since there is a workaround that checks if it SVP

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    assert_true((power_fuses_get_tdp_config(&tdp_config) == FPFW_STATUS_SUCCESS));
}

POWER_TEST(get_curve_assignment_null_args, NULL, NULL)
{
    unsigned core = 0x1;

    assert_true((power_fuses_get_curve_assignment(core, NULL) == FPFW_STATUS_NULL_POINTER));
}

POWER_TEST(get_curve_assignment_fuse_read_fail, NULL, NULL)
{
    unsigned core = 0x40;
    unsigned curve_assignment = 0x2;

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER);

    assert_true((power_fuses_get_curve_assignment(core, &curve_assignment) == FPFW_STATUS_NULL_POINTER));
}

POWER_TEST(get_curve_assignment_fuse_read_success, NULL, NULL)
{
    unsigned core = 0x1;
    unsigned curve_assignment = 0x2;

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    assert_true((power_fuses_get_curve_assignment(core, &curve_assignment) == FPFW_STATUS_SUCCESS));
}

POWER_TEST(read_vf_null_args, NULL, NULL)
{
    int8_t ldo_offset = 0x1;
    bool apply_fuse_override = true;

    assert_true((power_fuses_read_vf(NULL, ldo_offset, apply_fuse_override) == FPFW_STATUS_NULL_POINTER));
}

POWER_TEST(read_vf_zero_first_fuse_read_fail, NULL, NULL)
{
    power_fuse_vf_curveset_t vf;
    int8_t ldo_offset = 0x0;
    bool apply_fuse_override = true;

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER);

    assert_true((power_fuses_read_vf(&vf, ldo_offset, apply_fuse_override) == FPFW_STATUS_NULL_POINTER));
}

POWER_TEST(read_vf_second_fuse_read_fail_non_zero_data, NULL, NULL)
{
    power_fuse_vf_curveset_t vf;
    int8_t ldo_offset = 0x1;
    bool apply_fuse_override = true;

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    will_return(__wrap_platform_read_fuse, FPFW_STATUS_NULL_POINTER);

    assert_true((power_fuses_read_vf(&vf, ldo_offset, apply_fuse_override) == FPFW_STATUS_NULL_POINTER));
}

POWER_TEST(read_vf_success, set_fuse_data_to_zero, set_fuse_data_to_default)
{
    power_fuse_vf_curveset_t vf;
    int8_t ldo_offset = 0x1;
    bool apply_fuse_override = true;

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    assert_true((power_fuses_read_vf(&vf, ldo_offset, apply_fuse_override) == FPFW_STATUS_SUCCESS));
}

POWER_TEST(apply_es1_overrides, set_fuse_data_to_one, set_fuse_data_to_default)
{
    power_fuse_vf_curveset_t vf;
    int8_t ldo_offset = 0x1;
    bool apply_fuse_override = true;

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    assert_true((power_fuses_read_vf(&vf, ldo_offset, apply_fuse_override) == FPFW_STATUS_SUCCESS));
}
POWER_TEST(apply_es1_overrides_additional_pair, set_fuse_data_to_one, set_fuse_data_to_default)
{
#define ES1_CURVSET_OVERRIDE_INDEX    5    // index of curve set to override for es1 samples
#define ES1_CURVSET_OVERRIDE_FREQ_MHZ 3600 // frequency in Mhz to set for es1 override

    power_fuse_vf_curveset_t vf = {};
    int8_t ldo_offset = 0x0;
    bool apply_fuse_override = true;
    static const uint32_t expected_es1_ldo_dac_in_overrides[] = {473, 475, 480, 495};

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    assert_true((power_fuses_read_vf(&vf, ldo_offset, apply_fuse_override) == FPFW_STATUS_SUCCESS));

    for (unsigned curve_idx = 0; curve_idx < VFT_CURVESET_COUNT; ++curve_idx)
    {
        for (unsigned temp_idx = 0; temp_idx < VFT_CURVE_COUNT_PER_CURVESET; ++temp_idx)
        {
            assert_int_equal(vf.curveset[curve_idx].curve[temp_idx].pair[ES1_CURVSET_OVERRIDE_INDEX].freq_Mhz, 3600);
            assert_int_equal(vf.curveset[curve_idx].curve[temp_idx].pair[ES1_CURVSET_OVERRIDE_INDEX].ldo_dac_in,
                             expected_es1_ldo_dac_in_overrides[temp_idx]);
        }
    }
}
POWER_TEST(read_curve_temp_success, set_fuse_data_to_zero, set_fuse_data_to_default)
{
    int8_t curve_max_temp[NUM_DVFS_ITD_TEMPERATURE_LOOKUP_COLUMNS - 1];

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    assert_true((power_fuses_get_curve_temp(curve_max_temp, NUM_DVFS_ITD_TEMPERATURE_LOOKUP_COLUMNS - 1) == FPFW_STATUS_SUCCESS));
}

POWER_TEST(read_curve_temp_size_failure, set_fuse_data_to_zero, set_fuse_data_to_default)
{
    int8_t curve_max_temp[NUM_DVFS_ITD_TEMPERATURE_LOOKUP_COLUMNS - 1];

    assert_true((power_fuses_get_curve_temp(curve_max_temp, 0) == FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(read_curve_temp_null_array, set_fuse_data_to_zero, set_fuse_data_to_default)
{
    assert_true((power_fuses_get_curve_temp(NULL, NUM_DVFS_ITD_TEMPERATURE_LOOKUP_COLUMNS - 1) == FPFW_STATUS_NULL_POINTER));
}

POWER_TEST(read_curve_temp_read_fail, set_fuse_data_to_zero, set_fuse_data_to_default)
{
    int8_t curve_max_temp[NUM_DVFS_ITD_TEMPERATURE_LOOKUP_COLUMNS - 1];

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_FAIL);

    assert_true((power_fuses_get_curve_temp(curve_max_temp, NUM_DVFS_ITD_TEMPERATURE_LOOKUP_COLUMNS - 1) == FPFW_STATUS_FAIL));
}
