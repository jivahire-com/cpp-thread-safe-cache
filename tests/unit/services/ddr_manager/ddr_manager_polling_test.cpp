//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_polling_tests.cpp
 * DDR Manager Polling tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstddef>         // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <ddr_i3c.h>
#include <ddr_manager_i.h>   // for ddr_poll_dimms, ddr_worker_thread_func
#include <idhw.h>
} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
#define NUM_SEN (6*2) // 6 DIMMs, 2 sensors each
#define NUM_DIMM (6) // 6 DIMMs

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
extern "C" {
void __wrap_ddr_manager_engage_bwl()
{
    function_called();
}

void __wrap_ddr_manager_disengage_bwl()
{
    function_called();
}

void __wrap_ddr_manager_set_thermal_trip_gpio()
{
    function_called();
}

bool __wrap_ddr_manager_platform_is_polling_supported()
{
    return true;
}
}// extern "C"

//
// Tests
//
TEST_FUNCTION(test_ddr_manager_poll_below_high_to_low_thresh, NULL, NULL)
{
    // Arrange

    // Set up the expected calls
    will_return_always(__wrap_idhw_get_die_id, DIE_0);

    // High temp
    for(uint8_t sens_idx = 0; sens_idx < NUM_SEN; sens_idx++)
    {
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, dev_id, 2);
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, mr_reg, 2);
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, instance, 2);

        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT2 | BIT4 | BIT5 | BIT6 | BIT7));
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, DDR_I3C_INTERFACE_SUCCESS);

        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT1 | BIT2));
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, DDR_I3C_INTERFACE_SUCCESS);
    }

    // Low temp
    for(uint8_t sens_idx = 0; sens_idx < NUM_SEN; sens_idx++)
    {
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, dev_id, 2);
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, mr_reg, 2);
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, instance, 2);

        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT2 | BIT4 | BIT5 | BIT6 | BIT7));
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, DDR_I3C_INTERFACE_SUCCESS);

        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT1));
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, DDR_I3C_INTERFACE_SUCCESS);
    }

    expect_function_call(__wrap_ddr_manager_engage_bwl);
    expect_function_call(__wrap_ddr_manager_disengage_bwl);

    // Act
    ddr_poll_dimms();
    ddr_poll_dimms();

    // Assert
}

TEST_FUNCTION(test_ddr_manager_poll_crit_thresh, NULL, NULL)
{
    // Arrange
    will_return_always(__wrap_idhw_get_die_id, DIE_0);

    // Crit temp
    for(uint8_t sens_idx = 0; sens_idx < NUM_SEN; sens_idx++)
    {
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, dev_id, 2);
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, mr_reg, 2);
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, instance, 2);

        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT2 | BIT4 | BIT5 | BIT6 | BIT7));
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, DDR_I3C_INTERFACE_SUCCESS);

        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT1 | BIT2 | BIT3));
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, DDR_I3C_INTERFACE_SUCCESS);

    }

    for(uint8_t dimm_idx = 0; dimm_idx < NUM_DIMM; dimm_idx++)
    {
        expect_function_call(__wrap_ddr_manager_set_thermal_trip_gpio);
    }
    expect_function_call(__wrap_ddr_manager_engage_bwl);

    // Act
    ddr_poll_dimms();
}