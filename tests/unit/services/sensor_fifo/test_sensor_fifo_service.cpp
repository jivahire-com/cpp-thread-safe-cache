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
#include <DfwkStatus.h>                   // for DFWK_SUCCESS
#include <FpFwUtils.h>                    // for FPFW_UNUSED
#include <device_fifo_id.h>               // for DEVICE_FIFO_DIMM_TEMP_TL...
#include <fpfw_status.h>                  // for FPFW_STATUS_SUCCESS, FPFW_...
#include <sensor_fifo_driver_interface.h> // for sensor_fifo_device_propert...
#include <sensor_fifo_driver_interface_i.h>
#include <sensor_fifo_service.h>   // for SENSOR_FIFO_CORE_CURRENT_T...
#include <sensor_fifo_service_i.h> // for SENSOR_FIFO_CORE_CURRENT_T...
#include <sensor_fifo_service_init.h>
#include <stdint.h> // for uint8_t
}

/*-- Symbolic Constant Macros (defines) --*/
#define ENTRY_SIZE_BYTES  (32)
#define STRIDE_SIZE_BYTES (48)
#define START_ADDR        (7)
#define END_ADDR          (9)
#define STRIDE_COUNT      (3)

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

    s_test_fifo_properties[SENSOR_FIFO_PSTATE_TELEMETRY_HW].device_fifo_id = DEVICE_FIFO_PSTATE_TLM_HW_PROD;
    s_test_fifo_properties[SENSOR_FIFO_SCP_MSG_TELEMETRY_HW].device_fifo_id = DEVICE_FIFO_SCP_MSG_TLM_HW_PROD;
    s_test_fifo_properties[SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW].device_fifo_id = DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD;
    s_test_fifo_properties[SENSOR_FIFO_TILE_VOLTAGE_TELEMETRY_HW].device_fifo_id = DEVICE_FIFO_TILE_VOLT_TLM_HW_PROD;
    s_test_fifo_properties[SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW].device_fifo_id = DEVICE_FIFO_CORE_CURRENT_TLM_HW_PROD;
    s_test_fifo_properties[SENSOR_FIFO_PVT_TEMP_FW].device_fifo_id = DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD;
    s_test_fifo_properties[SENSOR_FIFO_PVT_VOLTAGE_FW].device_fifo_id = DEVICE_FIFO_PVT_VOLT_TLM_FW_PROD;
    s_test_fifo_properties[SENSOR_FIFO_DIMM_TEMP_FW].device_fifo_id = DEVICE_FIFO_DIMM_TEMP_TLM_FW_PROD;
    s_test_fifo_properties[SENSOR_FIFO_VR_TEMP_FW].device_fifo_id = DEVICE_FIFO_VR_TEMP_TLM_FW_PROD;
    s_test_fifo_properties[SENSOR_FIFO_VR_CURRENT_FW].device_fifo_id = DEVICE_FIFO_VR_CURRENT_TLM_FW_PROD;

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
    s_test_fifo_properties[SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW].start_address_incl = START_ADDR;
    s_test_fifo_properties[SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW].end_address_excl = END_ADDR;
    s_test_fifo_properties[SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW].entry_count = STRIDE_COUNT;
    s_test_fifo_properties[SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW].name = s_name;

    expect_value(__wrap_FpFwAssert, expression, true);
    expect_value(__wrap_FpFwAssert, expression, true);

    sensor_fifo_properties_t fifo_properties;
    sensor_fifo_svc_get_properties(SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW, &fifo_properties);

    assert_int_equal(fifo_properties.entry_size_bytes, ENTRY_SIZE_BYTES);
    assert_int_equal(fifo_properties.stride_size_bytes, STRIDE_SIZE_BYTES);
    assert_int_equal(fifo_properties.start_address_incl, START_ADDR);
    assert_int_equal(fifo_properties.end_address_excl, END_ADDR);
    assert_int_equal(fifo_properties.num_entries_or_strides, STRIDE_COUNT);
    assert_string_equal(fifo_properties.name, s_name);
}

TEST_FUNCTION(test_svc_global_hw_enable, test_setup, nullptr)
{
    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, SENSOR_FIFO_SYNC_SET_GLOBAL_HW_ENABLE);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_SUCCESS);

    sensor_fifo_svc_set_global_hw_enable(true);

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, SENSOR_FIFO_SYNC_SET_GLOBAL_HW_ENABLE);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_FAIL);

    sensor_fifo_svc_set_global_hw_enable(false);
}

