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

#include <CMockaWrapper.h>     // for expect_any_always, CmockaWrapperTest
#include <assert.h>            // for assert
#include <cstddef>             // for NULL, size_t
#include <fpfw_status.h>       // for FPFW_STATUS_INVALID_ARGS, FPFW_STATUS...
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
    // uint32_t* temp = (uint32_t*)target_addr;
    // printf("ACT_FUSE_DATA: 0x%llx TGT_ADDR: 0x%x\n", fuse_data, target_addr);
    memcpy((void*)target_addr, (void*)&fuse_data, fuse_size);
    // printf("TGT_PTR_VAL: 0x%x TGT_PTR: 0x%x\n", *temp, (uint32_t)temp);

    return mock_type(uint64_t);
}

bool __wrap_power_fuses_is_power_hw_supported()
{
    return mock_type(bool);
}

} // extern "C"

//
// Setup functions
//
#ifdef ENABLE_PC_SAMPLE_APIS
static int set_fuse_data_to_pc_valid(void** state)
{
    UNUSED(state);

    fuse_data = CUSTOMER_SAMPLE_MILESTONE_PC;

    return 0;
}
#endif

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

    return 0;
}

static int set_run_config_core_enabled_fuse_zero(void** state)
{
    UNUSED(state);

    fuse_data = 0x0;

    test_power_runconfig.fuses.valid_cores.bits[0] = 0x1;
    test_power_runconfig.fuses.valid_cores.bits[1] = 0x1;
    test_power_runconfig.fuses.valid_cores.bits[2] = 0x1;

    return 0;
}

static int reset_run_config_to_empty(void** state)
{
    UNUSED(state);

    test_power_runconfig = {};

    return 0;
}

static int reset_run_config_to_empty_fuse_default(void** state)
{
    UNUSED(state);

    test_power_runconfig = {};
    fuse_data = FUSE_MOCK_VALUE_FF;

    return 0;
}

//
// Tests
//

POWER_TEST(read_fuse_power_hw_not_supported, NULL, NULL)
{
    power_fuse_data_t test_fuses = {};

    expect_any_always(__wrap_FpFwAssert, expression);

    will_return(__wrap_power_runconfig_get, &test_power_runconfig);
    will_return(__wrap_power_fuses_is_power_hw_supported, false);

    power_fuses_read(&test_fuses);
}

POWER_TEST(read_fuse_power_hw_supported, set_fuse_data_to_default, NULL)
{
    power_fuse_data_t test_fuses = {};

    expect_any_always(__wrap_FpFwAssert, expression);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return(__wrap_power_runconfig_get, &test_power_runconfig);
    will_return(__wrap_power_fuses_is_power_hw_supported, true);
    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    power_fuses_read(&test_fuses);
}

POWER_TEST(read_fuse_power_hw_supported_core_enabled, set_run_config_core_enabled, reset_run_config_to_empty)
{
    power_fuse_data_t test_fuses = {};

    expect_any_always(__wrap_FpFwAssert, expression);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return(__wrap_power_runconfig_get, &test_power_runconfig);
    will_return(__wrap_power_fuses_is_power_hw_supported, true);
    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    power_fuses_read(&test_fuses);
}

POWER_TEST(read_fuse_power_hw_supported_core_enabled_curve_less_than_vft_count,
           set_run_config_core_enabled_fuse_zero,
           reset_run_config_to_empty_fuse_default)
{
    power_fuse_data_t test_fuses = {};

    expect_any_always(__wrap_FpFwAssert, expression);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return(__wrap_power_runconfig_get, &test_power_runconfig);
    will_return(__wrap_power_fuses_is_power_hw_supported, true);
    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    power_fuses_read(&test_fuses);
}

POWER_TEST(load_cap_to_max_no_cap, NULL, NULL)
{
    int32_t ret_val = 0xFFFFFFFF;
    assert_true((ldo_cap_to_max(0x100, 0xFFFFFFFF, 0x0) != ret_val));
}

