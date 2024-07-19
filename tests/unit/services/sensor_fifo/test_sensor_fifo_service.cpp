//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_sensor_fifo_service.cpp
 *
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include "DfwkStatus.h"                   // for DFWK_SUCCESS
#include "fpfw_status.h"                  // for FPFW_STATUS_SUCCESS, FPFW_...
#include "sensor_fifo_driver_interface.h" // for sensor_fifo_device_propert...
#include "sensor_fifo_service_init.h"

#include <FpFwUtils.h>           // for FPFW_UNUSED
#include <sensor_fifo_service.h> // for SENSOR_FIFO_CORE_CURRENT_T...
#include <stdint.h>              // for uint8_t
}

/*-- Symbolic Constant Macros (defines) --*/
#define ENTRY_SIZE_BYTES  (32)
#define STRIDE_SIZE_BYTES (48)
#define START_ADDR        (7)
#define END_ADDR          (9)
#define EPOCH_COUNT       (3)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static sensor_fifo_device_properties_t s_test_fifo_properties[SENSOR_FIFO_MAX_ID];
static sensor_fifo_driver_interface_t s_sensor_fifo_inf;
static sensor_fifo_device_t device;
static char s_name[] = "testName";

/*------------- Functions ----------------*/
static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);

    s_sensor_fifo_inf.device = &device;
    s_sensor_fifo_inf.device->fifo_property_table = s_test_fifo_properties;
    expect_value(__wrap_DfwkClientInterfaceOpen, Interface, &s_sensor_fifo_inf);
    will_return(__wrap_DfwkClientInterfaceOpen, DFWK_SUCCESS);
    expect_value(__wrap_FpFwAssert, expression, true);

    sensor_fifo_svc_initialize(&s_sensor_fifo_inf);

    return 0;
}

//
// Tests
//
TEST_FUNCTION(test_svc_get_prop, test_setup, nullptr)
{
    s_test_fifo_properties[SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW].entry_size_bytes = ENTRY_SIZE_BYTES;
    s_test_fifo_properties[SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW].stride_size_bytes = STRIDE_SIZE_BYTES;
    s_test_fifo_properties[SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW].start_address = START_ADDR;
    s_test_fifo_properties[SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW].end_address = ENTRY_SIZE_BYTES;
    s_test_fifo_properties[SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW].entry_count = EPOCH_COUNT;
    s_test_fifo_properties[SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW].name = s_name;

    expect_value(__wrap_FpFwAssert, expression, true);
    expect_value(__wrap_FpFwAssert, expression, true);

    sensor_fifo_properties_t fifo_properties;
    sensor_fifo_svc_get_properties(SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW, &fifo_properties);

    assert_int_equal(fifo_properties.entry_size_bytes, ENTRY_SIZE_BYTES);
    assert_int_equal(fifo_properties.stride_size_bytes, STRIDE_SIZE_BYTES);
    assert_int_equal(fifo_properties.start_address, START_ADDR);
    assert_int_equal(fifo_properties.end_address, ENTRY_SIZE_BYTES);
    assert_int_equal(fifo_properties.epoch_count, EPOCH_COUNT);
    assert_string_equal(fifo_properties.name, s_name);
}

