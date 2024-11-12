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
#include <FpFwUtils.h>
#include <ddr_manager_bwl.h>
} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Fixtures
//
static int setup(void** state)
{
    FPFW_UNUSED(state);

    ddr_manager_disable_bwl_i3c();
    ddr_manager_disable_bwl_force();
    ddr_manager_disable_bwl_mr4();

    return 0;
}

//
// Mocks
//

//
// Tests
//
TEST_FUNCTION(test_ddr_manager_enable_bwl_i3c, setup, NULL)
{
    // Arrange

    // Act
    ddr_manager_enable_bwl_i3c();

    // Assert
    assert_true(ddr_manager_get_bwl_engaged());
}

TEST_FUNCTION(test_ddr_manager_disable_bwl_i3c, setup, NULL)
{
    // Arrange

    // Act
    ddr_manager_disable_bwl_i3c();

    // Assert
    assert_false(ddr_manager_get_bwl_engaged());
}

TEST_FUNCTION(test_ddr_manager_enable_bwl_mr4, setup, NULL)
{
    // Arrange

    // Act
    ddr_manager_enable_bwl_mr4();

    // Assert
    assert_true(ddr_manager_get_bwl_engaged());
}

TEST_FUNCTION(test_ddr_manager_disable_bwl_mr4, setup, NULL)
{
    // Arrange

    // Act
    ddr_manager_disable_bwl_mr4();

    // Assert
    assert_false(ddr_manager_get_bwl_engaged());
}

TEST_FUNCTION(test_ddr_manager_enable_bwl_force, setup, NULL)
{
    // Arrange

    // Act
    ddr_manager_enable_bwl_force();

    // Assert
    assert_true(ddr_manager_get_bwl_engaged());
}

TEST_FUNCTION(test_ddr_manager_disable_bwl_force, setup, NULL)
{
    // Arrange

    // Act
    ddr_manager_disable_bwl_force();

    // Assert
    assert_false(ddr_manager_get_bwl_engaged());
}

TEST_FUNCTION(test_ddr_manager_enable_i3c_bwl_enable_mr4_disable_mr4, setup, NULL)
{
    // Arrange

    // Act
    ddr_manager_enable_bwl_i3c();
    ddr_manager_enable_bwl_mr4();
    ddr_manager_disable_bwl_mr4();

    // Assert
    assert_true(ddr_manager_get_bwl_engaged());
}

TEST_FUNCTION(test_ddr_manager_enable_mr4_bwl_enable_i3c_disable_mr4, setup, NULL)
{
    // Arrange

    // Act
    ddr_manager_enable_bwl_mr4();
    ddr_manager_enable_bwl_i3c();
    ddr_manager_disable_bwl_mr4();

    // Assert
    assert_true(ddr_manager_get_bwl_engaged());

    ddr_manager_disable_bwl_i3c();

    assert_false(ddr_manager_get_bwl_engaged());
}

TEST_FUNCTION(test_ddr_manager_enable_mr4_bwl_enable_i3c_disable_i3c, setup, NULL)
{
    // Arrange

    // Act
    ddr_manager_enable_bwl_mr4();
    ddr_manager_enable_bwl_i3c();
    ddr_manager_disable_bwl_i3c();

    // Assert
    assert_true(ddr_manager_get_bwl_engaged());

    ddr_manager_disable_bwl_mr4();

    assert_false(ddr_manager_get_bwl_engaged());
}

TEST_FUNCTION(test_ddr_manager_enable_i3c_force_disable_i3c, setup, NULL)
{
    // Arrange

    // Act
    ddr_manager_enable_bwl_i3c();
    ddr_manager_enable_bwl_force();
    ddr_manager_disable_bwl_i3c();

    // Assert
    assert_true(ddr_manager_get_bwl_engaged());

    ddr_manager_disable_bwl_force();

    assert_false(ddr_manager_get_bwl_engaged());
}