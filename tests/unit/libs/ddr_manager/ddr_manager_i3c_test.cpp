//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_test.cpp
 * DDR Manager tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstddef>         // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <ddr_i3c.h>
#include <ddr_manager_i3c.h> // for ddr_poll_dimms, ddr_worker_thread_func
#include <idhw.h>
} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/
#define I3C_REGISTER_OFFSET 5

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static i3c_instance_t i3c_instance_0;
static i3c_instance_t i3c_instance_1;

/*------------- Functions ----------------*/
//
// Mocks
//
extern "C" {
void __wrap_ddr_i3c_interface_get_instance(i3c_instance_t** instance_0, i3c_instance_t** instance_1)
{
    *instance_0 = &i3c_instance_0;
    *instance_1 = &i3c_instance_1;
}

int32_t __wrap_ddr_i3c_interface_read_temp_sensor_mr_reg(i3c_instance_t* instance,
                                                         i3c_cmd_t* s_i3c_cmd,
                                                         uint8_t dev_id,
                                                         uint8_t mr_reg,
                                                         uint8_t* data8,
                                                         uint8_t* data_len)
{
    check_expected_ptr(instance);
    assert_non_null(s_i3c_cmd);
    check_expected(dev_id);
    check_expected(mr_reg);
    *data8 = mock_type(uint8_t);
    *(data8 + 1) = mock_type(uint8_t);
    *data_len = 2;

    return mock_type(int32_t); // Return the desired status code
}

KNG_DIE_ID __wrap_idhw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

} // extern "C"

//
// Tests
//
TEST_FUNCTION(test_ddr_manager_temperature_sensor_read_ddr0, NULL, NULL)
{
    // Arrange
    const int dimm_idx = 0;
    const int channel_idx = 0;
    ddr_manager_i3c_temperature_t ts_scaled_celsius;

    // Set up the expected calls
    will_return_always(__wrap_idhw_get_die_id, DIE_0);
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, instance, &i3c_instance_1);
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, dev_id, DDR0_TS0);
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, mr_reg, TS_MR49);

    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT2 | BIT4 | BIT5 | BIT6 | BIT7));
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT1));
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, DDR_I3C_INTERFACE_SUCCESS);

    // Act
    int result = ddr_manager_temperature_sensor_read(dimm_idx, channel_idx, &ts_scaled_celsius);

    // Assert
    assert_int_equal(DDR_MANAGER_I3C_SUCCESS, result);
    assert_int_equal(63, ts_scaled_celsius.temp_int);
    assert_int_equal(25, ts_scaled_celsius.temp_frac);
    assert_true(ts_scaled_celsius.is_positive);
}

TEST_FUNCTION(test_ddr_manager_temperature_sensor_read_ddr0_die1, NULL, NULL)
{
    // Arrange
    const int dimm_idx = 0;
    const int channel_idx = 0;
    ddr_manager_i3c_temperature_t ts_scaled_celsius;

    // Set up the expected calls
    will_return_always(__wrap_idhw_get_die_id, DIE_1);
    ddr_manager_i3c_init();

    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, instance, &i3c_instance_0);
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, dev_id, DDR0_TS0);
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, mr_reg, TS_MR49);
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT2 | BIT4 | BIT5 | BIT6 | BIT7));
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT1));
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, DDR_I3C_INTERFACE_SUCCESS);

    // Act
    int result = ddr_manager_temperature_sensor_read(dimm_idx, channel_idx, &ts_scaled_celsius);

    // Assert
    assert_int_equal(DDR_MANAGER_I3C_SUCCESS, result);
    assert_int_equal(63, ts_scaled_celsius.temp_int);
    assert_int_equal(25, ts_scaled_celsius.temp_frac);
    assert_true(ts_scaled_celsius.is_positive);
}

TEST_FUNCTION(test_ddr_manager_temperature_sensor_read_ddr1, NULL, NULL)
{
    // Arrange
    const int dimm_idx = 1;
    const int channel_idx = 0;
    ddr_manager_i3c_temperature_t ts_scaled_celsius;

    // Set up the expected calls
    will_return_always(__wrap_idhw_get_die_id, DIE_0);
    ddr_manager_i3c_init();

    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, dev_id, DDR0_TS0);
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, mr_reg, TS_MR49);
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, instance, &i3c_instance_0);
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT2 | BIT4 | BIT5 | BIT6 | BIT7));
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT1));
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, DDR_I3C_INTERFACE_SUCCESS);

    // Act
    int result = ddr_manager_temperature_sensor_read(dimm_idx, channel_idx, &ts_scaled_celsius);

    // Assert
    assert_int_equal(DDR_MANAGER_I3C_SUCCESS, result);
    assert_int_equal(63, ts_scaled_celsius.temp_int);
    assert_int_equal(25, ts_scaled_celsius.temp_frac);
    assert_true(ts_scaled_celsius.is_positive);
}

TEST_FUNCTION(test_ddr_manager_temperature_sensor_read_ddr2, NULL, NULL)
{
    // Arrange
    const int dimm_idx = 2;
    const int channel_idx = 0;
    ddr_manager_i3c_temperature_t ts_scaled_celsius;

    // Set up the expected calls
    will_return_always(__wrap_idhw_get_die_id, DIE_0);
    ddr_manager_i3c_init();

    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, dev_id, DDR0_TS0 + DDR_I3C_DEV_ID_2 * I3C_REGISTER_OFFSET);
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, mr_reg, TS_MR49);
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, instance, &i3c_instance_1);
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT2 | BIT4 | BIT5 | BIT6 | BIT7));
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT1));
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, DDR_I3C_INTERFACE_SUCCESS);

    // Act
    int result = ddr_manager_temperature_sensor_read(dimm_idx, channel_idx, &ts_scaled_celsius);

    // Assert
    assert_int_equal(DDR_MANAGER_I3C_SUCCESS, result);
    assert_int_equal(63, ts_scaled_celsius.temp_int);
    assert_int_equal(25, ts_scaled_celsius.temp_frac);
    assert_true(ts_scaled_celsius.is_positive);
}

