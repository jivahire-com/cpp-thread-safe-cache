//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_create_package.cpp
 * Test telemetry package creation
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <in_band_tlm_cmpnt_i.h>
#include <package_creation_i.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t

}
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);
    return 0;
}

static int test_teardown(void** pContext)
{
    FPFW_UNUSED(pContext);
    return 0;
}

void assert_memset_to_ff(const uint8_t *array, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        assert_int_equal(array[i], 0xFF);  // Ensure each element is 0xFF
    }
}


/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

TEST_FUNCTION(test_get_pwr_core_pstate_data, test_setup, test_teardown)
{
    pwr_core_record_pstate_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_pstate_data, NUMBER_OF_CORES_PER_DIE);
    create_pwr_core_pstate_record(&record);

    assert_int_not_equal(record.record_header.timestamp, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size, (sizeof(pwr_core_record_pstate_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(record.pstate_collection[core_id].collection_header.provider_id,  EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM);
        assert_int_equal(record.pstate_collection[core_id].collection_header.event_id, POWER_TELEMETRY_EVENT_CORE_PSTATE);
        assert_int_equal(record.pstate_collection[core_id].collection_header.collection_id, core_id);
        assert_int_equal(record.pstate_collection[core_id].collection_header.number_of_events, NUMBER_OF_PSTATES);
        assert_int_equal(record.pstate_collection[core_id].collection_header.collection_payload_size, sizeof(pwr_core_collection_pstate_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t *)record.pstate_collection[core_id].pstate_event, sizeof(pwr_core_event_pstate_t) * NUMBER_OF_PSTATES);
    }
}

TEST_FUNCTION(test_get_pwr_core_cstate_data, test_setup, test_teardown)
{
    pwr_core_record_cstate_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_cstate_data, NUMBER_OF_CORES_PER_DIE);
    create_pwr_core_cstate_record(&record);

    assert_int_not_equal(record.record_header.timestamp, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size, (sizeof(pwr_core_record_cstate_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(record.cstate_collection[core_id].collection_header.provider_id,  EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM);
        assert_int_equal(record.cstate_collection[core_id].collection_header.event_id, POWER_TELEMETRY_EVENT_CORE_CSTATE);
        assert_int_equal(record.cstate_collection[core_id].collection_header.collection_id, core_id);
        assert_int_equal(record.cstate_collection[core_id].collection_header.number_of_events, NUMBER_OF_CSTATES);
        assert_int_equal(record.cstate_collection[core_id].collection_header.collection_payload_size, sizeof(pwr_core_collection_cstate_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t *)record.cstate_collection[core_id].cstate_event, sizeof(pwr_core_event_cstate_t) * NUMBER_OF_CSTATES);
    }
}

TEST_FUNCTION(test_get_pwr_core_throttle_data, test_setup, test_teardown)
{
    pwr_core_record_throttle_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_throttle_data, NUMBER_OF_CORES_PER_DIE);
    create_pwr_core_throttle_record(&record);

    assert_int_not_equal(record.record_header.timestamp, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size, (sizeof(pwr_core_record_throttle_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(record.throttle_collection[core_id].collection_header.provider_id,  EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM);
        assert_int_equal(record.throttle_collection[core_id].collection_header.event_id, POWER_TELEMETRY_EVENT_CORE_THROTTLE);
        assert_int_equal(record.throttle_collection[core_id].collection_header.collection_id, core_id);
        assert_int_equal(record.throttle_collection[core_id].collection_header.number_of_events, NUMBER_OF_THROTTLE_TYPES);
        assert_int_equal(record.throttle_collection[core_id].collection_header.collection_payload_size, sizeof(pwr_core_collection_throttle_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t *)record.throttle_collection[core_id].throttle_event, sizeof(pwr_core_event_throttle_t) * NUMBER_OF_THROTTLE_TYPES);
    }
}

TEST_FUNCTION(test_get_pwr_core_rack_priority_data, test_setup, test_teardown)
{
    pwr_core_record_rack_priorities_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_rack_priority_data, NUMBER_OF_CORES_PER_DIE);
    create_pwr_core_rack_priority_record(&record);

    assert_int_not_equal(record.record_header.timestamp, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size, (sizeof(pwr_core_record_rack_priorities_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(record.rack_priority_collection[core_id].collection_header.provider_id,  EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM);
        assert_int_equal(record.rack_priority_collection[core_id].collection_header.event_id, POWER_TELEMETRY_EVENT_CORE_RACK_PRIORITIES);
        assert_int_equal(record.rack_priority_collection[core_id].collection_header.collection_id, core_id);
        assert_int_equal(record.rack_priority_collection[core_id].collection_header.number_of_events, NUMBER_OF_RACK_PRIORITIES);
        assert_int_equal(record.rack_priority_collection[core_id].collection_header.collection_payload_size, sizeof(pwr_core_collection_rack_priorities_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t *)record.rack_priority_collection[core_id].rack_priority_event, sizeof(pwr_core_event_rack_priorities_t) * NUMBER_OF_RACK_PRIORITIES);
    }
}

TEST_FUNCTION(test_get_pwr_core_voltage_data, test_setup, test_teardown)
{
    pwr_core_record_voltage_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_voltage_data, NUMBER_OF_CORES_PER_DIE);
    create_pwr_core_voltage_record(&record);

    assert_int_not_equal(record.record_header.timestamp, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size, (sizeof(pwr_core_record_voltage_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(record.voltage_collection[core_id].collection_header.provider_id,  EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM);
        assert_int_equal(record.voltage_collection[core_id].collection_header.event_id, POWER_TELEMETRY_EVENT_CORE_VOLTAGE);
        assert_int_equal(record.voltage_collection[core_id].collection_header.collection_id, core_id);
        assert_int_equal(record.voltage_collection[core_id].collection_header.number_of_events, 1);
        assert_int_equal(record.voltage_collection[core_id].collection_header.collection_payload_size, sizeof(pwr_core_collection_voltage_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t *)&record.voltage_collection[core_id].voltage_event, sizeof(pwr_core_event_voltage_t));
    }
}

TEST_FUNCTION(test_get_pwr_core_current_data, test_setup, test_teardown)
{
    pwr_core_record_current_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_current_data, NUMBER_OF_CORES_PER_DIE);
    create_pwr_core_current_record(&record);

    assert_int_not_equal(record.record_header.timestamp, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size, (sizeof(pwr_core_record_current_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(record.current_collection[core_id].collection_header.provider_id,  EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM);
        assert_int_equal(record.current_collection[core_id].collection_header.event_id, POWER_TELEMETRY_EVENT_CORE_CURRENT);
        assert_int_equal(record.current_collection[core_id].collection_header.collection_id, core_id);
        assert_int_equal(record.current_collection[core_id].collection_header.number_of_events, 1);
        assert_int_equal(record.current_collection[core_id].collection_header.collection_payload_size, sizeof(pwr_core_collection_current_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t *)&record.current_collection[core_id].current_event, sizeof(pwr_core_event_current_t));
    }
}

TEST_FUNCTION(test_get_pwr_core_temperature_data, test_setup, test_teardown)
{
    pwr_core_record_temperature_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_temperature_data, NUMBER_OF_CORES_PER_DIE);
    create_pwr_core_temperature_record(&record);

    assert_int_not_equal(record.record_header.timestamp, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size, (sizeof(pwr_core_record_temperature_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(record.temperature_collection[core_id].collection_header.provider_id,  EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM);
        assert_int_equal(record.temperature_collection[core_id].collection_header.event_id, POWER_TELEMETRY_EVENT_CORE_TEMPERATURE);
        assert_int_equal(record.temperature_collection[core_id].collection_header.collection_id, core_id);
        assert_int_equal(record.temperature_collection[core_id].collection_header.number_of_events, 1);
        assert_int_equal(record.temperature_collection[core_id].collection_header.collection_payload_size, sizeof(pwr_core_collection_temperature_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t *)&record.temperature_collection[core_id].temperature_event, sizeof(pwr_core_event_temperature_t));
    }
}


TEST_FUNCTION(test_get_pwr_core_histogram_data, test_setup, test_teardown)
{
    pwr_core_record_histogram_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_histogram_data, NUMBER_OF_CORES_PER_DIE);
    create_pwr_core_histogram_record(&record);

    assert_int_not_equal(record.record_header.timestamp, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size, (sizeof(pwr_core_record_histogram_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(record.histogram_collection[core_id].collection_header.provider_id,  EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM);
        assert_int_equal(record.histogram_collection[core_id].collection_header.event_id, POWER_TELEMETRY_EVENT_CORE_HISTOGRAM);
        assert_int_equal(record.histogram_collection[core_id].collection_header.collection_id, core_id);
        assert_int_equal(record.histogram_collection[core_id].collection_header.number_of_events, NUMBER_OF_HS_VOLTAGE_SCALES * NUMBER_OF_HS_TEMP_SCALES);
        assert_int_equal(record.histogram_collection[core_id].collection_header.collection_payload_size, sizeof(pwr_core_collection_histogram_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t *)record.histogram_collection[core_id].histogram_event, sizeof(pwr_core_event_histogram_t) * NUMBER_OF_HS_VOLTAGE_SCALES * NUMBER_OF_HS_TEMP_SCALES);
    }
}

TEST_FUNCTION(test_get_pwr_soc_pc3_data, test_setup, test_teardown)
{
    pwr_soc_record_pc3_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_soc_pc3_data, 1);
    create_pwr_soc_pc3_record(&record);

    assert_int_not_equal(record.record_header.timestamp, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, 1);
    assert_int_equal(record.record_header.record_payload_size, (sizeof(pwr_soc_record_pc3_t) - sizeof(telemetry_record_hdr_t)));

    assert_int_equal(record.pc3_collection.collection_header.provider_id,  EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM);
    assert_int_equal(record.pc3_collection.collection_header.event_id, POWER_TELEMETRY_EVENT_SOC_PC3);
    assert_int_equal(record.pc3_collection.collection_header.collection_id, 1);
    assert_int_equal(record.pc3_collection.collection_header.number_of_events, 1);
    assert_int_equal(record.pc3_collection.collection_header.collection_payload_size, sizeof(pwr_soc_collection_pc3_t) - sizeof(telemetry_collection_hdr_t));

    // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
    // This verifies that the correct data ranges are passed to the data processing component get data api's
     assert_memset_to_ff((uint8_t *)&record.pc3_collection.pc3_event, sizeof(pwr_soc_event_pc3_t));
}

TEST_FUNCTION(test_get_pwr_soc_vr_rail_data, test_setup, test_teardown)
{
    pwr_soc_record_vr_rail_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_soc_vr_rail_data, MAX_NUM_OF_VR_RAILS);
    create_pwr_soc_vr_rail_record(&record);

    assert_int_not_equal(record.record_header.timestamp, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, MAX_NUM_OF_VR_RAILS);
    assert_int_equal(record.record_header.record_payload_size, (sizeof(pwr_soc_record_vr_rail_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t rail_id = 0; rail_id < MAX_NUM_OF_VR_RAILS; rail_id++)
    {
        assert_int_equal(record.rail_collection[rail_id].collection_header.provider_id,  EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM);
        assert_int_equal(record.rail_collection[rail_id].collection_header.event_id, POWER_TELEMETRY_EVENT_SOC_VR_RAILS);
        assert_int_equal(record.rail_collection[rail_id].collection_header.collection_id, rail_id);
        assert_int_equal(record.rail_collection[rail_id].collection_header.number_of_events, 1);
        assert_int_equal(record.rail_collection[rail_id].collection_header.collection_payload_size, sizeof(pwr_soc_collection_vr_rail_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t *)&record.rail_collection[rail_id].rail_event, sizeof(pwr_soc_event_vr_rail_t));
    }
}

TEST_FUNCTION(test_get_pwr_soc_hnf_data, test_setup, test_teardown)
{
    pwr_soc_record_hnf_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_soc_hnf_data, NUMBER_OF_HNF_CHANNELS_PER_DIE);
    create_pwr_soc_hnf_record(&record);

    assert_int_not_equal(record.record_header.timestamp, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_HNF_CHANNELS_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size, (sizeof(pwr_soc_record_hnf_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t hnf_chan = 0; hnf_chan < NUMBER_OF_HNF_CHANNELS_PER_DIE; hnf_chan++)
    {
        assert_int_equal(record.hnf_collection[hnf_chan].collection_header.provider_id,  EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM);
        assert_int_equal(record.hnf_collection[hnf_chan].collection_header.event_id, POWER_TELEMETRY_EVENT_SOC_HNF);
        assert_int_equal(record.hnf_collection[hnf_chan].collection_header.collection_id, hnf_chan);
        assert_int_equal(record.hnf_collection[hnf_chan].collection_header.number_of_events, 1);
        assert_int_equal(record.hnf_collection[hnf_chan].collection_header.collection_payload_size, sizeof(pwr_soc_collection_hnf_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t *)&record.hnf_collection[hnf_chan].hnf_event, sizeof(pwr_soc_event_hnf_t));
    }
}

TEST_FUNCTION(test_get_pwr_soc_dimm_data, test_setup, test_teardown)
{
    pwr_soc_record_dimm_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_soc_dimm_data, NUMBER_OF_DIMM_MODULES);
    create_pwr_soc_dimm_record(&record);

    assert_int_not_equal(record.record_header.timestamp, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_DIMM_MODULES);
    assert_int_equal(record.record_header.record_payload_size, (sizeof(pwr_soc_record_dimm_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t dimm_module = 0; dimm_module < NUMBER_OF_DIMM_MODULES; dimm_module++)
    {
        assert_int_equal(record.dimm_collection[dimm_module].collection_header.provider_id,  EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM);
        assert_int_equal(record.dimm_collection[dimm_module].collection_header.event_id, POWER_TELEMETRY_EVENT_SOC_DIMM);
        assert_int_equal(record.dimm_collection[dimm_module].collection_header.collection_id, dimm_module);
        assert_int_equal(record.dimm_collection[dimm_module].collection_header.number_of_events, 1);
        assert_int_equal(record.dimm_collection[dimm_module].collection_header.collection_payload_size, sizeof(pwr_soc_collection_dimm_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t *)&record.dimm_collection[dimm_module].dimm_event, sizeof(pwr_soc_event_dimm_t));
    }
}

TEST_FUNCTION(test_get_pwr_soc_sensor_temp_data, test_setup, test_teardown)
{
    pwr_soc_record_sensor_temp_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_soc_snsr_temp_data, NUMBER_OF_SOC_TEMP_SENSORS);
    create_pwr_soc_sensor_temp_record(&record);

    assert_int_not_equal(record.record_header.timestamp, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_SOC_TEMP_SENSORS);
    assert_int_equal(record.record_header.record_payload_size, (sizeof(pwr_soc_record_sensor_temp_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t sensor_id = 0; sensor_id < NUMBER_OF_SOC_TEMP_SENSORS; sensor_id++)
    {
        assert_int_equal(record.sensor_temp_collection[sensor_id].collection_header.provider_id,  EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM);
        assert_int_equal(record.sensor_temp_collection[sensor_id].collection_header.event_id, POWER_TELEMETRY_EVENT_SOC_SENSOR_TEMP);
        assert_int_equal(record.sensor_temp_collection[sensor_id].collection_header.collection_id, sensor_id);
        assert_int_equal(record.sensor_temp_collection[sensor_id].collection_header.number_of_events, 1);
        assert_int_equal(record.sensor_temp_collection[sensor_id].collection_header.collection_payload_size, sizeof(pwr_soc_collection_sensor_temp_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t *)&record.sensor_temp_collection[sensor_id].sensor_temp_event, sizeof(pwr_soc_event_sensor_temp_t));
    }
}

TEST_FUNCTION(test_get_pwr_mpam_pstate_data, test_setup, test_teardown)
{
    pwr_record_mpam_pstate_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_mpam_pstate_data, NUMBER_OF_MPAMS);
    create_pwr_mpam_pstate_record(&record);

    assert_int_not_equal(record.record_header.timestamp, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_MPAMS);
    assert_int_equal(record.record_header.record_payload_size, (sizeof(pwr_record_mpam_pstate_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t mpam_id = 0; mpam_id < NUMBER_OF_MPAMS; mpam_id++)
    {
        assert_int_equal(record.mpam_pstate_collection[mpam_id].collection_header.provider_id,  EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM);
        assert_int_equal(record.mpam_pstate_collection[mpam_id].collection_header.event_id, POWER_TELEMETRY_EVENT_MPAM);
        assert_int_equal(record.mpam_pstate_collection[mpam_id].collection_header.collection_id, mpam_id);
        assert_int_equal(record.mpam_pstate_collection[mpam_id].collection_header.number_of_events, NUMBER_OF_PSTATES);
        assert_int_equal(record.mpam_pstate_collection[mpam_id].collection_header.collection_payload_size, sizeof(pwr_collection_mpam_pstate_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t *)&record.mpam_pstate_collection[mpam_id].mpam_pstate_event, sizeof(pwr_event_mpam_pstate_t) * NUMBER_OF_PSTATES);
    }
}

TEST_FUNCTION(test_get_pwr_mpam_throttle_data, test_setup, test_teardown)
{
    pwr_record_mpam_throttle_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_soc_mpam_throttle_data, NUMBER_OF_MPAMS);
    create_pwr_mpam_throttle_record(&record);

    assert_int_not_equal(record.record_header.timestamp, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_MPAMS);
    assert_int_equal(record.record_header.record_payload_size, (sizeof(pwr_record_mpam_throttle_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t mpam_id = 0; mpam_id < NUMBER_OF_MPAMS; mpam_id++)
    {
        assert_int_equal(record.mpam_throttle_collection[mpam_id].collection_header.provider_id,  EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM);
        assert_int_equal(record.mpam_throttle_collection[mpam_id].collection_header.event_id, POWER_TELEMETRY_EVENT_MPAM_THROTTLE);
        assert_int_equal(record.mpam_throttle_collection[mpam_id].collection_header.collection_id, mpam_id);
        assert_int_equal(record.mpam_throttle_collection[mpam_id].collection_header.number_of_events, 1);
        assert_int_equal(record.mpam_throttle_collection[mpam_id].collection_header.collection_payload_size, sizeof(pwr_collection_mpam_throttle_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t *)&record.mpam_throttle_collection[mpam_id].mpam_throttle_event, sizeof(pwr_event_mpam_throttle_t));
    }
}

TEST_FUNCTION(test_get_perf_core_summary_data, test_setup, test_teardown)
{
    perf_core_record_summary_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_perf_soc_core_summary_data, NUMBER_OF_CORES_PER_DIE);
    create_perf_core_summary_record(&record);

    assert_int_not_equal(record.record_header.timestamp, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size, (sizeof(perf_core_record_summary_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(record.perf_core_summary_collection[core_id].collection_header.provider_id,  EVENT_TRACE_PROVIDER_ID_MCP_PERF_TLM);
        assert_int_equal(record.perf_core_summary_collection[core_id].collection_header.event_id, PERFORMANCE_TELEMETRY_EVENT_CORE);
        assert_int_equal(record.perf_core_summary_collection[core_id].collection_header.collection_id, core_id);
        assert_int_equal(record.perf_core_summary_collection[core_id].collection_header.number_of_events, 1);
        assert_int_equal(record.perf_core_summary_collection[core_id].collection_header.collection_payload_size, sizeof(perf_core_collection_summary_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t *)&record.perf_core_summary_collection[core_id].summary_event, sizeof(perf_core_event_summary_t));
    }
}

TEST_FUNCTION(test_get_perf_soc_rail_data, test_setup, test_teardown)
{
    perf_soc_record_rail_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_perf_soc_rail_data, MAX_NUM_OF_VR_RAILS);
    create_perf_soc_rail_record(&record);

    assert_int_not_equal(record.record_header.timestamp, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, MAX_NUM_OF_VR_RAILS);
    assert_int_equal(record.record_header.record_payload_size, (sizeof(perf_soc_record_rail_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t rail_id = 0; rail_id < MAX_NUM_OF_VR_RAILS; rail_id++)
    {
        assert_int_equal(record.rail_collection[rail_id].collection_header.provider_id,  EVENT_TRACE_PROVIDER_ID_MCP_PERF_TLM);
        assert_int_equal(record.rail_collection[rail_id].collection_header.event_id, PERFORMANCE_TELEMETRY_EVENT_SOC_RAILS);
        assert_int_equal(record.rail_collection[rail_id].collection_header.collection_id, rail_id);
        assert_int_equal(record.rail_collection[rail_id].collection_header.number_of_events, 1);
        assert_int_equal(record.rail_collection[rail_id].collection_header.collection_payload_size, sizeof(perf_soc_collection_rail_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t *)&record.rail_collection[rail_id].rail_event, sizeof(perf_soc_event_rail_t));
    }
}

TEST_FUNCTION(test_get_perf_soc_dimm_runtime_data, test_setup, test_teardown)
{
    perf_soc_record_dimm_runtime_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_perf_soc_dimm_runtime_data, NUMBER_OF_DIMM_MODULES);
    create_perf_soc_dimm_runtime_record(&record);

    assert_int_not_equal(record.record_header.timestamp, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_DIMM_MODULES);
    assert_int_equal(record.record_header.record_payload_size, (sizeof(perf_soc_record_dimm_runtime_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t module_id = 0; module_id < NUMBER_OF_DIMM_MODULES; module_id++)
    {
        assert_int_equal(record.dimm_collection[module_id].collection_header.provider_id,  EVENT_TRACE_PROVIDER_ID_MCP_PERF_TLM);
        assert_int_equal(record.dimm_collection[module_id].collection_header.event_id, PERFORMANCE_TELEMETRY_EVENT_SOC_DIMM_RT);
        assert_int_equal(record.dimm_collection[module_id].collection_header.collection_id, module_id);
        assert_int_equal(record.dimm_collection[module_id].collection_header.number_of_events, 1);
        assert_int_equal(record.dimm_collection[module_id].collection_header.collection_payload_size, sizeof(perf_soc_collection_dimm_runtime_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t *)&record.dimm_collection[module_id].dimm_rt_event, sizeof(perf_soc_event_dimm_runtime_t));
    }
}

TEST_FUNCTION(test_get_perf_soc_dimm_config_data, test_setup, test_teardown)
{
    perf_soc_record_dimm_config_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_perf_soc_dimm_config_data, 1);
    create_perf_soc_dimm_config_record(&record);

    assert_int_not_equal(record.record_header.timestamp, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, 1);
    assert_int_equal(record.record_header.record_payload_size, (sizeof(perf_soc_record_dimm_config_t) - sizeof(telemetry_record_hdr_t)));


    assert_int_equal(record.dimm_config_collection.collection_header.provider_id,  EVENT_TRACE_PROVIDER_ID_MCP_PERF_TLM);
    assert_int_equal(record.dimm_config_collection.collection_header.event_id, PERFORMANCE_TELEMETRY_EVENT_SOC_DIMM_CONFIG);
    assert_int_equal(record.dimm_config_collection.collection_header.collection_id, 1);
    assert_int_equal(record.dimm_config_collection.collection_header.number_of_events, 1);
    assert_int_equal(record.dimm_config_collection.collection_header.collection_payload_size, sizeof(perf_soc_collection_dimm_config_t) - sizeof(telemetry_collection_hdr_t));

    // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
    // This verifies that the correct data ranges are passed to the data processing component get data api's
    assert_memset_to_ff((uint8_t *)&record.dimm_config_collection.dimm_config_event, sizeof(perf_soc_event_dimm_config_t));
}


TEST_FUNCTION(test_get_perf_soc_sensor_temp_data, test_setup, test_teardown)
{
    perf_soc_record_sensor_temp_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_perf_soc_snsr_temp_data, NUMBER_OF_SOC_TEMP_SENSORS);
    create_perf_soc_sensor_temp_record(&record);

    assert_int_not_equal(record.record_header.timestamp, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_SOC_TEMP_SENSORS);
    assert_int_equal(record.record_header.record_payload_size, (sizeof(perf_soc_record_sensor_temp_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t sensor_id = 0; sensor_id < NUMBER_OF_SOC_TEMP_SENSORS; sensor_id++)
    {
        assert_int_equal(record.temperature_collection[sensor_id].collection_header.provider_id,  EVENT_TRACE_PROVIDER_ID_MCP_PERF_TLM);
        assert_int_equal(record.temperature_collection[sensor_id].collection_header.event_id, PERFORMANCE_TELEMETRY_EVENT_SOC_TEMP_SENSOR);
        assert_int_equal(record.temperature_collection[sensor_id].collection_header.collection_id, sensor_id);
        assert_int_equal(record.temperature_collection[sensor_id].collection_header.number_of_events, 1);
        assert_int_equal(record.temperature_collection[sensor_id].collection_header.collection_payload_size, sizeof(perf_soc_collection_sensor_temp_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t *)&record.temperature_collection[sensor_id].temperature_event, sizeof(perf_soc_event_sensor_temp_t));
    }
}

TEST_FUNCTION(test_get_perf_core_amu_counters_data, test_setup, test_teardown)
{
    perf_core_record_amu_counters_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_perf_core_amu_data, NUMBER_OF_CORES_PER_DIE);
    create_perf_core_amu_counters_record(&record);

    assert_int_not_equal(record.record_header.timestamp, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size, (sizeof(perf_core_record_amu_counters_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(record.amu_counter_collection[core_id].collection_header.provider_id,  EVENT_TRACE_PROVIDER_ID_MCP_PERF_TLM);
        assert_int_equal(record.amu_counter_collection[core_id].collection_header.event_id, PERFORMANCE_TELEMETRY_EVENT_AMU);
        assert_int_equal(record.amu_counter_collection[core_id].collection_header.collection_id, core_id);
        assert_int_equal(record.amu_counter_collection[core_id].collection_header.number_of_events, 1);
        assert_int_equal(record.amu_counter_collection[core_id].collection_header.collection_payload_size, sizeof(perf_core_collection_amu_counters_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t *)&record.amu_counter_collection[core_id].amu_counter_event, sizeof(perf_core_event_amu_counters_t));
    }
}

TEST_FUNCTION(test_package_create_power_report_none_enabled, test_setup, test_teardown)
{
    power_report_full_package_t package = {{{0}}};

    for(uint16_t i = 0; i < POWER_TELEMETRY_EVENT_ID_MAX; i++)
    {
        power_report_event_enable[i] = false;
    }

    fpfw_status_t status = package_create_power_report((uintptr_t)&package, sizeof(power_report_full_package_t));

    assert_int_equal(status, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_package_create_power_report_all_enabled, test_setup, test_teardown)
{
    power_report_full_package_t package = {{{0}}};

    for(uint16_t i = 0; i < POWER_TELEMETRY_EVENT_ID_MAX; i++)
    {
        power_report_event_enable[i] = true;
    }

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_pstate_data, NUMBER_OF_CORES_PER_DIE);
    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_cstate_data, NUMBER_OF_CORES_PER_DIE);
    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_throttle_data, NUMBER_OF_CORES_PER_DIE);
    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_rack_priority_data, NUMBER_OF_CORES_PER_DIE);
    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_voltage_data, NUMBER_OF_CORES_PER_DIE);
    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_current_data, NUMBER_OF_CORES_PER_DIE);
    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_temperature_data, NUMBER_OF_CORES_PER_DIE);
    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_histogram_data, NUMBER_OF_CORES_PER_DIE);
    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_soc_pc3_data, 1);
    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_soc_vr_rail_data, MAX_NUM_OF_VR_RAILS);
    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_soc_hnf_data, NUMBER_OF_HNF_CHANNELS_PER_DIE);
    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_soc_dimm_data, NUMBER_OF_DIMM_MODULES);
    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_soc_snsr_temp_data, NUMBER_OF_SOC_TEMP_SENSORS);
    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_mpam_pstate_data, NUMBER_OF_MPAMS);
    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_soc_mpam_throttle_data, NUMBER_OF_MPAMS);

    fpfw_status_t status = package_create_power_report((uintptr_t)&package, sizeof(power_report_full_package_t));

    assert_int_equal(status, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_package_create_power_report_some_enabled, test_setup, test_teardown)
{
    typedef struct
    {
        telemetry_package_hdr_t package_header;
        pwr_core_record_pstate_t pstate_record;
        pwr_soc_record_pc3_t pc3_record;
    } power_report_partial_package_t;

    power_report_partial_package_t package = {{{0}}};

    for(uint16_t i = 0; i < POWER_TELEMETRY_EVENT_ID_MAX; i++)
    {
        power_report_event_enable[i] = false;
    }

    power_report_event_enable[POWER_TELEMETRY_EVENT_CORE_PSTATE] = true;
    power_report_event_enable[POWER_TELEMETRY_EVENT_SOC_PC3] = true;

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_pstate_data, NUMBER_OF_CORES_PER_DIE);
    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_soc_pc3_data, 1);

    fpfw_status_t status = package_create_power_report((uintptr_t)&package, sizeof(power_report_partial_package_t));

    // verify the record headers unique fields are correct
    assert_int_equal(package.pstate_record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(package.pstate_record.record_header.record_payload_size, (sizeof(pwr_core_record_pstate_t) - sizeof(telemetry_record_hdr_t)));

    assert_int_equal(package.pc3_record.record_header.number_of_collections, 1);
    assert_int_equal(package.pc3_record.record_header.record_payload_size, (sizeof(pwr_soc_record_pc3_t) - sizeof(telemetry_record_hdr_t)));

    assert_int_equal(status, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_package_create_power_report_too_small, test_setup, test_teardown)
{
    typedef struct
    {
        telemetry_package_hdr_t package_header;
        pwr_core_record_pstate_t pstate_record;
        pwr_soc_record_pc3_t pc3_record;
    } power_report_partial_package_t;

    power_report_partial_package_t package = {{{0}}};

    for(uint16_t i = 0; i < POWER_TELEMETRY_EVENT_ID_MAX; i++)
    {
        power_report_event_enable[i] = false;
    }

    power_report_event_enable[POWER_TELEMETRY_EVENT_CORE_PSTATE] = true;
    power_report_event_enable[POWER_TELEMETRY_EVENT_SOC_PC3] = true;
    power_report_event_enable[POWER_TELEMETRY_EVENT_SOC_VR_RAILS] = true;

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_pstate_data, NUMBER_OF_CORES_PER_DIE);
    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_soc_pc3_data, 1);

    fpfw_status_t status = package_create_power_report((uintptr_t)&package, sizeof(power_report_partial_package_t));


    assert_int_equal(status, FPFW_STATUS_BUFFER_TOO_SMALL);
}

TEST_FUNCTION(test_package_create_perf_report_none_enabled, test_setup, test_teardown)
{
    perf_report_full_package_t package = {{{0}}};

    for(uint16_t i = 0; i < PERFORMANCE_TELEMETRY_EVENT_ID_MAX; i++)
    {
        perf_report_event_enable[i] = false;
    }

    fpfw_status_t status = package_create_perf_report((uintptr_t)&package, sizeof(perf_report_full_package_t));

    assert_int_equal(status, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_package_create_perf_report_all_enabled, test_setup, test_teardown)
{
    perf_report_full_package_t package = {{{0}}};

    for(uint16_t i = 0; i < PERFORMANCE_TELEMETRY_EVENT_ID_MAX; i++)
    {
        perf_report_event_enable[i] = true;
    }
    expect_function_calls(data_proc_tlm_cmpnt_get_perf_soc_core_summary_data, NUMBER_OF_CORES_PER_DIE);
    expect_function_calls(data_proc_tlm_cmpnt_get_perf_soc_rail_data, MAX_NUM_OF_VR_RAILS);
    expect_function_calls(data_proc_tlm_cmpnt_get_perf_soc_dimm_runtime_data, NUMBER_OF_DIMM_MODULES);
    expect_function_calls(data_proc_tlm_cmpnt_get_perf_soc_dimm_config_data, 1);
    expect_function_calls(data_proc_tlm_cmpnt_get_perf_soc_snsr_temp_data, NUMBER_OF_SOC_TEMP_SENSORS);
    expect_function_calls(data_proc_tlm_cmpnt_get_perf_core_amu_data, NUMBER_OF_CORES_PER_DIE);

    fpfw_status_t status = package_create_perf_report((uintptr_t)&package, sizeof(perf_report_full_package_t));

    assert_int_equal(status, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_package_create_perf_report_some_enabled, test_setup, test_teardown)
{
    typedef struct
    {
        telemetry_package_hdr_t package_header;
        perf_soc_record_rail_t rail_record;
        perf_soc_record_sensor_temp_t sensor_temp_record;
    } perf_report_partial_package_t;

    perf_report_partial_package_t package = {{{0}}};

    for(uint16_t i = 0; i < PERFORMANCE_TELEMETRY_EVENT_ID_MAX; i++)
    {
        perf_report_event_enable[i] = false;
    }

    perf_report_event_enable[PERFORMANCE_TELEMETRY_EVENT_SOC_RAILS] = true;
    perf_report_event_enable[PERFORMANCE_TELEMETRY_EVENT_SOC_TEMP_SENSOR] = true;

    expect_function_calls(data_proc_tlm_cmpnt_get_perf_soc_rail_data, MAX_NUM_OF_VR_RAILS);
    expect_function_calls(data_proc_tlm_cmpnt_get_perf_soc_snsr_temp_data, NUMBER_OF_SOC_TEMP_SENSORS);

    fpfw_status_t status = package_create_perf_report((uintptr_t)&package, sizeof(perf_report_partial_package_t));

    // verify the record headers unique fields are correct
    assert_int_equal(package.rail_record.record_header.number_of_collections, MAX_NUM_OF_VR_RAILS);
    assert_int_equal(package.rail_record.record_header.record_payload_size, (sizeof(perf_soc_record_rail_t) - sizeof(telemetry_record_hdr_t)));

    assert_int_equal(package.sensor_temp_record.record_header.number_of_collections, NUMBER_OF_SOC_TEMP_SENSORS);
    assert_int_equal(package.sensor_temp_record.record_header.record_payload_size, (sizeof(perf_soc_record_sensor_temp_t) - sizeof(telemetry_record_hdr_t)));

    assert_int_equal(status, FPFW_STATUS_SUCCESS);
}


TEST_FUNCTION(test_package_create_perf_report_too_small, test_setup, test_teardown)
{
      typedef struct
    {
        telemetry_package_hdr_t package_header;
        perf_soc_record_rail_t rail_record;
        perf_soc_record_sensor_temp_t sensor_temp_record;
    } perf_report_partial_package_t;

    perf_report_partial_package_t package = {{{0}}};

    for(uint16_t i = 0; i < PERFORMANCE_TELEMETRY_EVENT_ID_MAX; i++)
    {
        perf_report_event_enable[i] = false;
    }

    perf_report_event_enable[PERFORMANCE_TELEMETRY_EVENT_SOC_RAILS] = true;
    perf_report_event_enable[PERFORMANCE_TELEMETRY_EVENT_SOC_TEMP_SENSOR] = true;
    perf_report_event_enable[PERFORMANCE_TELEMETRY_EVENT_AMU] = true;

    expect_function_calls(data_proc_tlm_cmpnt_get_perf_soc_rail_data, MAX_NUM_OF_VR_RAILS);
    expect_function_calls(data_proc_tlm_cmpnt_get_perf_soc_snsr_temp_data, NUMBER_OF_SOC_TEMP_SENSORS);

    fpfw_status_t status = package_create_perf_report((uintptr_t)&package, sizeof(perf_report_partial_package_t));


    assert_int_equal(status, FPFW_STATUS_BUFFER_TOO_SMALL);
}