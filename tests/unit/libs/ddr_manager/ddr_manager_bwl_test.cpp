//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_bwl_test.cpp
 * DDR Manager BWL tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstddef>         // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include "ddr_mocks.h"

#include <FpFwUtils.h>
#include <ddr_manager_bwl.h>
} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
bool in_setup_teardown = false;

/*------------- Functions ----------------*/
//
// Fixtures
//
static int setup_disengaged_all(void** state)
{
    in_setup_teardown = true;
    FPFW_UNUSED(state);
    will_return_maybe(__wrap_idsw_get_die_id, DIE_0);
    will_return_maybe(__wrap_ddrss_bandwidth_limiter_config, SILIBS_SUCCESS);

    ddr_manager_disable_bwl_i3c();
    ddr_manager_disable_bwl_force();
    ddr_manager_disable_bwl_mr4();

    in_setup_teardown = false;
    return 0;
}

static int setup_engaged_i3c(void** state)
{
    in_setup_teardown = true;
    FPFW_UNUSED(state);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_maybe(__wrap_ddrss_bandwidth_limiter_config, SILIBS_SUCCESS);

    ddr_manager_enable_bwl_i3c();

    in_setup_teardown = false;
    return 0;
}

static int setup_engaged_force(void** state)
{
    in_setup_teardown = true;
    FPFW_UNUSED(state);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_maybe(__wrap_ddrss_bandwidth_limiter_config, SILIBS_SUCCESS);

    ddr_manager_enable_bwl_force();

    in_setup_teardown = false;
    return 0;
}

static int setup_engaged_mr4(void** state)
{
    in_setup_teardown = true;
    FPFW_UNUSED(state);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_maybe(__wrap_ddrss_bandwidth_limiter_config, SILIBS_SUCCESS);

    ddr_manager_enable_bwl_mr4();

    in_setup_teardown = false;
    return 0;
}

//
// Mocks
//

//
// Tests
//
TEST_FUNCTION(test_ddr_manager_enable_bwl_i3c, setup_disengaged_all, setup_disengaged_all)
{
    // Arrange
    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_write32);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return_count(__wrap_ddrss_bandwidth_limiter_config, SILIBS_SUCCESS, DDRSS_MAX_SS_NUM);

    // Act
    ddr_manager_enable_bwl_i3c();

    // Assert
    assert_true(ddr_manager_get_bwl_engaged());
    assert_int_equal(ddr_manager_get_bwl_state(), DIMM_THROTTLE_SOURCE_EXT_TEMP_SENSOR);
}

TEST_FUNCTION(test_ddr_manager_disable_bwl_i3c, setup_engaged_i3c, setup_disengaged_all)
{
    assert_true(ddr_manager_get_bwl_engaged());

    // Arrange
    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_write32);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return_count(__wrap_ddrss_bandwidth_limiter_config, SILIBS_SUCCESS, DDRSS_MAX_SS_NUM);

    // Act
    ddr_manager_disable_bwl_i3c();

    // Assert
    assert_false(ddr_manager_get_bwl_engaged());
    assert_int_equal(ddr_manager_get_bwl_state(), DIMM_THROTTLE_SOURCE_NONE);
}

TEST_FUNCTION(test_ddr_manager_enable_bwl_mr4, setup_disengaged_all, setup_disengaged_all)
{
    // Arrange
    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_write32);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return_count(__wrap_ddrss_bandwidth_limiter_config, SILIBS_SUCCESS, DDRSS_MAX_SS_NUM);

    // Act
    ddr_manager_enable_bwl_mr4();

    // Assert
    assert_true(ddr_manager_get_bwl_engaged());
    assert_int_equal(ddr_manager_get_bwl_state(), DIMM_THROTTLE_SOURCE_MR4);
}

TEST_FUNCTION(test_ddr_manager_disable_bwl_mr4, setup_engaged_mr4, setup_disengaged_all)
{
    // Arrange
    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_write32);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return_count(__wrap_ddrss_bandwidth_limiter_config, SILIBS_SUCCESS, DDRSS_MAX_SS_NUM);

    // Act
    ddr_manager_disable_bwl_mr4();

    // Assert
    assert_false(ddr_manager_get_bwl_engaged());
    assert_int_equal(ddr_manager_get_bwl_state(), DIMM_THROTTLE_SOURCE_NONE);
}

TEST_FUNCTION(test_ddr_manager_enable_bwl_force, setup_disengaged_all, setup_disengaged_all)
{
    // Arrange
    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_write32);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return_count(__wrap_ddrss_bandwidth_limiter_config, SILIBS_SUCCESS, DDRSS_MAX_SS_NUM);

    // Act
    ddr_manager_enable_bwl_force();

    // Assert
    assert_true(ddr_manager_get_bwl_engaged());
    assert_int_equal(ddr_manager_get_bwl_state(), DIMM_THROTTLE_SOURCE_FORCED_CLI);
}

TEST_FUNCTION(test_ddr_manager_disable_bwl_force, setup_engaged_force, setup_disengaged_all)
{
    // Arrange
    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_write32);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return_count(__wrap_ddrss_bandwidth_limiter_config, SILIBS_SUCCESS, DDRSS_MAX_SS_NUM);

    // Act
    ddr_manager_disable_bwl_force();

    // Assert
    assert_false(ddr_manager_get_bwl_engaged());
    assert_int_equal(ddr_manager_get_bwl_state(), DIMM_THROTTLE_SOURCE_NONE);
}

