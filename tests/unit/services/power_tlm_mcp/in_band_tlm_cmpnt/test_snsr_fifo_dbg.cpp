//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_snsr_fifo_dbg.cpp
 * Test sensor fifo debug mode functionality
 *
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:telemetry

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <ddr_manager_i.h>
#include <in_band_tlm_cmpnt.h>
#include <in_band_tlm_cmpnt_i.h>
#include <mts_manager_i.h>
#include <package_creation_i.h>
#include <sensor_fifo_service.h> // for QUADWORD_SIZE, sensor_ram_...
#include <snsr_fifo_debug_i.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t
#include <telemetry_events_i.h>
#include <telemetry_package_defs.h>
}

/*-- Symbolic Constant Macros (defines) --*/
#define TEST_BUFFER_SIZE   (1024)
#define TEST_RECORD_NUMBER (8)
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
uint8_t test_buffer[TEST_BUFFER_SIZE] = {0};
bool mocked_is_empty[SENSOR_FIFO_MAX_ID];

sensor_ram_poll_status_t more_status = {.curr_data_is_valid = true, .more_entries = true};
sensor_ram_poll_status_t last_status = {.curr_data_is_valid = true, .more_entries = false};
sensor_ram_poll_status_t done_status = {.curr_data_is_valid = false, .more_entries = false};

/*------------- Functions ----------------*/

static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);

    sfd_pkg_start_location = (uintptr_t)test_buffer;
    sfd_pkg_available_size = TEST_BUFFER_SIZE;
    sfd_pkg_used_size = 0;
    sfd_next_record_number = TEST_RECORD_NUMBER;

    for (uint16_t i = 0; i < SENSOR_FIFO_MAX_ID; i++)
    {
        mocked_is_empty[i] = true;
    }

    return 0;
}

TEST_FUNCTION(test_in_band_tlm_cmpnt_begin_sensor_fifo_dbg_collection, test_setup, nullptr)
{
    sfd_package_number = 4;
    in_band_tlm_cmpnt_begin_sensor_fifo_dbg_collection();

    p_telemetry_package_hdr_t package_hdr = (p_telemetry_package_hdr_t)sfd_pkg_start_location;
    assert_int_equal(package_hdr->payload_header.package_number, 4);
    assert_int_equal(sfd_pkg_used_size, sizeof(telemetry_package_hdr_t));
    assert_int_equal(sfd_next_record_number, 1);
}

TEST_FUNCTION(test_in_band_tlm_cmpnt_end_sensor_fifo_dbg_collection, test_setup, nullptr)
{
    sfd_pkg_used_size = 128;

    FpFwListInitialize(&pkg_free_list);
    for (size_t i = 0; i < MAX_PENDING_PACKAGES; i++)
    {
        FpFwListEntryInitialize(&mts_active_pkg_buffer[i].list_entry);
        FpFwListInsertTail(&pkg_free_list, &mts_active_pkg_buffer[i].list_entry);
    }
    FpFwListInitialize(&pkg_active_list);

    // test notification of adding to empty list and notifying host
    will_return(__wrap_mts_is_primary_instance, true);
    expect_function_call(__wrap_mts_client_send_dcp_notification);

    in_band_tlm_cmpnt_end_sensor_fifo_dbg_collection();

    p_telemetry_package_hdr_t package_hdr = (p_telemetry_package_hdr_t)sfd_pkg_start_location;
    assert_int_equal(package_hdr->payload_header.package_payload_size, sfd_pkg_used_size - sizeof(telemetry_package_hdr_t));
}

