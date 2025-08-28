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
#include <FpFwUtils.h>
#include <ddr_i3c.h>
#include <ddr_manager_bwl.h>
#include <ddr_manager_i.h> // for ddr_poll_dimms, ddr_worker_thread_func
#include <ddrss_intu.h>
#include <idhw.h>
} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
#define NUM_DIMM_PER_DIE     (6)                    // 6 DIMMs
#define NUM_SENSORS_PER_DIMM (NUM_DIMM_PER_DIE * 2) // 6 DIMMs, 2 sensors each
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

    ddr_manager_disable_bwl_i3c();
    ddr_manager_disable_bwl_force();
    ddr_manager_disable_bwl_mr4();

    in_setup_teardown = false;
    return 0;
}

//
// Tests
//
TEST_FUNCTION(test_ddr_manager_poll_below_high_to_low_thresh, setup_disengaged_all, setup_disengaged_all)
{
    // Arrange enable_bwl_i3c()
    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_write32);

    // Arrange disable_bwl_i3c()
    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_write32);

    // High temp (> 85C): 88
    for (uint8_t sens_idx = 0; sens_idx < NUM_SENSORS_PER_DIMM; sens_idx++)
    {
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, dev_id, 1);
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, mr_reg, 1);
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, instance, 1);

        // Low byte 8
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT7));
        // High byte 16 + 64 = 80
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT2));
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, DDR_I3C_INTERFACE_SUCCESS);
    }

    // Act on 88 degrees C
    ddr_poll_dimms();
    check_dimm_temp_thresholds();
    assert_true(ddr_manager_get_bwl_engaged());

    // Low temp
    for (uint8_t sens_idx = 0; sens_idx < NUM_SENSORS_PER_DIMM; sens_idx++)
    {
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, dev_id, 1);
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, mr_reg, 1);
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, instance, 1);

        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT4 | BIT5));
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT2));
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, DDR_I3C_INTERFACE_SUCCESS);
    }

    // Act on 67 degrees C
    ddr_poll_dimms();
    check_dimm_temp_thresholds();
    assert_false(ddr_manager_get_bwl_engaged());
}

TEST_FUNCTION(test_ddr_manager_poll_crit_thresh, NULL, NULL)
{
    // Crit temp
    for (uint8_t sens_idx = 0; sens_idx < NUM_SENSORS_PER_DIMM; sens_idx++)
    {
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, dev_id, 1);
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, mr_reg, 1);
        expect_any_count(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, instance, 1);

        // Low: 1+2+4+8 = 15
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT4 | BIT5 | BIT6 | BIT7));
        // High: 16+32+64+128 = 240
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, (BIT0 | BIT1 | BIT2 | BIT3));
        will_return(__wrap_ddr_i3c_interface_read_temp_sensor_mr_reg, DDR_I3C_INTERFACE_SUCCESS);
    }

    // Expect a GPIO Read/Write for critical temperature
    for (uint8_t dimm_idx = 0; dimm_idx < NUM_DIMM_PER_DIE; dimm_idx++)
    {
        expect_value(__wrap_prod_ddrss_get_intr_event_cper, mc, dimm_idx * 2);
        expect_value(__wrap_prod_ddrss_get_intr_event_cper, intr_event, DDRSS_INTU_MC_MEDIAREFTEMPCHANGED);
        will_return(__wrap_prod_ddrss_get_intr_event_cper, SILIBS_SUCCESS);

        expect_value(__wrap_hm_submit_cper, error_domain_idx, ACPI_ERROR_DOMAIN_DDR);
        expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
        expect_function_call(__wrap_hm_submit_cper);

        expect_function_call(__wrap_mmio_read32);
        will_return(__wrap_mmio_read32, 0);
        expect_function_call(__wrap_mmio_write32);
    }

    // Expect another set of GPIO Read/Writes for ddr_manager_enable_bwl_i3c(); after the critical temp is met
    expect_function_call(__wrap_mmio_read32);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_write32);

    // Act
    ddr_poll_dimms();
    check_dimm_temp_thresholds();
}