TEST_FUNCTION(test_ddr_manager_enable_i3c_bwl_enable_mr4_disable_mr4, setup_disengaged_all, setup_disengaged_all)
{
    // Arrange: bwl stays engaged & thus, there is only one set of read/write
    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_write32);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_count(__wrap_ddrss_bandwidth_limiter_config, SILIBS_SUCCESS, DDRSS_MAX_SS_NUM);

    // Act & Assert
    ddr_manager_enable_bwl_i3c();
    ddr_manager_enable_bwl_mr4();
    assert_int_equal(ddr_manager_get_bwl_state(), DIMM_THROTTLE_SOURCE_EXT_TEMP_SENSOR | DIMM_THROTTLE_SOURCE_MR4);

    ddr_manager_disable_bwl_mr4();

    // Assert
    assert_true(ddr_manager_get_bwl_engaged());
    assert_int_equal(ddr_manager_get_bwl_state(), DIMM_THROTTLE_SOURCE_EXT_TEMP_SENSOR);
}

TEST_FUNCTION(test_ddr_manager_enable_mr4_bwl_enable_i3c_disable_mr4, setup_disengaged_all, setup_disengaged_all)
{
    // Arrange enable_bwl_mr4()
    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_write32);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_count(__wrap_ddrss_bandwidth_limiter_config, SILIBS_SUCCESS, DDRSS_MAX_SS_NUM);

    // enable_bwl_i3c won't touch GPIO since bwl is already engaged
    // Arrange disable_bwl_mr4()
    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_write32);
    will_return_count(__wrap_ddrss_bandwidth_limiter_config, SILIBS_SUCCESS, DDRSS_MAX_SS_NUM);

    // Act
    ddr_manager_enable_bwl_mr4();
    ddr_manager_enable_bwl_i3c();
    assert_int_equal(ddr_manager_get_bwl_state(), DIMM_THROTTLE_SOURCE_EXT_TEMP_SENSOR | DIMM_THROTTLE_SOURCE_MR4);
    ddr_manager_disable_bwl_mr4();

    // Assert
    assert_true(ddr_manager_get_bwl_engaged());
    assert_int_equal(ddr_manager_get_bwl_state(), DIMM_THROTTLE_SOURCE_EXT_TEMP_SENSOR);

    // This one does not touch GPIOs since BWL is already disengaged
    ddr_manager_disable_bwl_i3c();

    assert_false(ddr_manager_get_bwl_engaged());
    assert_int_equal(ddr_manager_get_bwl_state(), DIMM_THROTTLE_SOURCE_NONE);
}

TEST_FUNCTION(test_ddr_manager_enable_mr4_bwl_enable_i3c_disable_i3c, setup_disengaged_all, setup_disengaged_all)
{
    // Arrange enable_bwl_mr4()
    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_write32);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_count(__wrap_ddrss_bandwidth_limiter_config, SILIBS_SUCCESS, DDRSS_MAX_SS_NUM);

    // enable_bwl_i3c and disable_bwl_i3c won't touch GPIO since bwl is already engaged and still engaged due
    // to mr4 reason Arrange disable_bwl_mr4()

    // Arrange disable_bwl_mr4()
    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_write32);
    will_return_count(__wrap_ddrss_bandwidth_limiter_config, SILIBS_SUCCESS, DDRSS_MAX_SS_NUM);

    // Act
    ddr_manager_enable_bwl_mr4();
    ddr_manager_enable_bwl_i3c();
    ddr_manager_disable_bwl_i3c();

    // Assert
    assert_true(ddr_manager_get_bwl_engaged());
    assert_int_equal(ddr_manager_get_bwl_state(), DIMM_THROTTLE_SOURCE_MR4);

    ddr_manager_disable_bwl_mr4();

    assert_false(ddr_manager_get_bwl_engaged());
    assert_int_equal(ddr_manager_get_bwl_state(), DIMM_THROTTLE_SOURCE_NONE);
}

TEST_FUNCTION(test_ddr_manager_enable_i3c_force_disable_i3c, setup_disengaged_all, setup_disengaged_all)
{
    // Arrange enable_bwl_i3c()
    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_write32);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_count(__wrap_ddrss_bandwidth_limiter_config, SILIBS_SUCCESS, DDRSS_MAX_SS_NUM);

    // enable_bwl_force won't touch GPIO since bwl is already engaged
    // disable_bwl_i3c won't touch GPIO since bwl is still engaged due to force reason

    // Arrange disable_bwl_force()
    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_write32);
    will_return_count(__wrap_ddrss_bandwidth_limiter_config, SILIBS_SUCCESS, DDRSS_MAX_SS_NUM);

    // Act
    ddr_manager_enable_bwl_i3c();
    ddr_manager_enable_bwl_force();
    assert_int_equal(ddr_manager_get_bwl_state(), DIMM_THROTTLE_SOURCE_EXT_TEMP_SENSOR | DIMM_THROTTLE_SOURCE_FORCED_CLI);
    ddr_manager_disable_bwl_i3c();

    // Assert
    assert_true(ddr_manager_get_bwl_engaged());
    assert_int_equal(ddr_manager_get_bwl_state(), DIMM_THROTTLE_SOURCE_FORCED_CLI);

    ddr_manager_disable_bwl_force();

    assert_false(ddr_manager_get_bwl_engaged());
    assert_int_equal(ddr_manager_get_bwl_state(), DIMM_THROTTLE_SOURCE_NONE);
}