TEST_FUNCTION(test_svc_fifo_enable_disable, test_setup, nullptr)
{
    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, SENSOR_FIFO_SYNC_SET_FIFO_ENABLE);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_SUCCESS);

    sensor_fifo_svc_enable_fifo(SENSOR_FIFO_VR_CURRENT_FW);

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, SENSOR_FIFO_SYNC_SET_FIFO_ENABLE);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_FAIL);

    sensor_fifo_svc_enable_fifo(SENSOR_FIFO_VR_CURRENT_FW);

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, SENSOR_FIFO_SYNC_SET_FIFO_ENABLE);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_SUCCESS);

    sensor_fifo_svc_disable_fifo(SENSOR_FIFO_VR_CURRENT_FW);

    expect_value(__wrap_DfwkInterfaceSendSync, Request->RequestType, SENSOR_FIFO_SYNC_SET_FIFO_ENABLE);
    will_return(__wrap_DfwkInterfaceSendSync, FPFW_STATUS_FAIL);

    sensor_fifo_svc_disable_fifo(SENSOR_FIFO_VR_CURRENT_FW);
}

TEST_FUNCTION(test_svc_add_pvt_temp, test_setup, nullptr)
{
    soc_pvt_temp_t sensor_data = {0};

    expect_value(__wrap_sensor_fifo_driver_write_entries, fifo_id, DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD);
    expect_value(__wrap_sensor_fifo_driver_write_entries, entry_size, sizeof(soc_pvt_temp_t));
    expect_value(__wrap_sensor_fifo_driver_write_entries, num_entries, 1);
    expect_value(__wrap_sensor_fifo_driver_write_entries, stride_index, 0);

    will_return(__wrap_sensor_fifo_driver_write_entries, FPFW_STATUS_SUCCESS);

    sensor_fifo_svc_add_soc_pvt_temperature(&sensor_data);
}

TEST_FUNCTION(test_svc_add_pvt_voltage, test_setup, nullptr)
{
    soc_pvt_voltage_t sensor_data = {0};

    expect_value(__wrap_sensor_fifo_driver_write_entries, fifo_id, DEVICE_FIFO_PVT_VOLT_TLM_FW_PROD);
    expect_value(__wrap_sensor_fifo_driver_write_entries, entry_size, sizeof(soc_pvt_voltage_t));
    expect_value(__wrap_sensor_fifo_driver_write_entries, num_entries, 1);
    expect_value(__wrap_sensor_fifo_driver_write_entries, stride_index, 0);

    will_return(__wrap_sensor_fifo_driver_write_entries, FPFW_STATUS_SUCCESS);

    sensor_fifo_svc_add_soc_pvt_voltage(&sensor_data);
}

TEST_FUNCTION(test_svc_add_dimm_info, test_setup, nullptr)
{
    sensor_ram_dimm_info_t sensor_data = {0};

    expect_value(__wrap_sensor_fifo_driver_write_entries, fifo_id, DEVICE_FIFO_DIMM_TEMP_TLM_FW_PROD);
    expect_value(__wrap_sensor_fifo_driver_write_entries, entry_size, sizeof(sensor_ram_dimm_info_t));
    expect_value(__wrap_sensor_fifo_driver_write_entries, num_entries, 1);
    expect_value(__wrap_sensor_fifo_driver_write_entries, stride_index, 0);

    will_return(__wrap_sensor_fifo_driver_write_entries, FPFW_STATUS_SUCCESS);

    sensor_fifo_svc_add_dimm_info(&sensor_data);
}

TEST_FUNCTION(test_svc_vr_temp, test_setup, nullptr)
{
    vr_temp_t sensor_data = {0};

    expect_value(__wrap_sensor_fifo_driver_write_entries, fifo_id, DEVICE_FIFO_VR_TEMP_TLM_FW_PROD);
    expect_value(__wrap_sensor_fifo_driver_write_entries, entry_size, sizeof(vr_temp_t));
    expect_value(__wrap_sensor_fifo_driver_write_entries, num_entries, 1);
    expect_value(__wrap_sensor_fifo_driver_write_entries, stride_index, 0);

    will_return(__wrap_sensor_fifo_driver_write_entries, FPFW_STATUS_SUCCESS);

    sensor_fifo_svc_add_vr_temperature(&sensor_data);
}