TEST_FUNCTION(test_add_pstate_collection, test_setup, nullptr)
{
    sfd_pkg_used_size = 100;
    p_telemetry_collection_hdr_t collection_hdr_out =
        (p_telemetry_collection_hdr_t)(sfd_pkg_start_location + sfd_pkg_used_size);

    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &done_status);

    add_pstate_collection();

    assert_int_equal(collection_hdr_out->provider_id, UINT16_MAX);
    assert_int_equal(collection_hdr_out->element_id, UINT16_MAX);
    assert_int_equal(collection_hdr_out->collection_id, SENSOR_FIFO_PSTATE_TELEMETRY_HW);
    assert_int_equal(collection_hdr_out->number_of_elements, 3);
    assert_int_equal(collection_hdr_out->collection_payload_size, 3 * sizeof(pstate_telem_t));

    assert_int_equal(sfd_pkg_used_size, 100 + sizeof(telemetry_collection_hdr_t) + (3 * sizeof(pstate_telem_t)));
}

TEST_FUNCTION(test_add_tile_temperature_collection, test_setup, nullptr)
{
    sfd_pkg_used_size = 80;
    p_telemetry_collection_hdr_t collection_hdr_out =
        (p_telemetry_collection_hdr_t)(sfd_pkg_start_location + sfd_pkg_used_size);

    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &done_status);

    add_tile_temperature_collection();

    assert_int_equal(collection_hdr_out->provider_id, UINT16_MAX);
    assert_int_equal(collection_hdr_out->element_id, UINT16_MAX);
    assert_int_equal(collection_hdr_out->collection_id, SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW);
    assert_int_equal(collection_hdr_out->number_of_elements, 2);
    assert_int_equal(collection_hdr_out->collection_payload_size, 2 * sizeof(tile_temp_t));

    assert_int_equal(sfd_pkg_used_size, 80 + sizeof(telemetry_collection_hdr_t) + (2 * sizeof(tile_temp_t)));
}

TEST_FUNCTION(test_add_tile_voltage_collection, test_setup, nullptr)
{
    sfd_pkg_used_size = 120;
    p_telemetry_collection_hdr_t collection_hdr_out =
        (p_telemetry_collection_hdr_t)(sfd_pkg_start_location + sfd_pkg_used_size);

    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &done_status);

    add_tile_voltage_collection();

    assert_int_equal(collection_hdr_out->provider_id, UINT16_MAX);
    assert_int_equal(collection_hdr_out->element_id, UINT16_MAX);
    assert_int_equal(collection_hdr_out->collection_id, SENSOR_FIFO_TILE_VOLTAGE_TELEMETRY_HW);
    assert_int_equal(collection_hdr_out->number_of_elements, 4);
    assert_int_equal(collection_hdr_out->collection_payload_size, 4 * sizeof(tile_voltage_t));

    assert_int_equal(sfd_pkg_used_size, 120 + sizeof(telemetry_collection_hdr_t) + (4 * sizeof(tile_voltage_t)));
}

TEST_FUNCTION(test_add_core_current_collection, test_setup, nullptr)
{
    sfd_pkg_used_size = 140;
    p_telemetry_collection_hdr_t collection_hdr_out =
        (p_telemetry_collection_hdr_t)(sfd_pkg_start_location + sfd_pkg_used_size);

    will_return(__wrap_sensor_fifo_svc_poll_core_current, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &done_status);

    add_core_current_collection();

    assert_int_equal(collection_hdr_out->provider_id, UINT16_MAX);
    assert_int_equal(collection_hdr_out->element_id, UINT16_MAX);
    assert_int_equal(collection_hdr_out->collection_id, SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW);
    assert_int_equal(collection_hdr_out->number_of_elements, 2);
    assert_int_equal(collection_hdr_out->collection_payload_size, 2 * sizeof(core_current_t));

    assert_int_equal(sfd_pkg_used_size, 140 + sizeof(telemetry_collection_hdr_t) + (2 * sizeof(core_current_t)));
}