POWER_TEST(get_pmm_rev_rev_0, set_fuse_data_to_zero, set_fuse_data_to_default)
{
    expect_any_always(__wrap_FpFwAssert, expression);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    assert_true((power_fuses_get_pmm_rev() == FUSE_MOCK_VALUE_ZERO));
}

POWER_TEST(get_dts_coeff_null_arg, set_fuse_data_to_zero, set_fuse_data_to_default)
{
    unsigned k_offset = 0x1;
    unsigned k_width = 0x2;
    unsigned y_offset = 0x3;
    unsigned y_width = 0x4;
    unsigned fuse_elements = 0x5;
    unsigned coeff_count = 0x6;
    unsigned coeff_spacing = 0x7;

    expect_any_always(__wrap_FpFwAssert, expression);

    assert((power_fuses_get_dts_coeff(k_offset, k_width, y_offset, y_width, fuse_elements, coeff_count, coeff_spacing, NULL) ==
            FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(get_dts_coeff_y_fuse_read_fail, set_fuse_data_to_one, set_fuse_data_to_default)
{
    unsigned k_offset = 0x1;
    unsigned k_width = 0x2;
    unsigned y_offset = 0x3;
    unsigned y_width = 0x4;
    unsigned fuse_elements = 0x5;
    unsigned coeff_count = 0x6;
    unsigned coeff_spacing = 0x7;
    dts_coeff_t dts_coeff = {};

    expect_any_always(__wrap_FpFwAssert, expression);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    will_return(__wrap_platform_read_fuse, FPFW_STATUS_INVALID_ARGS);

    assert_true((power_fuses_get_dts_coeff(k_offset, k_width, y_offset, y_width, fuse_elements, coeff_count, coeff_spacing, &dts_coeff) ==
                 FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(get_dts_coeff_y_fuse_read_zero, set_fuse_data_to_zero, set_fuse_data_to_default)
{
    unsigned k_offset = 0x1;
    unsigned k_width = 0x2;
    unsigned y_offset = 0x3;
    unsigned y_width = 0x4;
    unsigned fuse_elements = 0x5;
    unsigned coeff_count = 0x6;
    unsigned coeff_spacing = 0x7;
    dts_coeff_t dts_coeff = {};

    expect_any_always(__wrap_FpFwAssert, expression);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    assert_true((power_fuses_get_dts_coeff(k_offset, k_width, y_offset, y_width, fuse_elements, coeff_count, coeff_spacing, &dts_coeff) ==
                 FPFW_STATUS_SUCCESS));
}

POWER_TEST(get_dts_coeff_k_fuse_read_fail, set_fuse_data_to_one, set_fuse_data_to_default)
{
    unsigned k_offset = 0x1;
    unsigned k_width = 0x2;
    unsigned y_offset = 0x3;
    unsigned y_width = 0x4;
    unsigned fuse_elements = 0x5;
    unsigned coeff_count = 0x6;
    unsigned coeff_spacing = 0x7;
    dts_coeff_t dts_coeff = {};

    expect_any_always(__wrap_FpFwAssert, expression);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    will_return(__wrap_platform_read_fuse, FPFW_STATUS_INVALID_ARGS);

    assert_true((power_fuses_get_dts_coeff(k_offset, k_width, y_offset, y_width, fuse_elements, coeff_count, coeff_spacing, &dts_coeff) ==
                 FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(read_memasst_null_arg, NULL, NULL)
{
    expect_any_always(__wrap_FpFwAssert, expression);

    assert_true((power_fuses_read_memasst(NULL) == (FPFW_STATUS_INVALID_ARGS)));
}

POWER_TEST(read_memasst_fuse_read_fail, NULL, NULL)
{
    dvfs_core_memasst_entries_t entries = {};

    expect_any_always(__wrap_FpFwAssert, expression);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_INVALID_ARGS);

    assert_true((power_fuses_read_memasst(&entries) == FPFW_STATUS_FAIL));
}

POWER_TEST(clear_core_valid_bits_null_args, NULL, NULL)
{
    assert_true((power_fuses_clear_core_valid_bits(NULL) == FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(clear_core_valid_bits_fuse_read_fail, NULL, NULL)
{
    corebits_t valid_bits = {};

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_INVALID_ARGS);

    assert_true((power_fuses_clear_core_valid_bits(&valid_bits) == FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(loadac_to_volage_null_args, NULL, NULL)
{
    assert_true((power_fuses_get_ldodac_to_voltage(NULL) == FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(loadac_to_volage_zero_fuse_data, set_fuse_data_to_zero, set_fuse_data_to_default)
{
    dvfs_vf_slope_t slope_offset = {};

    expect_any_always(__wrap_FpFwAssert, expression);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    power_fuses_get_ldodac_to_voltage(&slope_offset); // This is expected to assert
}

POWER_TEST(loadac_to_volage_fuse_read_fail, NULL, NULL)
{
    dvfs_vf_slope_t slope_offset = {};

    expect_any_always(__wrap_FpFwAssert, expression);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_INVALID_ARGS);

    power_fuses_get_ldodac_to_voltage(&slope_offset); // This called is expected to call assert
}

POWER_TEST(loadac_to_volage_valid_fuse_data, set_fuse_data_to_default, NULL)
{
    dvfs_vf_slope_t slope_offset = {};

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    assert_true((power_fuses_get_ldodac_to_voltage(&slope_offset) == FPFW_STATUS_SUCCESS));
}

POWER_TEST(get_ldo_headroom_null_args, NULL, NULL)
{
    assert_true((power_fuses_get_ldo_headroom(NULL) == FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(get_ldo_headroom_fuse_read_fail, NULL, NULL)
{
    uint8_t ldo_headroom;

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_INVALID_ARGS);

    assert_true((power_fuses_get_ldo_headroom(&ldo_headroom) == FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(vcpu_guardband_null_args, NULL, NULL)
{
    assert_true((power_fuses_get_vcpu_guardband(NULL) == FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(vcpu_guardband_fuse_read_fail, NULL, NULL)
{
    uint8_t vcpu_guardband;

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_INVALID_ARGS);

    assert_true((power_fuses_get_vcpu_guardband(&vcpu_guardband) == FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(vcpu_leakage_null_args, NULL, NULL)
{
    expect_any_always(__wrap_FpFwAssert, expression);

    assert_true((power_fuses_vcpu_leakage(NULL, 0x1) == FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(vcpu_leakage_fuse_read_fail, NULL, NULL)
{
    power_vcpu_interp_t vcpu_leakage[7];
    unsigned count = 0x1;

    expect_any_always(__wrap_FpFwAssert, expression);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_INVALID_ARGS);

    assert_true((power_fuses_vcpu_leakage(vcpu_leakage, count) == FPFW_STATUS_FAIL));
}

POWER_TEST(ldo_dyn_null_args, NULL, NULL)
{
    expect_any_always(__wrap_FpFwAssert, expression);

    assert_true((power_fuses_ldo_dyn(NULL, 0x1) == FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(ldo_dyn_fuse_read_fail, NULL, NULL)
{
    power_vcpu_interp_t vcpu_intercept[7];
    unsigned count = 0x1;

    expect_any_always(__wrap_FpFwAssert, expression);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_INVALID_ARGS);

    assert_true((power_fuses_ldo_dyn(vcpu_intercept, count) == FPFW_STATUS_FAIL));
}

POWER_TEST(core_cdyn_null_args, NULL, NULL)
{
    expect_any_always(__wrap_FpFwAssert, expression);

    assert_true((power_fuses_core_cdyn(NULL, 0x1) == FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(core_cdyn_fuse_read_fail, NULL, NULL)
{
    power_vcpu_interp_t core_cdyn_intercept[7];
    unsigned count = 0x1;

    expect_any_always(__wrap_FpFwAssert, expression);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_INVALID_ARGS);

    assert_true((power_fuses_core_cdyn(core_cdyn_intercept, count) == FPFW_STATUS_FAIL));
}

POWER_TEST(process_id_null_args, NULL, NULL)
{
    assert_true((power_fuses_process_id(NULL) == FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(process_id_fuse_read_fail, NULL, NULL)
{
    power_fuse_process_id_t process_id;

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_INVALID_ARGS);

    assert_true((power_fuses_process_id(&process_id) == FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(get_tdp_config_null_args, NULL, NULL)
{
    assert_true((power_fuses_get_tdp_config(NULL) == FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(get_tdp_config_first_fuse_read_fail, set_fuse_data_to_default, NULL)
{
    power_fuse_tdp_t tdp_config;

    expect_any_always(__wrap_FpFwAssert, expression);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_INVALID_ARGS);

    power_fuses_get_tdp_config(&tdp_config);
}

POWER_TEST(get_tdp_config_second_fuse_read_fail, set_fuse_data_to_default, NULL)
{
    power_fuse_tdp_t tdp_config;

    // expect_any_always(__wrap_FpFwAssert, expression);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    will_return(__wrap_platform_read_fuse, FPFW_STATUS_INVALID_ARGS);

    assert_true((power_fuses_get_tdp_config(&tdp_config) == FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(get_tdp_config_fuse_read_success, set_fuse_data_to_zero, set_fuse_data_to_default)
{
    power_fuse_tdp_t tdp_config;

    expect_any_always(__wrap_FpFwAssert, expression);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    assert_true((power_fuses_get_tdp_config(&tdp_config) == FPFW_STATUS_SUCCESS));
}

POWER_TEST(get_tdp_config_fuse_success, NULL, NULL)
{
    power_fuse_tdp_t tdp_config;

    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    assert_true((power_fuses_get_tdp_config(&tdp_config) == FPFW_STATUS_SUCCESS));
}

POWER_TEST(get_curve_assignment_null_args, NULL, NULL)
{
    unsigned core = 0x1;

    expect_any_always(__wrap_FpFwAssert, expression);
    assert_true((power_fuses_get_curve_assignment(core, NULL) == FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(get_curve_assignment_fuse_read_fail, NULL, NULL)
{
    unsigned core = 0x1;
    unsigned curve_assignment = 0x2;

    expect_any_always(__wrap_FpFwAssert, expression);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_INVALID_ARGS);

    assert_true((power_fuses_get_curve_assignment(core, &curve_assignment) == FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(get_curve_assignment_fuse_read_success, NULL, NULL)
{
    unsigned core = 0x1;
    unsigned curve_assignment = 0x2;

    expect_any_always(__wrap_FpFwAssert, expression);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    assert_true((power_fuses_get_curve_assignment(core, &curve_assignment) == FPFW_STATUS_SUCCESS));
}

POWER_TEST(read_vf_null_args, NULL, NULL)
{
    int8_t ldo_offset = 0x1;

    expect_any_always(__wrap_FpFwAssert, expression);

    assert_true((power_fuses_read_vf(NULL, ldo_offset) == FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(read_vf_zero_first_fuse_read_fail, NULL, NULL)
{
    power_fuse_vf_curveset_t vf;
    int8_t ldo_offset = 0x0;

    expect_any_always(__wrap_FpFwAssert, expression);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_INVALID_ARGS);

    assert_true((power_fuses_read_vf(&vf, ldo_offset) == FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(read_vf_second_fuse_read_fail_non_zero_data, NULL, NULL)
{
    power_fuse_vf_curveset_t vf;
    int8_t ldo_offset = 0x1;

    expect_any_always(__wrap_FpFwAssert, expression);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);
    will_return(__wrap_platform_read_fuse, FPFW_STATUS_INVALID_ARGS);

    assert_true((power_fuses_read_vf(&vf, ldo_offset) == FPFW_STATUS_INVALID_ARGS));
}

POWER_TEST(read_vf_success, set_fuse_data_to_zero, set_fuse_data_to_default)
{
    power_fuse_vf_curveset_t vf;
    int8_t ldo_offset = 0x1;

    expect_any_always(__wrap_FpFwAssert, expression);
    expect_any_always(__wrap_platform_read_fuse, target_addr);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_offset);
    expect_any_always(__wrap_platform_read_fuse, fuse_bit_size);

    will_return_always(__wrap_platform_read_fuse, FPFW_STATUS_SUCCESS);

    assert_true((power_fuses_read_vf(&vf, ldo_offset) == FPFW_STATUS_SUCCESS));
    // printf("complete\n");
}