TEST_FUNCTION(test_svc_vr_current, test_setup, nullptr)
{
    vr_current_t sensor_data = {0};

    expect_value(__wrap_sensor_fifo_driver_write_entries, fifo_id, DEVICE_FIFO_VR_CURRENT_TLM_FW_PROD);
    expect_value(__wrap_sensor_fifo_driver_write_entries, entry_size, sizeof(vr_current_t));
    expect_value(__wrap_sensor_fifo_driver_write_entries, num_entries, 1);
    expect_value(__wrap_sensor_fifo_driver_write_entries, stride_index, 0);

    will_return(__wrap_sensor_fifo_driver_write_entries, FPFW_STATUS_SUCCESS);

    sensor_fifo_svc_add_vr_current(&sensor_data);
}

TEST_FUNCTION(test_svc_poll_helper, test_setup, nullptr)
{

    uint8_t mock_data[ENTRY_SIZE_BYTES] = {0};
    uint16_t stride_index = 0;

    expect_value(__wrap_sensor_fifo_driver_read_entry, fifo_id, DEVICE_FIFO_DIMM_TEMP_TLM_FW_PROD);
    expect_value(__wrap_sensor_fifo_driver_read_entry, entry_size, ENTRY_SIZE_BYTES);
    will_return(__wrap_sensor_fifo_driver_read_entry, 1);  // num_entries_read
    will_return(__wrap_sensor_fifo_driver_read_entry, 0);  // num_entries_remaining
    will_return(__wrap_sensor_fifo_driver_read_entry, 62); // stride_index
    will_return(__wrap_sensor_fifo_driver_read_entry, FPFW_STATUS_SUCCESS);

    sensor_ram_poll_status_t status =
        fifo_poll_helper(DEVICE_FIFO_DIMM_TEMP_TLM_FW_PROD, ENTRY_SIZE_BYTES, mock_data, &stride_index);

    assert_true(status.curr_data_is_valid);
    assert_false(status.more_entries);

    expect_value(__wrap_sensor_fifo_driver_read_entry, fifo_id, DEVICE_FIFO_DIMM_TEMP_TLM_FW_PROD);
    expect_value(__wrap_sensor_fifo_driver_read_entry, entry_size, ENTRY_SIZE_BYTES);
    will_return(__wrap_sensor_fifo_driver_read_entry, 0);  // num_entries_read
    will_return(__wrap_sensor_fifo_driver_read_entry, 5);  // num_entries_remaining
    will_return(__wrap_sensor_fifo_driver_read_entry, 62); // stride_index
    will_return(__wrap_sensor_fifo_driver_read_entry, FPFW_STATUS_SUCCESS);

    status = fifo_poll_helper(DEVICE_FIFO_DIMM_TEMP_TLM_FW_PROD, ENTRY_SIZE_BYTES, mock_data, &stride_index);

    assert_false(status.curr_data_is_valid);
    assert_true(status.more_entries);

    expect_value(__wrap_sensor_fifo_driver_read_entry, fifo_id, DEVICE_FIFO_DIMM_TEMP_TLM_FW_PROD);
    expect_value(__wrap_sensor_fifo_driver_read_entry, entry_size, ENTRY_SIZE_BYTES);
    will_return(__wrap_sensor_fifo_driver_read_entry, 5);  // num_entries_read
    will_return(__wrap_sensor_fifo_driver_read_entry, 5);  // num_entries_remaining
    will_return(__wrap_sensor_fifo_driver_read_entry, 62); // stride_index
    will_return(__wrap_sensor_fifo_driver_read_entry, FPFW_STATUS_FAIL);

    status = fifo_poll_helper(DEVICE_FIFO_DIMM_TEMP_TLM_FW_PROD, ENTRY_SIZE_BYTES, mock_data, &stride_index);

    assert_false(status.curr_data_is_valid);
    assert_false(status.more_entries);
}