TEST_FUNCTION(test_add_soc_pvt_temperature_collection, test_setup, nullptr)
{
    sfd_pkg_used_size = 100;
    p_telemetry_collection_hdr_t collection_hdr_out =
        (p_telemetry_collection_hdr_t)(sfd_pkg_start_location + sfd_pkg_used_size);

    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &done_status);

    add_soc_pvt_temperature_collection();

    assert_int_equal(collection_hdr_out->provider_id, UINT16_MAX);
    assert_int_equal(collection_hdr_out->element_id, UINT16_MAX);
    assert_int_equal(collection_hdr_out->collection_id, SENSOR_FIFO_PVT_TEMP_FW);
    assert_int_equal(collection_hdr_out->number_of_elements, 3);
    assert_int_equal(collection_hdr_out->collection_payload_size, 3 * sizeof(soc_pvt_temp_t));

    assert_int_equal(sfd_pkg_used_size, 100 + sizeof(telemetry_collection_hdr_t) + (3 * sizeof(soc_pvt_temp_t)));
}

TEST_FUNCTION(test_add_dimm_info_collection, test_setup, nullptr)
{
    sfd_pkg_used_size = 80;
    p_telemetry_collection_hdr_t collection_hdr_out =
        (p_telemetry_collection_hdr_t)(sfd_pkg_start_location + sfd_pkg_used_size);

    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &done_status);

    add_dimm_info_collection();

    assert_int_equal(collection_hdr_out->provider_id, UINT16_MAX);
    assert_int_equal(collection_hdr_out->element_id, UINT16_MAX);
    assert_int_equal(collection_hdr_out->collection_id, SENSOR_FIFO_DIMM_TEMP_FW);
    assert_int_equal(collection_hdr_out->number_of_elements, 3);
    assert_int_equal(collection_hdr_out->collection_payload_size, 3 * sizeof(sensor_ram_dimm_info_t));

    assert_int_equal(sfd_pkg_used_size, 80 + sizeof(telemetry_collection_hdr_t) + (3 * sizeof(sensor_ram_dimm_info_t)));
}

TEST_FUNCTION(test_add_vr_temperature_collection, test_setup, nullptr)
{
    sfd_pkg_used_size = 200;
    p_telemetry_collection_hdr_t collection_hdr_out =
        (p_telemetry_collection_hdr_t)(sfd_pkg_start_location + sfd_pkg_used_size);

    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &done_status);

    add_vr_temperature_collection();

    assert_int_equal(collection_hdr_out->provider_id, UINT16_MAX);
    assert_int_equal(collection_hdr_out->element_id, UINT16_MAX);
    assert_int_equal(collection_hdr_out->collection_id, SENSOR_FIFO_VR_TEMP_FW);
    assert_int_equal(collection_hdr_out->number_of_elements, 2);
    assert_int_equal(collection_hdr_out->collection_payload_size, 2 * sizeof(vr_temp_t));

    assert_int_equal(sfd_pkg_used_size, 200 + sizeof(telemetry_collection_hdr_t) + (2 * sizeof(vr_temp_t)));
}

TEST_FUNCTION(test_add_vr_current_collection, test_setup, nullptr)
{
    sfd_pkg_used_size = 100;
    p_telemetry_collection_hdr_t collection_hdr_out =
        (p_telemetry_collection_hdr_t)(sfd_pkg_start_location + sfd_pkg_used_size);

    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &done_status);

    add_vr_current_collection();

    assert_int_equal(collection_hdr_out->provider_id, UINT16_MAX);
    assert_int_equal(collection_hdr_out->element_id, UINT16_MAX);
    assert_int_equal(collection_hdr_out->collection_id, SENSOR_FIFO_VR_CURRENT_FW);
    assert_int_equal(collection_hdr_out->number_of_elements, 4);
    assert_int_equal(collection_hdr_out->collection_payload_size, 4 * sizeof(vr_current_t));

    assert_int_equal(sfd_pkg_used_size, 100 + sizeof(telemetry_collection_hdr_t) + (4 * sizeof(vr_current_t)));
}