TEST_FUNCTION(test_ddr_manager_temperature_sensor_read_ddr3, NULL, NULL)
{
    // Arrange
    const int dimm_idx = 3;
    const int channel_idx = 0;
    ddr_manager_i3c_temperature_t ts_scaled_celsius;

    // Set up the expected calls
    will_return_always(__wrap_idhw_get_die_id, DIE_0);
    ddr_manager_i3c_init();

    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, dev_id, DDR0_TS0 + DDR_I3C_DEV_ID_2 * I3C_REGISTER_OFFSET);
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, mr_reg, TS_MR49);
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, instance, &i3c_instance_0);
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT2 | BIT4 | BIT5 | BIT6 | BIT7));
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT1));
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, DDR_I3C_INTERFACE_SUCCESS);

    // Act
    int result = ddr_manager_temperature_sensor_read(dimm_idx, channel_idx, &ts_scaled_celsius);

    // Assert
    assert_int_equal(DDR_MANAGER_I3C_SUCCESS, result);
    assert_int_equal(63, ts_scaled_celsius.temp_int);
    assert_int_equal(25, ts_scaled_celsius.temp_frac);
    assert_true(ts_scaled_celsius.is_positive);
}

TEST_FUNCTION(test_ddr_manager_temperature_sensor_read_ddr4, NULL, NULL)
{
    // Arrange
    const int dimm_idx = 4;
    const int channel_idx = 0;
    ddr_manager_i3c_temperature_t ts_scaled_celsius;

    // Set up the expected calls
    will_return_always(__wrap_idhw_get_die_id, DIE_0);
    ddr_manager_i3c_init();

    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, dev_id, DDR0_TS0 + DDR_I3C_DEV_ID_1 * I3C_REGISTER_OFFSET);
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, mr_reg, TS_MR49);
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, instance, &i3c_instance_1);
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT2 | BIT4 | BIT5 | BIT6 | BIT7));
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT1));
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, DDR_I3C_INTERFACE_SUCCESS);

    // Act
    int result = ddr_manager_temperature_sensor_read(dimm_idx, channel_idx, &ts_scaled_celsius);

    // Assert
    assert_int_equal(DDR_MANAGER_I3C_SUCCESS, result);
    assert_int_equal(63, ts_scaled_celsius.temp_int);
    assert_int_equal(25, ts_scaled_celsius.temp_frac);
    assert_true(ts_scaled_celsius.is_positive);
}

TEST_FUNCTION(test_ddr_manager_temperature_sensor_read_ddr0_negative, NULL, NULL)
{
    // Arrange
    const int dimm_idx = 0;
    const int channel_idx = 0;
    ddr_manager_i3c_temperature_t ts_scaled_celsius;

    // Set up the expected calls
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, dev_id, DDR0_TS0);
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, mr_reg, TS_MR49);
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, instance, &i3c_instance_1);
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT2 | BIT4 | BIT5 | BIT6 | BIT7));
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT1 | BIT4));
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, DDR_I3C_INTERFACE_SUCCESS);

    // Act
    int result = ddr_manager_temperature_sensor_read(dimm_idx, channel_idx, &ts_scaled_celsius);

    // Assert
    assert_int_equal(DDR_MANAGER_I3C_SUCCESS, result);
    assert_int_equal(0x3f19, ts_scaled_celsius.as_uint16);
    assert_false(ts_scaled_celsius.is_positive);
}

TEST_FUNCTION(test_ddr_manager_temperature_sensor_read_ddr0_low_word_error, NULL, NULL)
{
    // Arrange
    const int dimm_idx = 0;
    const int channel_idx = 0;
    ddr_manager_i3c_temperature_t ts_scaled_celsius;

    // Set up the expected calls
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, dev_id, DDR0_TS0);
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, mr_reg, TS_MR49);
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, instance, &i3c_instance_1);
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT2 | BIT4 | BIT5 | BIT6 | BIT7));
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0));
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, DDR_I3C_INTERFACE_ERROR);

    // Act
    int result = ddr_manager_temperature_sensor_read(dimm_idx, channel_idx, &ts_scaled_celsius);

    // Assert
    assert_int_equal(DDR_MANAGER_I3C_TRANSACTION_ERROR, result);
}

TEST_FUNCTION(test_ddr_manager_temperature_sensor_read_ddr0_high_word_error, NULL, NULL)
{
    // Arrange
    const int dimm_idx = 0;
    const int channel_idx = 0;
    ddr_manager_i3c_temperature_t ts_scaled_celsius;

    // Set up the expected calls
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, dev_id, DDR0_TS0);
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, mr_reg, TS_MR49);
    expect_value(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, instance, &i3c_instance_1);
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT2 | BIT4 | BIT5 | BIT6 | BIT7));
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT1 | BIT4));
    will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, DDR_I3C_INTERFACE_ERROR);

    // Act
    int result = ddr_manager_temperature_sensor_read(dimm_idx, channel_idx, &ts_scaled_celsius);

    // Assert
    assert_int_equal(DDR_MANAGER_I3C_TRANSACTION_ERROR, result);
}