TEST_FUNCTION(test_svc_poll_tile_temp, test_setup, nullptr)
{
#define EXPECTED_TIMESTAMP (0x323431)

    // num_entries_read = 1, num_entries_remaining = 0
    tile_temp_t temperature_data = {0};
    tile_temp_t mock_data;
    mock_data.timestamp = EXPECTED_TIMESTAMP;
    uint16_t tile_index = 0;

    will_return(__wrap_sensor_fifo_driver_read_entry, &mock_data);
    will_return(__wrap_sensor_fifo_driver_read_entry, 1);  // num_entries_read
    will_return(__wrap_sensor_fifo_driver_read_entry, 0);  // num_entries_remaining
    will_return(__wrap_sensor_fifo_driver_read_entry, 62); // stride_index
    will_return(__wrap_sensor_fifo_driver_read_entry, FPFW_STATUS_SUCCESS);

    sensor_ram_poll_status_t status = sensor_fifo_svc_poll_tile_temperature(&temperature_data, &tile_index);

    assert_true(status.curr_data_is_valid);
    assert_false(status.more_entries);
    assert_int_equal(temperature_data.timestamp, EXPECTED_TIMESTAMP);

    // num_entries_read = 0, num_entries_remaining = 1
    temperature_data.timestamp = 0;
    will_return(__wrap_sensor_fifo_driver_read_entry, &mock_data);
    will_return(__wrap_sensor_fifo_driver_read_entry, 0);  // num_entries_read
    will_return(__wrap_sensor_fifo_driver_read_entry, 1);  // num_entries_remaining
    will_return(__wrap_sensor_fifo_driver_read_entry, 62); // stride_index
    will_return(__wrap_sensor_fifo_driver_read_entry, FPFW_STATUS_SUCCESS);

    status = sensor_fifo_svc_poll_tile_temperature(&temperature_data, &tile_index);

    assert_false(status.curr_data_is_valid);
    assert_true(status.more_entries);
    assert_int_equal(temperature_data.timestamp, 0);

    // num_entries_read = 1, num_entries_remaining = 1, FPFW_STATUS_FAIL
    temperature_data.timestamp = 0;
    will_return(__wrap_sensor_fifo_driver_read_entry, &mock_data);
    will_return(__wrap_sensor_fifo_driver_read_entry, 1);  // num_entries_read
    will_return(__wrap_sensor_fifo_driver_read_entry, 1);  // num_entries_remaining
    will_return(__wrap_sensor_fifo_driver_read_entry, 62); // stride_index
    will_return(__wrap_sensor_fifo_driver_read_entry, FPFW_STATUS_FAIL);

    status = sensor_fifo_svc_poll_tile_temperature(&temperature_data, &tile_index);

    assert_false(status.curr_data_is_valid);
    assert_false(status.more_entries);
    ;
}

TEST_FUNCTION(test_svc_poll_scp_msg, test_setup, nullptr)
{
#define EXPECTED_TIMESTAMP (0x323431)

    // num_entries_read = 1, num_entries_remaining = 0
    plimit_msg_telem_t scp_msg = {0};
    plimit_msg_telem_t mock_data;
    mock_data.timestamp = EXPECTED_TIMESTAMP;

    will_return(__wrap_sensor_fifo_driver_read_entry, &mock_data);
    will_return(__wrap_sensor_fifo_driver_read_entry, 1);  // num_entries_read
    will_return(__wrap_sensor_fifo_driver_read_entry, 0);  // num_entries_remaining
    will_return(__wrap_sensor_fifo_driver_read_entry, 62); // stride_index
    will_return(__wrap_sensor_fifo_driver_read_entry, FPFW_STATUS_SUCCESS);

    sensor_ram_poll_status_t status = sensor_fifo_svc_poll_scp_message(&scp_msg);

    assert_true(status.curr_data_is_valid);
    assert_false(status.more_entries);
    assert_int_equal(scp_msg.timestamp, EXPECTED_TIMESTAMP);

    // num_entries_read = 0, num_entries_remaining = 1
    scp_msg.timestamp = 0;
    will_return(__wrap_sensor_fifo_driver_read_entry, &mock_data);
    will_return(__wrap_sensor_fifo_driver_read_entry, 0);  // num_entries_read
    will_return(__wrap_sensor_fifo_driver_read_entry, 1);  // num_entries_remaining
    will_return(__wrap_sensor_fifo_driver_read_entry, 62); // stride_index
    will_return(__wrap_sensor_fifo_driver_read_entry, FPFW_STATUS_SUCCESS);

    status = sensor_fifo_svc_poll_scp_message(&scp_msg);

    assert_false(status.curr_data_is_valid);
    assert_true(status.more_entries);
    assert_int_equal(scp_msg.timestamp, 0);

    // num_entries_read = 1, num_entries_remaining = 1, FPFW_STATUS_FAIL
    scp_msg.timestamp = 0;
    will_return(__wrap_sensor_fifo_driver_read_entry, &mock_data);
    will_return(__wrap_sensor_fifo_driver_read_entry, 1);  // num_entries_read
    will_return(__wrap_sensor_fifo_driver_read_entry, 1);  // num_entries_remaining
    will_return(__wrap_sensor_fifo_driver_read_entry, 62); // stride_index
    will_return(__wrap_sensor_fifo_driver_read_entry, FPFW_STATUS_FAIL);

    status = sensor_fifo_svc_poll_scp_message(&scp_msg);

    assert_false(status.curr_data_is_valid);
    assert_false(status.more_entries);
    ;
}