TEST_FUNCTION(test_add_soc_pvt_voltage_collection, test_setup, nullptr)
{
    sfd_pkg_used_size = 60;
    p_telemetry_collection_hdr_t collection_hdr_out =
        (p_telemetry_collection_hdr_t)(sfd_pkg_start_location + sfd_pkg_used_size);

    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_voltage, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_voltage, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_voltage, &done_status);

    add_soc_pvt_voltage_collection();

    assert_int_equal(collection_hdr_out->provider_id, UINT16_MAX);
    assert_int_equal(collection_hdr_out->element_id, UINT16_MAX);
    assert_int_equal(collection_hdr_out->collection_id, SENSOR_FIFO_PVT_VOLTAGE_FW);
    assert_int_equal(collection_hdr_out->number_of_elements, 2);
    assert_int_equal(collection_hdr_out->collection_payload_size, 2 * sizeof(soc_pvt_voltage_t));

    assert_int_equal(sfd_pkg_used_size, 60 + sizeof(telemetry_collection_hdr_t) + (2 * sizeof(soc_pvt_voltage_t)));
}

TEST_FUNCTION(testin_band_tlm_cmpnt_sample_sensor_fifo_dbg_data, test_setup, nullptr)
{
    sfd_pkg_used_size = 200;

    // test_setup sets all of mocked_is_empty to true

    will_return(__wrap_sensor_fifo_svc_is_empty, mocked_is_empty);

    in_band_tlm_cmpnt_sample_sensor_fifo_dbg_data();

    // verify unchanged since no data in an fifo
    assert_int_equal(sfd_pkg_used_size, 200);
}

TEST_FUNCTION(test_functional_pkg, test_setup, nullptr)
{
    sfd_package_number = 48;

    in_band_tlm_cmpnt_begin_sensor_fifo_dbg_collection();

    for (uint16_t i = 0; i < SENSOR_FIFO_MAX_ID; i++)
    {
        mocked_is_empty[i] = false;
    }

    will_return(__wrap_sensor_fifo_svc_is_empty, mocked_is_empty);

    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &last_status);

    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &last_status);

    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &last_status);

    will_return(__wrap_sensor_fifo_svc_poll_core_current, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &last_status);

    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &last_status);

    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_voltage, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_voltage, &last_status);

    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &last_status);

    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &last_status);

    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &last_status);

    in_band_tlm_cmpnt_sample_sensor_fifo_dbg_data();

    will_return(__wrap_sensor_fifo_svc_is_empty, mocked_is_empty);

    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &last_status);

    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &last_status);

    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &last_status);

    will_return(__wrap_sensor_fifo_svc_poll_core_current, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_core_current, &last_status);

    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_temperature, &last_status);

    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_voltage, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_soc_pvt_voltage, &last_status);

    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_dimm_info, &last_status);

    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &last_status);

    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &last_status);
    in_band_tlm_cmpnt_sample_sensor_fifo_dbg_data();

    // setup for mts_manager_queue_tlm_package()
    FpFwListInitialize(&pkg_free_list);
    for (size_t i = 0; i < MAX_PENDING_PACKAGES; i++)
    {
        FpFwListEntryInitialize(&mts_active_pkg_buffer[i].list_entry);
        FpFwListInsertTail(&pkg_free_list, &mts_active_pkg_buffer[i].list_entry);
    }
    FpFwListInitialize(&pkg_active_list);

    // test notification of adding to empty list and notifying host
    will_return(__wrap_mts_is_primary_instance, true);
    expect_function_call(__wrap_mts_client_send_dcp_notification);

    in_band_tlm_cmpnt_end_sensor_fifo_dbg_collection();

    p_telemetry_package_hdr_t package_hdr = (p_telemetry_package_hdr_t)sfd_pkg_start_location;

    uint32_t expected_pkg_payload_size =
        (2 * sizeof(telemetry_record_hdr_t)) + (18 * sizeof(telemetry_collection_hdr_t)) +
        (4 * sizeof(pstate_telem_t)) + (4 * sizeof(tile_temp_t)) + (4 * sizeof(tile_voltage_t)) +
        (4 * sizeof(core_current_t)) + (4 * sizeof(soc_pvt_temp_t)) + (4 * sizeof(soc_pvt_voltage_t)) +
        (4 * sizeof(sensor_ram_dimm_info_t)) + (4 * sizeof(vr_temp_t)) + (4 * sizeof(vr_current_t));

    assert_int_equal(package_hdr->payload_header.source_die, 0);
    assert_int_equal(package_hdr->payload_header.package_number, 48);
    assert_int_equal(package_hdr->payload_header.number_of_records, 2);
    assert_int_equal(package_hdr->payload_header.package_payload_size, expected_pkg_payload_size);

    assert_int_equal(sfd_pkg_used_size, sizeof(telemetry_package_hdr_t) + expected_pkg_payload_size);
}

