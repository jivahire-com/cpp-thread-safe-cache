//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_hw_int_telemetry_test.cpp
 * Power service telemetry interface tests
 */

/*------------- Includes -----------------*/
#include "power_test.h" // for POWER_TEST

#include <cstddef> // for NULL

extern "C" {

#include <CMockaWrapper.h>       // for CmockaWrapperTest, assert_int_equal
#include <corebits.h>            // for corebits_t, corebits_is_bit_set
#include <power_hw_int_i.h>      // for power_init_core, power_init_soc
#include <sensor_fifo_service.h> // for sensor_fifo_svc_get_properties

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_sensor_fifo_svc_get_properties(SENSOR_FIFO_ID fifo, psensor_fifo_properties_t properties)
{
    check_expected(fifo);
    assert_non_null(properties);
    memcpy(properties, mock_type(psensor_fifo_properties_t), sizeof(sensor_fifo_properties_t));
}

void __wrap_sensor_fifo_svc_enable_fifo(SENSOR_FIFO_ID fifo)
{
    check_expected(fifo);
}

void __wrap_sensor_fifo_svc_set_global_hw_enable(bool enable)
{
    check_expected(enable);
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_scp_message(plimit_telem_msg_t* plimit_msg)
{
    sensor_ram_poll_status_t status;
    status.curr_data_is_valid = mock_type(bool);
    status.more_entries = mock_type(bool);
    if (status.curr_data_is_valid)
    {
        memcpy(plimit_msg, mock_type(plimit_telem_msg_t*), sizeof(plimit_telem_msg_t));
    }
    return status;
}
// end mocks

} // extern "C"

/*-------- Function Prototypes -----------*/

/*------------- Functions ----------------*/

POWER_TEST(power_telemetry_init_config, NULL, NULL)
{
#define EPOCH_COUNT   0x34
#define ENTRY_SIZE    0x12
#define STRIDE_SIZE   0x56
#define START_ADDRESS 0x12345678

    power_telcfg_t telemetry_config;
    sensor_fifo_properties_t properties = {.entry_size_bytes = ENTRY_SIZE,
                                           .stride_size_bytes = STRIDE_SIZE,
                                           .start_address_incl = START_ADDRESS,
                                           .num_entries_or_strides = EPOCH_COUNT};

    // always return the above properties structure
    will_return_always(__wrap_sensor_fifo_svc_get_properties, &properties);

    // expect these calls to get the properties
    expect_value(__wrap_sensor_fifo_svc_get_properties, fifo, SENSOR_FIFO_PSTATE_TELEMETRY_HW);
    expect_value(__wrap_sensor_fifo_svc_get_properties, fifo, SENSOR_FIFO_SCP_MSG_TELEMETRY_HW);
    expect_value(__wrap_sensor_fifo_svc_get_properties, fifo, SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW);
    expect_value(__wrap_sensor_fifo_svc_get_properties, fifo, SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW);
    expect_value(__wrap_sensor_fifo_svc_get_properties, fifo, SENSOR_FIFO_TILE_VOLTAGE_TELEMETRY_HW);

    power_telemetry_init_config(&telemetry_config);

    // check buffer sizes
    assert_int_equal(telemetry_config.current_telem_buffer_size, EPOCH_COUNT - 1);
    assert_int_equal(telemetry_config.temp_telem_buffer_size, EPOCH_COUNT - 1);
    assert_int_equal(telemetry_config.volt_telem_buffer_size, EPOCH_COUNT - 1);
    // entry sizes should be the same
    assert_int_equal(telemetry_config.current_telem_entry_size, ENTRY_SIZE);
    assert_int_equal(telemetry_config.temp_telem_entry_size, ENTRY_SIZE);
    assert_int_equal(telemetry_config.volt_telem_entry_size, ENTRY_SIZE);
    // stride sizes should be the same
    assert_int_equal(telemetry_config.current_telem_stride_size, STRIDE_SIZE);
    assert_int_equal(telemetry_config.temp_telem_stride_size, STRIDE_SIZE);
    assert_int_equal(telemetry_config.volt_telem_stride_size, STRIDE_SIZE);

    // start addresses should be the same
    assert_int_equal(telemetry_config.pstate_telem_wr_address, START_ADDRESS + FIFO_TIMESTAMP_SIZE);
    assert_int_equal(telemetry_config.scp_msg_telem_wr_address, START_ADDRESS + FIFO_TIMESTAMP_SIZE);
    assert_int_equal(telemetry_config.current_telem_start_addr, START_ADDRESS + FIFO_TIMESTAMP_SIZE);
    assert_int_equal(telemetry_config.temp_telem_start_addr, START_ADDRESS + FIFO_TIMESTAMP_SIZE);
    assert_int_equal(telemetry_config.volt_telem_start_addr, START_ADDRESS + FIFO_TIMESTAMP_SIZE);
}

POWER_TEST(power_telemetry_enable, NULL, NULL)
{
    expect_value(__wrap_sensor_fifo_svc_enable_fifo, fifo, SENSOR_FIFO_PSTATE_TELEMETRY_HW);
    expect_value(__wrap_sensor_fifo_svc_enable_fifo, fifo, SENSOR_FIFO_SCP_MSG_TELEMETRY_HW);
    expect_value(__wrap_sensor_fifo_svc_enable_fifo, fifo, SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW);
    expect_value(__wrap_sensor_fifo_svc_enable_fifo, fifo, SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW);
    expect_value(__wrap_sensor_fifo_svc_enable_fifo, fifo, SENSOR_FIFO_TILE_VOLTAGE_TELEMETRY_HW);
    expect_value(__wrap_sensor_fifo_svc_enable_fifo, fifo, SENSOR_FIFO_PVT_TEMP_FW);
    expect_value(__wrap_sensor_fifo_svc_enable_fifo, fifo, SENSOR_FIFO_PVT_VOLTAGE_FW);
    expect_value(__wrap_sensor_fifo_svc_enable_fifo, fifo, SENSOR_FIFO_VR_TEMP_FW);
    expect_value(__wrap_sensor_fifo_svc_enable_fifo, fifo, SENSOR_FIFO_VR_CURRENT_FW);

    expect_value(__wrap_sensor_fifo_svc_set_global_hw_enable, enable, true);

    power_telemetry_enable();
}

#define TEST_CORE     4
#define TEST_DESIRED  0x1B
#define TEST_SELECTED 0x1E

void message_update_callback(unsigned int core_id, uint8_t desired_pstate, uint8_t base_pstate, uint8_t throttle_pri, uint8_t boost_pri)
{
    check_expected(core_id);
    check_expected(desired_pstate);
    check_expected(base_pstate);
    check_expected(throttle_pri);
    check_expected(boost_pri);
}

void message_success_callback(unsigned int core_id, uint8_t desired_pstate, uint8_t current_pstate)
{
    check_expected(core_id);
    check_expected(desired_pstate);
    check_expected(current_pstate);
}

POWER_TEST(telemetry_message_poll__success, NULL, NULL)
{
    plimit_telem_msg_t plimit_msg;

    plimit_msg.data.type = PLIMIT_SUCCESS_TYPE;
    plimit_msg.data.plimit_success_msg.cppc_desired = TEST_DESIRED;
    plimit_msg.data.plimit = TEST_SELECTED;
    plimit_msg.data.core_id = TEST_CORE;

    will_return(__wrap_sensor_fifo_svc_poll_scp_message, true);  // valid data
    will_return(__wrap_sensor_fifo_svc_poll_scp_message, false); // more entries
    will_return(__wrap_sensor_fifo_svc_poll_scp_message, &plimit_msg);

    expect_value(message_success_callback, core_id, TEST_CORE);
    expect_value(message_success_callback, desired_pstate, TEST_DESIRED);
    expect_value(message_success_callback, current_pstate, TEST_SELECTED);

    power_telemetry_message_poll(message_update_callback, message_success_callback);
}

POWER_TEST(telemetry_message_poll__success_not_expected__invalid_core, NULL, NULL)
{
    plimit_telem_msg_t plimit_msg;

    plimit_msg.data.core_id = NUM_AP_CORES_PER_DIE;

    will_return(__wrap_sensor_fifo_svc_poll_scp_message, true);  // valid data
    will_return(__wrap_sensor_fifo_svc_poll_scp_message, false); // more entries
    will_return(__wrap_sensor_fifo_svc_poll_scp_message, &plimit_msg);

    power_telemetry_message_poll(message_update_callback, message_success_callback);
}

POWER_TEST(telemetry_message_poll__update, NULL, NULL)
{
#define TEST_THROTTLE  0x5
#define TEST_BOOST     0x7
#define TEST_BASE_PERF 0x11

    plimit_telem_msg_t plimit_msg;

    plimit_msg.data.type = PLIMIT_UPDATE_TYPE;
    plimit_msg.data.core_id = TEST_CORE;
    plimit_msg.data.pstate = TEST_DESIRED;
    plimit_msg.data.cppc_update_msg.desired_perf = TEST_BASE_PERF;
    plimit_msg.data.cppc_update_msg.throttle_pri = TEST_THROTTLE;
    plimit_msg.data.cppc_update_msg.boost_pri = TEST_BOOST;

    will_return(__wrap_sensor_fifo_svc_poll_scp_message, true);  // valid data
    will_return(__wrap_sensor_fifo_svc_poll_scp_message, false); // more entries
    will_return(__wrap_sensor_fifo_svc_poll_scp_message, &plimit_msg);

    expect_value(message_update_callback, core_id, TEST_CORE);
    expect_value(message_update_callback, desired_pstate, TEST_DESIRED);
    expect_value(message_update_callback, base_pstate, TEST_BASE_PERF);
    expect_value(message_update_callback, throttle_pri, TEST_THROTTLE);
    expect_value(message_update_callback, boost_pri, TEST_BOOST);

    power_telemetry_message_poll(message_update_callback, message_success_callback);
}