TEST_FUNCTION(test_svc_poll_tile_temp, test_setup, nullptr)
{
    tile_temp_t temperature_data = {0};
    uint16_t tile_index = 0;

    expect_value(__wrap_sensor_fifo_driver_read_entry, fifo_id, DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD);
    expect_value(__wrap_sensor_fifo_driver_read_entry, entry_size, sizeof(tile_temp_t));
    will_return(__wrap_sensor_fifo_driver_read_entry, 1);  // num_entries_read
    will_return(__wrap_sensor_fifo_driver_read_entry, 0);  // num_entries_remaining
    will_return(__wrap_sensor_fifo_driver_read_entry, 62); // stride_index
    will_return(__wrap_sensor_fifo_driver_read_entry, FPFW_STATUS_SUCCESS);

    sensor_fifo_svc_poll_tile_temperature(&temperature_data, &tile_index);
}

TEST_FUNCTION(test_svc_poll_tile_voltage, test_setup, nullptr)
{
    tile_voltage_t sensor_data = {0};
    uint16_t tile_index = 0;

    expect_value(__wrap_sensor_fifo_driver_read_entry, fifo_id, DEVICE_FIFO_TILE_VOLT_TLM_HW_PROD);
    expect_value(__wrap_sensor_fifo_driver_read_entry, entry_size, sizeof(tile_voltage_t));
    will_return(__wrap_sensor_fifo_driver_read_entry, 1);  // num_entries_read
    will_return(__wrap_sensor_fifo_driver_read_entry, 0);  // num_entries_remaining
    will_return(__wrap_sensor_fifo_driver_read_entry, 62); // stride_index
    will_return(__wrap_sensor_fifo_driver_read_entry, FPFW_STATUS_SUCCESS);

    sensor_fifo_svc_poll_tile_voltage(&sensor_data, &tile_index);
}

TEST_FUNCTION(test_svc_poll_core_current, test_setup, nullptr)
{
    core_current_t sensor_data = {0};
    uint16_t core_index = 0;

    expect_value(__wrap_sensor_fifo_driver_read_entry, fifo_id, DEVICE_FIFO_CORE_CURRENT_TLM_HW_PROD);
    expect_value(__wrap_sensor_fifo_driver_read_entry, entry_size, sizeof(core_current_t));
    will_return(__wrap_sensor_fifo_driver_read_entry, 1);  // num_entries_read
    will_return(__wrap_sensor_fifo_driver_read_entry, 0);  // num_entries_remaining
    will_return(__wrap_sensor_fifo_driver_read_entry, 62); // stride_index
    will_return(__wrap_sensor_fifo_driver_read_entry, FPFW_STATUS_SUCCESS);

    sensor_fifo_svc_poll_core_current(&sensor_data, &core_index);
}

TEST_FUNCTION(test_svc_poll_pstate, test_setup, nullptr)
{
    pstate_telem_t sensor_data = {0};

    expect_value(__wrap_sensor_fifo_driver_read_entry, fifo_id, DEVICE_FIFO_PSTATE_TLM_HW_PROD);
    expect_value(__wrap_sensor_fifo_driver_read_entry, entry_size, sizeof(pstate_telem_t));
    will_return(__wrap_sensor_fifo_driver_read_entry, 1);  // num_entries_read
    will_return(__wrap_sensor_fifo_driver_read_entry, 0);  // num_entries_remaining
    will_return(__wrap_sensor_fifo_driver_read_entry, 62); // stride_index
    will_return(__wrap_sensor_fifo_driver_read_entry, FPFW_STATUS_SUCCESS);

    sensor_fifo_svc_poll_core_pstate(&sensor_data);
}

TEST_FUNCTION(test_svc_poll_scp_msg, test_setup, nullptr)
{
    plimit_telem_msg_t sensor_data = {0};

    expect_value(__wrap_sensor_fifo_driver_read_entry, fifo_id, DEVICE_FIFO_SCP_MSG_TLM_HW_PROD);
    expect_value(__wrap_sensor_fifo_driver_read_entry, entry_size, sizeof(plimit_telem_msg_t));
    will_return(__wrap_sensor_fifo_driver_read_entry, 1);  // num_entries_read
    will_return(__wrap_sensor_fifo_driver_read_entry, 0);  // num_entries_remaining
    will_return(__wrap_sensor_fifo_driver_read_entry, 62); // stride_index
    will_return(__wrap_sensor_fifo_driver_read_entry, FPFW_STATUS_SUCCESS);

    sensor_fifo_svc_poll_scp_message(&sensor_data);
}