TEST_FUNCTION(test_functional_record, test_setup, nullptr)
{
    in_band_tlm_cmpnt_begin_sensor_fifo_dbg_collection();

    sfd_package_number = 100;
    sfd_pkg_used_size = 200;
    sfd_next_record_number = 38;

    p_telemetry_record_hdr_t record_hdr = (p_telemetry_record_hdr_t)(sfd_pkg_start_location + sfd_pkg_used_size);

    mocked_is_empty[SENSOR_FIFO_VR_TEMP_FW] = false;
    mocked_is_empty[SENSOR_FIFO_VR_CURRENT_FW] = false;

    will_return(__wrap_sensor_fifo_svc_is_empty, mocked_is_empty);

    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_vr_temperature, &last_status);

    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &more_status);
    will_return(__wrap_sensor_fifo_svc_poll_vr_current, &last_status);

    in_band_tlm_cmpnt_sample_sensor_fifo_dbg_data();

    assert_int_equal(record_hdr->record_number, 38);
    assert_int_equal(record_hdr->number_of_collections, 2);
    assert_int_equal(record_hdr->record_payload_size,
                     (2 * sizeof(telemetry_collection_hdr_t)) + (2 * sizeof(vr_temp_t)) + (2 * sizeof(vr_current_t)));
}

TEST_FUNCTION(test_not_enough_space, test_setup, nullptr)
{
    // test_setup ensures there is less than MAX_RECORD_RESERVATION_SIZE bytes available
    sfd_pkg_used_size = 300;
    sfd_next_record_number = 5;

    p_telemetry_package_hdr_t package_hdr = (p_telemetry_package_hdr_t)sfd_pkg_start_location;

    mocked_is_empty[SENSOR_FIFO_VR_TEMP_FW] = false;
    mocked_is_empty[SENSOR_FIFO_VR_CURRENT_FW] = false;

    will_return(__wrap_sensor_fifo_svc_is_empty, mocked_is_empty);

    // setup for mts_manager_queue_tlm_package()
    FpFwListInitialize(&pkg_free_list);
    for (size_t i = 0; i < MAX_PENDING_PACKAGES; i++)
    {
        FpFwListEntryInitialize(&mts_active_pkg_buffer[i].list_entry);
        FpFwListInsertTail(&pkg_free_list, &mts_active_pkg_buffer[i].list_entry);
    }
    FpFwListInitialize(&pkg_active_list);

    // test notification of adding to empty list and notifying host
    will_return(__wrap_mts_is_primary_instance, true);
    expect_function_call(__wrap_mts_client_send_dcp_notification);

    expect_function_call(exec_tlm_cmpnt_change_telemetry_mode);

    in_band_tlm_cmpnt_sample_sensor_fifo_dbg_data();

    assert_int_equal(package_hdr->payload_header.number_of_records, 4);
    assert_int_equal(package_hdr->payload_header.package_payload_size, 300 - sizeof(telemetry_package_hdr_t));
}