TEST_FUNCTION(test_svc_poll_pvt_temp, test_setup, nullptr)
{
    soc_pvt_temp_t sensor_data = {0};

    expect_value(__wrap_sensor_fifo_driver_read_entry, fifo_id, DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD);
    expect_value(__wrap_sensor_fifo_driver_read_entry, entry_size, sizeof(soc_pvt_temp_t));
    will_return(__wrap_sensor_fifo_driver_read_entry, 1);  // num_entries_read
    will_return(__wrap_sensor_fifo_driver_read_entry, 0);  // num_entries_remaining
    will_return(__wrap_sensor_fifo_driver_read_entry, 62); // stride_index
    will_return(__wrap_sensor_fifo_driver_read_entry, FPFW_STATUS_SUCCESS);

    sensor_fifo_svc_poll_soc_pvt_temperature(&sensor_data);
}

TEST_FUNCTION(test_svc_poll_pvt_voltage, test_setup, nullptr)
{
    soc_pvt_voltage_t sensor_data = {0};

    expect_value(__wrap_sensor_fifo_driver_read_entry, fifo_id, DEVICE_FIFO_PVT_VOLT_TLM_FW_PROD);
    expect_value(__wrap_sensor_fifo_driver_read_entry, entry_size, sizeof(soc_pvt_voltage_t));
    will_return(__wrap_sensor_fifo_driver_read_entry, 1);  // num_entries_read
    will_return(__wrap_sensor_fifo_driver_read_entry, 0);  // num_entries_remaining
    will_return(__wrap_sensor_fifo_driver_read_entry, 62); // stride_index
    will_return(__wrap_sensor_fifo_driver_read_entry, FPFW_STATUS_SUCCESS);

    sensor_fifo_svc_poll_soc_pvt_voltage(&sensor_data);
}

TEST_FUNCTION(test_svc_poll_dimm_info, test_setup, nullptr)
{
    sensor_ram_dimm_info_t sensor_data = {0};

    expect_value(__wrap_sensor_fifo_driver_read_entry, fifo_id, DEVICE_FIFO_DIMM_TEMP_TLM_FW_PROD);
    expect_value(__wrap_sensor_fifo_driver_read_entry, entry_size, sizeof(sensor_ram_dimm_info_t));
    will_return(__wrap_sensor_fifo_driver_read_entry, 1);  // num_entries_read
    will_return(__wrap_sensor_fifo_driver_read_entry, 0);  // num_entries_remaining
    will_return(__wrap_sensor_fifo_driver_read_entry, 62); // stride_index
    will_return(__wrap_sensor_fifo_driver_read_entry, FPFW_STATUS_SUCCESS);

    sensor_fifo_svc_poll_dimm_info(&sensor_data);
}

TEST_FUNCTION(test_svc_poll_vr_temp, test_setup, nullptr)
{
    vr_temp_t sensor_data = {0};

    expect_value(__wrap_sensor_fifo_driver_read_entry, fifo_id, DEVICE_FIFO_VR_TEMP_TLM_FW_PROD);
    expect_value(__wrap_sensor_fifo_driver_read_entry, entry_size, sizeof(vr_temp_t));
    will_return(__wrap_sensor_fifo_driver_read_entry, 1);  // num_entries_read
    will_return(__wrap_sensor_fifo_driver_read_entry, 0);  // num_entries_remaining
    will_return(__wrap_sensor_fifo_driver_read_entry, 62); // stride_index
    will_return(__wrap_sensor_fifo_driver_read_entry, FPFW_STATUS_SUCCESS);

    sensor_fifo_svc_poll_vr_temperature(&sensor_data);
}

TEST_FUNCTION(test_svc_poll_vr_current, test_setup, nullptr)
{
    vr_current_t sensor_data = {0};

    expect_value(__wrap_sensor_fifo_driver_read_entry, fifo_id, DEVICE_FIFO_VR_CURRENT_TLM_FW_PROD);
    expect_value(__wrap_sensor_fifo_driver_read_entry, entry_size, sizeof(vr_current_t));
    will_return(__wrap_sensor_fifo_driver_read_entry, 1);  // num_entries_read
    will_return(__wrap_sensor_fifo_driver_read_entry, 0);  // num_entries_remaining
    will_return(__wrap_sensor_fifo_driver_read_entry, 62); // stride_index
    will_return(__wrap_sensor_fifo_driver_read_entry, FPFW_STATUS_SUCCESS);

    sensor_fifo_svc_poll_vr_current(&sensor_data);
}