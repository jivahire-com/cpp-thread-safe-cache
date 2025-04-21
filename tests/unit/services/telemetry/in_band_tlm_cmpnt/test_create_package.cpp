//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_create_package.cpp
 * Test telemetry package creation
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
#include <package_creation_i.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t
}
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/
uint8_t cr_max_package_mem[POWER_PKG_MAX_SIZE] = {0};
telemetry_package_hdr_t pkg_header;

extern "C" {

extern bool g_die_id_mocked;
}

/*-------- Function Prototypes -----------*/
static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);

    return 0;
}

static int test_teardown(void** pContext)
{
    FPFW_UNUSED(pContext);
    g_die_id_mocked = false;
    return 0;
}

void assert_memset_to_ff(const uint8_t* array, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        assert_int_equal(array[i], 0xFF); // Ensure each element is 0xFF
    }
}

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

TEST_FUNCTION(test_record_enable_disable, test_setup, test_teardown)
{
    for (uint16_t i = 0; i < POWER_TELEMETRY_ELEMENT_ID_MAX; i++)
    {
        power_pkg_element_enable[i] = false;
    }
    for (uint16_t i = 0; i < INST_TELEMETRY_ELEMENT_ID_MAX; i++)
    {
        inst_pkg_element_enable[i] = false;
    }

    package_create_enable_disable_pwr_record(POWER_TELEMETRY_ELEMENT_SOC_PC3, true);
    assert_true(power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_PC3]);

    package_create_enable_disable_pwr_record(POWER_TELEMETRY_ELEMENT_SOC_PC3, false);
    assert_false(power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_PC3]);

    package_create_enable_disable_inst_record(INST_TELEMETRY_ELEMENT_CORE, true);
    assert_true(inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_CORE]);

    package_create_enable_disable_inst_record(INST_TELEMETRY_ELEMENT_CORE, false);
    assert_false(inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_CORE]);

    package_create_enable_disable_pwr_record(POWER_TELEMETRY_ELEMENT_ID_MAX, true);
    package_create_enable_disable_inst_record(INST_TELEMETRY_ELEMENT_ID_MAX, true);
}

TEST_FUNCTION(test_get_pwr_core_pstate_data, test_setup, test_teardown)
{
    pwr_core_record_pstate_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_pstate_data, NUMBER_OF_CORES_PER_DIE);
    uint32_t record_size = package_create_pwr_core_pstate_record(&record);

    assert_int_equal(record_size, sizeof(pwr_core_record_pstate_t));
    assert_int_not_equal(record.record_header.timestamp_uS, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size,
                     (sizeof(pwr_core_record_pstate_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(record.pstate_collection[core_id].collection_header.provider_id,
                         EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA);
        assert_int_equal(record.pstate_collection[core_id].collection_header.element_id, POWER_TELEMETRY_ELEMENT_CORE_PSTATE);
        assert_int_equal(record.pstate_collection[core_id].collection_header.collection_id, core_id);
        assert_int_equal(record.pstate_collection[core_id].collection_header.number_of_elements, NUMBER_OF_PSTATES);
        assert_int_equal(record.pstate_collection[core_id].collection_header.collection_payload_size,
                         sizeof(pwr_core_collection_pstate_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t*)record.pstate_collection[core_id].pstate_element,
                            sizeof(pwr_core_element_pstate_t) * NUMBER_OF_PSTATES);
    }
}

TEST_FUNCTION(test_get_pwr_core_cstate_data, test_setup, test_teardown)
{
    pwr_core_record_cstate_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_cstate_data, NUMBER_OF_CORES_PER_DIE);
    uint32_t record_size = package_create_pwr_core_cstate_record(&record);

    assert_int_equal(record_size, sizeof(pwr_core_record_cstate_t));
    assert_int_not_equal(record.record_header.timestamp_uS, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size,
                     (sizeof(pwr_core_record_cstate_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(record.cstate_collection[core_id].collection_header.provider_id,
                         EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA);
        assert_int_equal(record.cstate_collection[core_id].collection_header.element_id, POWER_TELEMETRY_ELEMENT_CORE_CSTATE);
        assert_int_equal(record.cstate_collection[core_id].collection_header.collection_id, core_id);
        assert_int_equal(record.cstate_collection[core_id].collection_header.number_of_elements, NUMBER_OF_CSTATES);
        assert_int_equal(record.cstate_collection[core_id].collection_header.collection_payload_size,
                         sizeof(pwr_core_collection_cstate_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t*)record.cstate_collection[core_id].cstate_element,
                            sizeof(pwr_core_element_cstate_t) * NUMBER_OF_CSTATES);
    }
}

TEST_FUNCTION(test_get_pwr_core_throttle_data, test_setup, test_teardown)
{
    pwr_core_record_throttle_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_throttle_data, NUMBER_OF_CORES_PER_DIE);
    uint32_t record_size = package_create_pwr_core_throttle_record(&record);

    assert_int_equal(record_size, sizeof(pwr_core_record_throttle_t));
    assert_int_not_equal(record.record_header.timestamp_uS, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size,
                     (sizeof(pwr_core_record_throttle_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(record.throttle_collection[core_id].collection_header.provider_id,
                         EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA);
        assert_int_equal(record.throttle_collection[core_id].collection_header.element_id,
                         POWER_TELEMETRY_ELEMENT_CORE_THROTTLE);
        assert_int_equal(record.throttle_collection[core_id].collection_header.collection_id, core_id);
        assert_int_equal(record.throttle_collection[core_id].collection_header.number_of_elements, NUMBER_OF_THROTTLE_TYPES);
        assert_int_equal(record.throttle_collection[core_id].collection_header.collection_payload_size,
                         sizeof(pwr_core_collection_throttle_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t*)record.throttle_collection[core_id].throttle_element,
                            sizeof(pwr_core_element_throttle_t) * NUMBER_OF_THROTTLE_TYPES);
    }
}

TEST_FUNCTION(test_get_pwr_core_rack_priority_data, test_setup, test_teardown)
{
    pwr_core_record_rack_priorities_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_rack_priority_data, NUMBER_OF_CORES_PER_DIE);
    uint32_t record_size = package_create_pwr_core_rack_priority_record(&record);

    assert_int_equal(record_size, sizeof(pwr_core_record_rack_priorities_t));
    assert_int_not_equal(record.record_header.timestamp_uS, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size,
                     (sizeof(pwr_core_record_rack_priorities_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(record.rack_priority_collection[core_id].collection_header.provider_id,
                         EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA);
        assert_int_equal(record.rack_priority_collection[core_id].collection_header.element_id,
                         POWER_TELEMETRY_ELEMENT_CORE_RACK_PRIORITIES);
        assert_int_equal(record.rack_priority_collection[core_id].collection_header.collection_id, core_id);
        assert_int_equal(record.rack_priority_collection[core_id].collection_header.number_of_elements,
                         NUMBER_OF_RACK_PRIORITIES);
        assert_int_equal(record.rack_priority_collection[core_id].collection_header.collection_payload_size,
                         sizeof(pwr_core_collection_rack_priorities_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t*)record.rack_priority_collection[core_id].rack_priority_element,
                            sizeof(pwr_core_element_rack_priorities_t) * NUMBER_OF_RACK_PRIORITIES);
    }
}

TEST_FUNCTION(test_get_pwr_core_voltage_data, test_setup, test_teardown)
{
    pwr_core_record_voltage_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_voltage_data, NUMBER_OF_CORES_PER_DIE);
    uint32_t record_size = package_create_pwr_core_voltage_record(&record);

    assert_int_equal(record_size, sizeof(pwr_core_record_voltage_t));
    assert_int_not_equal(record.record_header.timestamp_uS, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size,
                     (sizeof(pwr_core_record_voltage_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(record.voltage_collection[core_id].collection_header.provider_id,
                         EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA);
        assert_int_equal(record.voltage_collection[core_id].collection_header.element_id, POWER_TELEMETRY_ELEMENT_CORE_VOLTAGE);
        assert_int_equal(record.voltage_collection[core_id].collection_header.collection_id, core_id);
        assert_int_equal(record.voltage_collection[core_id].collection_header.number_of_elements, 1);
        assert_int_equal(record.voltage_collection[core_id].collection_header.collection_payload_size,
                         sizeof(pwr_core_collection_voltage_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t*)&record.voltage_collection[core_id].voltage_element,
                            sizeof(pwr_core_element_voltage_t));
    }
}

TEST_FUNCTION(test_get_pwr_core_current_data, test_setup, test_teardown)
{
    pwr_core_record_current_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_current_data, NUMBER_OF_CORES_PER_DIE);
    uint32_t record_size = package_create_pwr_core_current_record(&record);

    assert_int_equal(record_size, sizeof(pwr_core_record_current_t));
    assert_int_not_equal(record.record_header.timestamp_uS, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size,
                     (sizeof(pwr_core_record_current_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(record.current_collection[core_id].collection_header.provider_id,
                         EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA);
        assert_int_equal(record.current_collection[core_id].collection_header.element_id, POWER_TELEMETRY_ELEMENT_CORE_CURRENT);
        assert_int_equal(record.current_collection[core_id].collection_header.collection_id, core_id);
        assert_int_equal(record.current_collection[core_id].collection_header.number_of_elements, 1);
        assert_int_equal(record.current_collection[core_id].collection_header.collection_payload_size,
                         sizeof(pwr_core_collection_current_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t*)&record.current_collection[core_id].current_element,
                            sizeof(pwr_core_element_current_t));
    }
}

TEST_FUNCTION(test_get_pwr_core_temperature_data, test_setup, test_teardown)
{
    pwr_core_record_temperature_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_temperature_data, NUMBER_OF_CORES_PER_DIE);
    uint32_t record_size = package_create_pwr_core_temperature_record(&record);

    assert_int_equal(record_size, sizeof(pwr_core_record_temperature_t));
    assert_int_not_equal(record.record_header.timestamp_uS, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size,
                     (sizeof(pwr_core_record_temperature_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(record.temperature_collection[core_id].collection_header.provider_id,
                         EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA);
        assert_int_equal(record.temperature_collection[core_id].collection_header.element_id,
                         POWER_TELEMETRY_ELEMENT_CORE_TEMPERATURE);
        assert_int_equal(record.temperature_collection[core_id].collection_header.collection_id, core_id);
        assert_int_equal(record.temperature_collection[core_id].collection_header.number_of_elements, 1);
        assert_int_equal(record.temperature_collection[core_id].collection_header.collection_payload_size,
                         sizeof(pwr_core_collection_temperature_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t*)&record.temperature_collection[core_id].temperature_element,
                            sizeof(pwr_core_element_temperature_t));
    }
}

TEST_FUNCTION(test_get_pwr_core_histogram_data, test_setup, test_teardown)
{
    pwr_core_record_histogram_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_histogram_data, NUMBER_OF_CORES_PER_DIE);
    uint32_t record_size = package_create_pwr_core_histogram_record(&record);

    assert_int_equal(record_size, sizeof(pwr_core_record_histogram_t));
    assert_int_not_equal(record.record_header.timestamp_uS, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size,
                     (sizeof(pwr_core_record_histogram_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(record.histogram_collection[core_id].collection_header.provider_id,
                         EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA);
        assert_int_equal(record.histogram_collection[core_id].collection_header.element_id,
                         POWER_TELEMETRY_ELEMENT_CORE_HISTOGRAM);
        assert_int_equal(record.histogram_collection[core_id].collection_header.collection_id, core_id);
        assert_int_equal(record.histogram_collection[core_id].collection_header.number_of_elements,
                         NUMBER_OF_HS_VOLTAGE_SCALES * NUMBER_OF_HS_TEMP_SCALES);
        assert_int_equal(record.histogram_collection[core_id].collection_header.collection_payload_size,
                         sizeof(pwr_core_collection_histogram_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t*)record.histogram_collection[core_id].histogram_element,
                            sizeof(pwr_core_element_histogram_t) * NUMBER_OF_HS_VOLTAGE_SCALES * NUMBER_OF_HS_TEMP_SCALES);
    }
}

TEST_FUNCTION(test_get_pwr_soc_pc3_data, test_setup, test_teardown)
{
    pwr_soc_record_pc3_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_soc_pc3_data, 1);
    uint32_t record_size = package_create_pwr_soc_pc3_record(&record);

    assert_int_equal(record_size, sizeof(pwr_soc_record_pc3_t));
    assert_int_not_equal(record.record_header.timestamp_uS, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, 1);
    assert_int_equal(record.record_header.record_payload_size,
                     (sizeof(pwr_soc_record_pc3_t) - sizeof(telemetry_record_hdr_t)));

    assert_int_equal(record.pc3_collection.collection_header.provider_id, EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA);
    assert_int_equal(record.pc3_collection.collection_header.element_id, POWER_TELEMETRY_ELEMENT_SOC_PC3);
    assert_int_equal(record.pc3_collection.collection_header.collection_id, 1);
    assert_int_equal(record.pc3_collection.collection_header.number_of_elements, 1);
    assert_int_equal(record.pc3_collection.collection_header.collection_payload_size,
                     sizeof(pwr_soc_collection_pc3_t) - sizeof(telemetry_collection_hdr_t));

    // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
    // This verifies that the correct data ranges are passed to the data processing component get data api's
    assert_memset_to_ff((uint8_t*)&record.pc3_collection.pc3_element, sizeof(pwr_soc_element_pc3_t));
}

TEST_FUNCTION(test_get_pwr_soc_vr_rail_data, test_setup, test_teardown)
{
    pwr_soc_record_vr_rail_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_soc_vr_rail_data, MAX_NUM_OF_VR_RAILS);
    uint32_t record_size = package_create_pwr_soc_vr_rail_record(&record);

    assert_int_equal(record_size, sizeof(pwr_soc_record_vr_rail_t));
    assert_int_not_equal(record.record_header.timestamp_uS, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, MAX_NUM_OF_VR_RAILS);
    assert_int_equal(record.record_header.record_payload_size,
                     (sizeof(pwr_soc_record_vr_rail_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t rail_id = 0; rail_id < MAX_NUM_OF_VR_RAILS; rail_id++)
    {
        assert_int_equal(record.rail_collection[rail_id].collection_header.provider_id,
                         EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA);
        assert_int_equal(record.rail_collection[rail_id].collection_header.element_id, POWER_TELEMETRY_ELEMENT_SOC_VR_RAILS);
        assert_int_equal(record.rail_collection[rail_id].collection_header.collection_id, rail_id);
        assert_int_equal(record.rail_collection[rail_id].collection_header.number_of_elements, 1);
        assert_int_equal(record.rail_collection[rail_id].collection_header.collection_payload_size,
                         sizeof(pwr_soc_collection_vr_rail_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t*)&record.rail_collection[rail_id].rail_element, sizeof(pwr_soc_element_vr_rail_t));
    }
}

TEST_FUNCTION(test_get_pwr_soc_hnf_data, test_setup, test_teardown)
{
    pwr_soc_record_hnf_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_soc_hnf_data, NUMBER_OF_HNF_CHANNELS_PER_DIE);
    uint32_t record_size = package_create_pwr_soc_hnf_record(&record);

    assert_int_equal(record_size, sizeof(pwr_soc_record_hnf_t));
    assert_int_not_equal(record.record_header.timestamp_uS, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_HNF_CHANNELS_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size,
                     (sizeof(pwr_soc_record_hnf_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t hnf_chan = 0; hnf_chan < NUMBER_OF_HNF_CHANNELS_PER_DIE; hnf_chan++)
    {
        assert_int_equal(record.hnf_collection[hnf_chan].collection_header.provider_id,
                         EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA);
        assert_int_equal(record.hnf_collection[hnf_chan].collection_header.element_id, POWER_TELEMETRY_ELEMENT_SOC_HNF);
        assert_int_equal(record.hnf_collection[hnf_chan].collection_header.collection_id, hnf_chan);
        assert_int_equal(record.hnf_collection[hnf_chan].collection_header.number_of_elements, 1);
        assert_int_equal(record.hnf_collection[hnf_chan].collection_header.collection_payload_size,
                         sizeof(pwr_soc_collection_hnf_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t*)&record.hnf_collection[hnf_chan].hnf_element, sizeof(pwr_soc_element_hnf_t));
    }
}

TEST_FUNCTION(test_get_pwr_soc_dimm_data, test_setup, test_teardown)
{
    pwr_soc_record_dimm_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_soc_dimm_data, NUMBER_OF_DIMM_MODULES);
    uint32_t record_size = package_create_pwr_soc_dimm_record(&record);

    assert_int_equal(record_size, sizeof(pwr_soc_record_dimm_t));
    assert_int_not_equal(record.record_header.timestamp_uS, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_DIMM_MODULES);
    assert_int_equal(record.record_header.record_payload_size,
                     (sizeof(pwr_soc_record_dimm_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t dimm_module = 0; dimm_module < NUMBER_OF_DIMM_MODULES; dimm_module++)
    {
        assert_int_equal(record.dimm_collection[dimm_module].collection_header.provider_id,
                         EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA);
        assert_int_equal(record.dimm_collection[dimm_module].collection_header.element_id, POWER_TELEMETRY_ELEMENT_SOC_DIMM);
        assert_int_equal(record.dimm_collection[dimm_module].collection_header.collection_id, dimm_module);
        assert_int_equal(record.dimm_collection[dimm_module].collection_header.number_of_elements, 1);
        assert_int_equal(record.dimm_collection[dimm_module].collection_header.collection_payload_size,
                         sizeof(pwr_soc_collection_dimm_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t*)&record.dimm_collection[dimm_module].dimm_element, sizeof(pwr_soc_element_dimm_t));
    }
}

TEST_FUNCTION(test_get_pwr_soc_sensor_temp_data, test_setup, test_teardown)
{
    pwr_soc_record_sensor_temp_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_soc_snsr_temp_data, NUMBER_OF_SOC_TEMP_SENSORS);
    uint32_t record_size = package_create_pwr_soc_sensor_temp_record(&record);

    assert_int_equal(record_size, sizeof(pwr_soc_record_sensor_temp_t));
    assert_int_not_equal(record.record_header.timestamp_uS, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_SOC_TEMP_SENSORS);
    assert_int_equal(record.record_header.record_payload_size,
                     (sizeof(pwr_soc_record_sensor_temp_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t sensor_id = 0; sensor_id < NUMBER_OF_SOC_TEMP_SENSORS; sensor_id++)
    {
        assert_int_equal(record.sensor_temp_collection[sensor_id].collection_header.provider_id,
                         EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA);
        assert_int_equal(record.sensor_temp_collection[sensor_id].collection_header.element_id,
                         POWER_TELEMETRY_ELEMENT_SOC_SENSOR_TEMP);
        assert_int_equal(record.sensor_temp_collection[sensor_id].collection_header.collection_id, sensor_id);
        assert_int_equal(record.sensor_temp_collection[sensor_id].collection_header.number_of_elements, 1);
        assert_int_equal(record.sensor_temp_collection[sensor_id].collection_header.collection_payload_size,
                         sizeof(pwr_soc_collection_sensor_temp_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t*)&record.sensor_temp_collection[sensor_id].sensor_temp_element,
                            sizeof(pwr_soc_element_sensor_temp_t));
    }
}

TEST_FUNCTION(test_get_pwr_mpam_pstate_data, test_setup, test_teardown)
{
    pwr_record_mpam_pstate_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_mpam_pstate_data, NUMBER_OF_MPAMS);
    uint32_t record_size = package_create_pwr_mpam_pstate_record(&record);

    assert_int_equal(record_size, sizeof(pwr_record_mpam_pstate_t));
    assert_int_not_equal(record.record_header.timestamp_uS, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_MPAMS);
    assert_int_equal(record.record_header.record_payload_size,
                     (sizeof(pwr_record_mpam_pstate_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t mpam_id = 0; mpam_id < NUMBER_OF_MPAMS; mpam_id++)
    {
        assert_int_equal(record.mpam_pstate_collection[mpam_id].collection_header.provider_id,
                         EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA);
        assert_int_equal(record.mpam_pstate_collection[mpam_id].collection_header.element_id, POWER_TELEMETRY_ELEMENT_MPAM);
        assert_int_equal(record.mpam_pstate_collection[mpam_id].collection_header.collection_id, mpam_id);
        assert_int_equal(record.mpam_pstate_collection[mpam_id].collection_header.number_of_elements, NUMBER_OF_PSTATES);
        assert_int_equal(record.mpam_pstate_collection[mpam_id].collection_header.collection_payload_size,
                         sizeof(pwr_collection_mpam_pstate_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t*)&record.mpam_pstate_collection[mpam_id].mpam_pstate_element,
                            sizeof(pwr_element_mpam_pstate_t) * NUMBER_OF_PSTATES);
    }
}

TEST_FUNCTION(test_get_pwr_mpam_throttle_data, test_setup, test_teardown)
{
    pwr_record_mpam_throttle_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_soc_mpam_throttle_data, NUMBER_OF_MPAMS);
    uint32_t record_size = package_create_pwr_mpam_throttle_record(&record);

    assert_int_equal(record_size, sizeof(pwr_record_mpam_throttle_t));
    assert_int_not_equal(record.record_header.timestamp_uS, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_MPAMS);
    assert_int_equal(record.record_header.record_payload_size,
                     (sizeof(pwr_record_mpam_throttle_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t mpam_id = 0; mpam_id < NUMBER_OF_MPAMS; mpam_id++)
    {
        assert_int_equal(record.mpam_throttle_collection[mpam_id].collection_header.provider_id,
                         EVENT_TRACE_PROVIDER_ID_MCP_POWER_TLM_SCHEMA);
        assert_int_equal(record.mpam_throttle_collection[mpam_id].collection_header.element_id,
                         POWER_TELEMETRY_ELEMENT_MPAM_THROTTLE);
        assert_int_equal(record.mpam_throttle_collection[mpam_id].collection_header.collection_id, mpam_id);
        assert_int_equal(record.mpam_throttle_collection[mpam_id].collection_header.number_of_elements, 1);
        assert_int_equal(record.mpam_throttle_collection[mpam_id].collection_header.collection_payload_size,
                         sizeof(pwr_collection_mpam_throttle_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t*)&record.mpam_throttle_collection[mpam_id].mpam_throttle_element,
                            sizeof(pwr_element_mpam_throttle_t));
    }
}

TEST_FUNCTION(test_get_inst_core_summary_data, test_setup, test_teardown)
{
    inst_core_record_summary_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_inst_soc_core_summary_data, NUMBER_OF_CORES_PER_DIE);
    uint32_t record_size = package_create_inst_core_summary_record(&record);

    assert_int_equal(record_size, sizeof(inst_core_record_summary_t));
    assert_int_not_equal(record.record_header.timestamp_uS, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size,
                     (sizeof(inst_core_record_summary_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(record.inst_core_summary_collection[core_id].collection_header.provider_id,
                         EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA);
        assert_int_equal(record.inst_core_summary_collection[core_id].collection_header.element_id, INST_TELEMETRY_ELEMENT_CORE);
        assert_int_equal(record.inst_core_summary_collection[core_id].collection_header.collection_id, core_id);
        assert_int_equal(record.inst_core_summary_collection[core_id].collection_header.number_of_elements, 1);
        assert_int_equal(record.inst_core_summary_collection[core_id].collection_header.collection_payload_size,
                         sizeof(inst_core_collection_summary_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t*)&record.inst_core_summary_collection[core_id].summary_element,
                            sizeof(inst_core_element_summary_t));
    }
}

TEST_FUNCTION(test_get_inst_soc_rail_data, test_setup, test_teardown)
{
    inst_soc_record_rail_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_inst_soc_rail_data, MAX_NUM_OF_VR_RAILS);
    uint32_t record_size = package_create_inst_soc_rail_record(&record);

    assert_int_equal(record_size, sizeof(inst_soc_record_rail_t));
    assert_int_not_equal(record.record_header.timestamp_uS, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, MAX_NUM_OF_VR_RAILS);
    assert_int_equal(record.record_header.record_payload_size,
                     (sizeof(inst_soc_record_rail_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t rail_id = 0; rail_id < MAX_NUM_OF_VR_RAILS; rail_id++)
    {
        assert_int_equal(record.rail_collection[rail_id].collection_header.provider_id,
                         EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA);
        assert_int_equal(record.rail_collection[rail_id].collection_header.element_id, INST_TELEMETRY_ELEMENT_SOC_RAILS);
        assert_int_equal(record.rail_collection[rail_id].collection_header.collection_id, rail_id);
        assert_int_equal(record.rail_collection[rail_id].collection_header.number_of_elements, 1);
        assert_int_equal(record.rail_collection[rail_id].collection_header.collection_payload_size,
                         sizeof(inst_soc_collection_rail_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t*)&record.rail_collection[rail_id].rail_element, sizeof(inst_soc_element_rail_t));
    }
}

TEST_FUNCTION(test_get_inst_soc_dimm_runtime_data, test_setup, test_teardown)
{
    inst_soc_record_dimm_runtime_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_inst_soc_dimm_runtime_data, NUMBER_OF_DIMM_MODULES);
    uint32_t record_size = package_create_inst_soc_dimm_runtime_record(&record);

    assert_int_equal(record_size, sizeof(inst_soc_record_dimm_runtime_t));
    assert_int_not_equal(record.record_header.timestamp_uS, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_DIMM_MODULES);
    assert_int_equal(record.record_header.record_payload_size,
                     (sizeof(inst_soc_record_dimm_runtime_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t module_id = 0; module_id < NUMBER_OF_DIMM_MODULES; module_id++)
    {
        assert_int_equal(record.dimm_collection[module_id].collection_header.provider_id,
                         EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA);
        assert_int_equal(record.dimm_collection[module_id].collection_header.element_id, INST_TELEMETRY_ELEMENT_SOC_DIMM_RT);
        assert_int_equal(record.dimm_collection[module_id].collection_header.collection_id, module_id);
        assert_int_equal(record.dimm_collection[module_id].collection_header.number_of_elements, 1);
        assert_int_equal(record.dimm_collection[module_id].collection_header.collection_payload_size,
                         sizeof(inst_soc_collection_dimm_runtime_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t*)&record.dimm_collection[module_id].dimm_rt_element,
                            sizeof(inst_soc_element_dimm_runtime_t));
    }
}

TEST_FUNCTION(test_get_inst_soc_dimm_config_data, test_setup, test_teardown)
{
    inst_soc_record_dimm_config_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_inst_soc_dimm_config_data, 1);
    uint32_t record_size = package_create_inst_soc_dimm_config_record(&record);

    assert_int_equal(record_size, sizeof(inst_soc_record_dimm_config_t));
    assert_int_not_equal(record.record_header.timestamp_uS, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, 1);
    assert_int_equal(record.record_header.record_payload_size,
                     (sizeof(inst_soc_record_dimm_config_t) - sizeof(telemetry_record_hdr_t)));

    assert_int_equal(record.dimm_config_collection.collection_header.provider_id, EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA);
    assert_int_equal(record.dimm_config_collection.collection_header.element_id, INST_TELEMETRY_ELEMENT_SOC_DIMM_CONFIG);
    assert_int_equal(record.dimm_config_collection.collection_header.collection_id, 1);
    assert_int_equal(record.dimm_config_collection.collection_header.number_of_elements, 1);
    assert_int_equal(record.dimm_config_collection.collection_header.collection_payload_size,
                     sizeof(inst_soc_collection_dimm_config_t) - sizeof(telemetry_collection_hdr_t));

    // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
    // This verifies that the correct data ranges are passed to the data processing component get data api's
    assert_memset_to_ff((uint8_t*)&record.dimm_config_collection.dimm_config_element,
                        sizeof(inst_soc_element_dimm_config_t));
}

TEST_FUNCTION(test_get_inst_soc_sensor_temp_data, test_setup, test_teardown)
{
    inst_soc_record_sensor_temp_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_inst_soc_snsr_temp_data, NUMBER_OF_SOC_TEMP_SENSORS);
    uint32_t record_size = package_create_inst_soc_sensor_temp_record(&record);

    assert_int_equal(record_size, sizeof(inst_soc_record_sensor_temp_t));
    assert_int_not_equal(record.record_header.timestamp_uS, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_SOC_TEMP_SENSORS);
    assert_int_equal(record.record_header.record_payload_size,
                     (sizeof(inst_soc_record_sensor_temp_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t sensor_id = 0; sensor_id < NUMBER_OF_SOC_TEMP_SENSORS; sensor_id++)
    {
        assert_int_equal(record.temperature_collection[sensor_id].collection_header.provider_id,
                         EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA);
        assert_int_equal(record.temperature_collection[sensor_id].collection_header.element_id,
                         INST_TELEMETRY_ELEMENT_SOC_TEMP_SENSOR);
        assert_int_equal(record.temperature_collection[sensor_id].collection_header.collection_id, sensor_id);
        assert_int_equal(record.temperature_collection[sensor_id].collection_header.number_of_elements, 1);
        assert_int_equal(record.temperature_collection[sensor_id].collection_header.collection_payload_size,
                         sizeof(inst_soc_collection_sensor_temp_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t*)&record.temperature_collection[sensor_id].temperature_element,
                            sizeof(inst_soc_element_sensor_temp_t));
    }
}

TEST_FUNCTION(test_get_inst_core_amu_counters_data, test_setup, test_teardown)
{
    inst_core_record_amu_counters_t record = {{0}};

    expect_function_calls(data_proc_tlm_cmpnt_get_inst_core_amu_data, NUMBER_OF_CORES_PER_DIE);
    uint32_t record_size = package_create_inst_core_amu_counters_record(&record);

    assert_int_equal(record_size, sizeof(inst_core_record_amu_counters_t));
    assert_int_not_equal(record.record_header.timestamp_uS, 0);
    assert_int_not_equal(record.record_header.record_number, 0);
    assert_int_equal(record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(record.record_header.record_payload_size,
                     (sizeof(inst_core_record_amu_counters_t) - sizeof(telemetry_record_hdr_t)));

    for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        assert_int_equal(record.amu_counter_collection[core_id].collection_header.provider_id,
                         EVENT_TRACE_PROVIDER_ID_MCP_INST_TLM_SCHEMA);
        assert_int_equal(record.amu_counter_collection[core_id].collection_header.element_id, INST_TELEMETRY_ELEMENT_AMU);
        assert_int_equal(record.amu_counter_collection[core_id].collection_header.collection_id, core_id);
        assert_int_equal(record.amu_counter_collection[core_id].collection_header.number_of_elements, 1);
        assert_int_equal(record.amu_counter_collection[core_id].collection_header.collection_payload_size,
                         sizeof(inst_core_collection_amu_counters_t) - sizeof(telemetry_collection_hdr_t));

        // event data ranges are initialized to 0, the mock Get Api sets them to 0xFF
        // This verifies that the correct data ranges are passed to the data processing component get data api's
        assert_memset_to_ff((uint8_t*)&record.amu_counter_collection[core_id].amu_counter_element,
                            sizeof(inst_core_element_amu_counters_t));
    }
}

TEST_FUNCTION(test_package_create_power_pkg_none_enabled, test_setup, test_teardown)
{
    for (uint16_t i = 0; i < POWER_TELEMETRY_ELEMENT_ID_MAX; i++)
    {
        power_pkg_element_enable[i] = false;
    }

    uint32_t pkg_size = package_create_power_pkg((uintptr_t)cr_max_package_mem, POWER_PKG_MAX_SIZE);

    assert_int_equal(pkg_size, 0);
}

TEST_FUNCTION(test_package_create_power_pkg_all_enabled, test_setup, test_teardown)
{
    for (uint16_t i = 0; i < POWER_TELEMETRY_ELEMENT_ID_MAX; i++)
    {
        power_pkg_element_enable[i] = true;
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

    uint32_t pkg_size = package_create_power_pkg((uintptr_t)cr_max_package_mem, POWER_PKG_MAX_SIZE);

    assert_int_equal(pkg_size, POWER_PKG_MAX_SIZE);
}

TEST_FUNCTION(test_package_create_power_pkg_some_enabled, test_setup, test_teardown)
{
    typedef struct
    {
        telemetry_package_hdr_t package_header;
        pwr_core_record_pstate_t pstate_record;
        pwr_soc_record_pc3_t pc3_record;
    } power_report_partial_package_t;

    power_report_partial_package_t* package = (power_report_partial_package_t*)cr_max_package_mem;

    for (uint16_t i = 0; i < POWER_TELEMETRY_ELEMENT_ID_MAX; i++)
    {
        power_pkg_element_enable[i] = false;
    }

    power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_CORE_PSTATE] = true;
    power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_PC3] = true;

    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_pstate_data, NUMBER_OF_CORES_PER_DIE);
    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_soc_pc3_data, 1);

    uint32_t pkg_size = package_create_power_pkg((uintptr_t)cr_max_package_mem, POWER_PKG_MAX_SIZE);
    assert_int_equal(pkg_size, sizeof(pwr_core_record_pstate_t) + sizeof(pwr_soc_record_pc3_t) + sizeof(telemetry_package_hdr_t));

    // verify the record headers unique fields are correct
    assert_int_equal(package->pstate_record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);
    assert_int_equal(package->pstate_record.record_header.record_payload_size,
                     (sizeof(pwr_core_record_pstate_t) - sizeof(telemetry_record_hdr_t)));

    assert_int_equal(package->pc3_record.record_header.number_of_collections, 1);
    assert_int_equal(package->pc3_record.record_header.record_payload_size,
                     (sizeof(pwr_soc_record_pc3_t) - sizeof(telemetry_record_hdr_t)));
}

TEST_FUNCTION(test_package_create_power_pkg_too_small, test_setup, test_teardown)
{
    typedef struct
    {
        telemetry_package_hdr_t package_header;
        pwr_core_record_pstate_t pstate_record;
        pwr_soc_record_pc3_t pc3_record;
    } power_report_partial_package_t;

    for (uint16_t i = 0; i < POWER_TELEMETRY_ELEMENT_ID_MAX; i++)
    {
        power_pkg_element_enable[i] = false;
    }

    power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_CORE_PSTATE] = true;
    power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_PC3] = true;
    power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_SOC_VR_RAILS] = true;

    uint32_t pkg_size = package_create_power_pkg((uintptr_t)cr_max_package_mem, sizeof(power_report_partial_package_t));
    assert_int_equal(pkg_size, 0);
}

TEST_FUNCTION(test_package_create_append_to_inst_pkg_none_enabled, test_setup, test_teardown)
{
    for (uint16_t i = 0; i < INST_TELEMETRY_ELEMENT_ID_MAX; i++)
    {
        inst_pkg_element_enable[i] = false;
    }

    uint32_t pkg_size = package_create_append_to_inst_pkg((uintptr_t)cr_max_package_mem, INST_PKG_MAX_SIZE, &pkg_header);

    assert_int_equal(pkg_size, 0);
}

TEST_FUNCTION(test_package_create_append_to_inst_pkg_all_enabled, test_setup, test_teardown)
{
    for (uint16_t i = 0; i < INST_TELEMETRY_ELEMENT_ID_MAX; i++)
    {
        inst_pkg_element_enable[i] = true;
    }
    expect_function_calls(data_proc_tlm_cmpnt_get_inst_soc_core_summary_data, NUMBER_OF_CORES_PER_DIE);
    expect_function_calls(data_proc_tlm_cmpnt_get_inst_soc_rail_data, MAX_NUM_OF_VR_RAILS);
    expect_function_calls(data_proc_tlm_cmpnt_get_inst_soc_dimm_runtime_data, NUMBER_OF_DIMM_MODULES);
    expect_function_calls(data_proc_tlm_cmpnt_get_inst_soc_dimm_config_data, 1);
    expect_function_calls(data_proc_tlm_cmpnt_get_inst_soc_snsr_temp_data, NUMBER_OF_SOC_TEMP_SENSORS);
    expect_function_calls(data_proc_tlm_cmpnt_get_inst_core_amu_data, NUMBER_OF_CORES_PER_DIE);

    uint32_t pkg_size = package_create_append_to_inst_pkg((uintptr_t)cr_max_package_mem, INST_PKG_MAX_SIZE, &pkg_header);

    assert_int_equal(pkg_size, sizeof(inst_full_package_t));
}

TEST_FUNCTION(test_package_create_append_to_inst_pkg_some_enabled, test_setup, test_teardown)
{
    typedef struct
    {
        inst_soc_record_rail_t rail_record;
        inst_soc_record_sensor_temp_t sensor_temp_record;
    } inst_report_partial_package_t;

    inst_report_partial_package_t* package = (inst_report_partial_package_t*)cr_max_package_mem;

    for (uint16_t i = 0; i < INST_TELEMETRY_ELEMENT_ID_MAX; i++)
    {
        inst_pkg_element_enable[i] = false;
    }

    inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_SOC_RAILS] = true;
    inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_SOC_TEMP_SENSOR] = true;

    expect_function_calls(data_proc_tlm_cmpnt_get_inst_soc_rail_data, MAX_NUM_OF_VR_RAILS);
    expect_function_calls(data_proc_tlm_cmpnt_get_inst_soc_snsr_temp_data, NUMBER_OF_SOC_TEMP_SENSORS);

    uint32_t pkg_size = package_create_append_to_inst_pkg((uintptr_t)&cr_max_package_mem, INST_PKG_MAX_SIZE, &pkg_header);
    assert_int_equal(pkg_size, sizeof(inst_soc_record_rail_t) + sizeof(inst_soc_record_sensor_temp_t));
    // verify the record headers unique fields are correct
    assert_int_equal(package->rail_record.record_header.number_of_collections, MAX_NUM_OF_VR_RAILS);
    assert_int_equal(package->rail_record.record_header.record_payload_size,
                     (sizeof(inst_soc_record_rail_t) - sizeof(telemetry_record_hdr_t)));

    assert_int_equal(package->sensor_temp_record.record_header.number_of_collections, NUMBER_OF_SOC_TEMP_SENSORS);
    assert_int_equal(package->sensor_temp_record.record_header.record_payload_size,
                     (sizeof(inst_soc_record_sensor_temp_t) - sizeof(telemetry_record_hdr_t)));
}

TEST_FUNCTION(test_package_create_append_to_inst_pkg_too_small, test_setup, test_teardown)
{
    typedef struct
    {
        inst_soc_record_rail_t rail_record;
        inst_soc_record_sensor_temp_t sensor_temp_record;
    } inst_report_partial_package_t;

    for (uint16_t i = 0; i < INST_TELEMETRY_ELEMENT_ID_MAX; i++)
    {
        inst_pkg_element_enable[i] = false;
    }

    inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_SOC_RAILS] = true;
    inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_SOC_TEMP_SENSOR] = true;
    inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_AMU] = true;

    uint32_t pkg_size =
        package_create_append_to_inst_pkg((uintptr_t)&cr_max_package_mem, sizeof(inst_report_partial_package_t), &pkg_header);
    assert_int_equal(pkg_size, 0);
}

TEST_FUNCTION(test_in_band_tlm_cmpnt_is_instantaneous_enabled, test_setup, test_teardown)
{
    for (uint32_t i = 0; i < INST_TELEMETRY_ELEMENT_ID_MAX; i++)
    {
        inst_pkg_element_enable[i] = false;
    }
    bool is_enabled = in_band_tlm_cmpnt_is_instantaneous_enabled();
    assert_int_equal(is_enabled, false);

    inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_SOC_RAILS] = true;
    is_enabled = in_band_tlm_cmpnt_is_instantaneous_enabled();
    assert_int_equal(is_enabled, true);
}

TEST_FUNCTION(test_in_band_tlm_cmpnt_core_id_die_offset, test_setup, test_teardown)
{

    // The core id is offset for any package containing a collection of records per core.
    // To validate this we don't need to actually look at any of the data in the record,
    // just the collection id matching the expected core id for that die for that event.

    g_die_id_mocked = true;

    // Test the p state collections
    for (uint8_t die_id = 0; die_id < 2; die_id++)
    {
        // Setup die id
        will_return(__wrap_mts_get_this_die_id, die_id);
        package_creation_init();

        // Fill in the record
        expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_pstate_data, NUMBER_OF_CORES_PER_DIE);
        pwr_core_record_pstate_t pstate_record;
        FPFW_UNUSED(package_create_pwr_core_pstate_record(&pstate_record));

        for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
        {
            assert_int_equal(pstate_record.pstate_collection[core_id].collection_header.collection_id,
                             CORE_ID_WITH_DIE_OFFSET(core_id));
        }
    }

    // Test the c state collections
    for (uint8_t die_id = 0; die_id < 2; die_id++)
    {
        // Setup die id
        will_return(__wrap_mts_get_this_die_id, die_id);
        package_creation_init();

        // Fill in the record
        expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_cstate_data, NUMBER_OF_CORES_PER_DIE);
        pwr_core_record_cstate_t cstate_record;
        FPFW_UNUSED(package_create_pwr_core_cstate_record(&cstate_record));

        for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
        {
            assert_int_equal(cstate_record.cstate_collection[core_id].collection_header.collection_id,
                             CORE_ID_WITH_DIE_OFFSET(core_id));
        }
    }

    // Test the throttle collections
    for (uint8_t die_id = 0; die_id < 2; die_id++)
    {
        // Setup die id
        will_return(__wrap_mts_get_this_die_id, die_id);
        package_creation_init();

        // Fill in the record
        expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_throttle_data, NUMBER_OF_CORES_PER_DIE);
        pwr_core_record_throttle_t throttle_record;
        FPFW_UNUSED(package_create_pwr_core_throttle_record(&throttle_record));

        for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
        {
            assert_int_equal(throttle_record.throttle_collection[core_id].collection_header.collection_id,
                             CORE_ID_WITH_DIE_OFFSET(core_id));
        }
    }

    // Test the rack priority collections
    for (uint8_t die_id = 0; die_id < 2; die_id++)
    {
        // Setup die id
        will_return(__wrap_mts_get_this_die_id, die_id);
        package_creation_init();

        // Fill in the record
        expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_rack_priority_data, NUMBER_OF_CORES_PER_DIE);
        pwr_core_record_rack_priorities_t rack_priority_record;
        FPFW_UNUSED(package_create_pwr_core_rack_priority_record(&rack_priority_record));

        for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
        {
            assert_int_equal(rack_priority_record.rack_priority_collection[core_id].collection_header.collection_id,
                             CORE_ID_WITH_DIE_OFFSET(core_id));
        }
    }

    // Test the voltage collections
    for (uint8_t die_id = 0; die_id < 2; die_id++)
    {
        // Setup die id
        will_return(__wrap_mts_get_this_die_id, die_id);
        package_creation_init();

        // Fill in the record
        expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_voltage_data, NUMBER_OF_CORES_PER_DIE);
        pwr_core_record_voltage_t voltage_record;
        FPFW_UNUSED(package_create_pwr_core_voltage_record(&voltage_record));

        for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
        {
            assert_int_equal(voltage_record.voltage_collection[core_id].collection_header.collection_id,
                             CORE_ID_WITH_DIE_OFFSET(core_id));
        }
    }

    // Test the current collections
    for (uint8_t die_id = 0; die_id < 2; die_id++)
    {
        // Setup die id
        will_return(__wrap_mts_get_this_die_id, die_id);
        package_creation_init();

        // Fill in the record
        expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_current_data, NUMBER_OF_CORES_PER_DIE);
        pwr_core_record_current_t current_record;
        FPFW_UNUSED(package_create_pwr_core_current_record(&current_record));

        for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
        {
            assert_int_equal(current_record.current_collection[core_id].collection_header.collection_id,
                             CORE_ID_WITH_DIE_OFFSET(core_id));
        }
    }

    // Test the temperature collections
    for (uint8_t die_id = 0; die_id < 2; die_id++)
    {
        // Setup die id
        will_return(__wrap_mts_get_this_die_id, die_id);
        package_creation_init();

        // Fill in the record
        expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_temperature_data, NUMBER_OF_CORES_PER_DIE);
        pwr_core_record_temperature_t temperature_record;
        FPFW_UNUSED(package_create_pwr_core_temperature_record(&temperature_record));

        for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
        {
            assert_int_equal(temperature_record.temperature_collection[core_id].collection_header.collection_id,
                             CORE_ID_WITH_DIE_OFFSET(core_id));
        }
    }

    // Test the histogram collections
    for (uint8_t die_id = 0; die_id < 2; die_id++)
    {
        // Setup die id
        will_return(__wrap_mts_get_this_die_id, die_id);
        package_creation_init();

        // Fill in the record
        expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_histogram_data, NUMBER_OF_CORES_PER_DIE);
        pwr_core_record_histogram_t histogram_record;
        FPFW_UNUSED(package_create_pwr_core_histogram_record(&histogram_record));

        for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
        {
            assert_int_equal(histogram_record.histogram_collection[core_id].collection_header.collection_id,
                             CORE_ID_WITH_DIE_OFFSET(core_id));
        }
    }

    // Test the instantaneous core collections
    for (uint8_t die_id = 0; die_id < 2; die_id++)
    {
        // Setup die id
        will_return(__wrap_mts_get_this_die_id, die_id);
        package_creation_init();

        // Fill in the record
        expect_function_calls(data_proc_tlm_cmpnt_get_inst_soc_core_summary_data, NUMBER_OF_CORES_PER_DIE);
        inst_core_record_summary_t summary_record;
        FPFW_UNUSED(package_create_inst_core_summary_record(&summary_record));

        for (uint16_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
        {
            assert_int_equal(summary_record.inst_core_summary_collection[core_id].collection_header.collection_id,
                             CORE_ID_WITH_DIE_OFFSET(core_id));
        }
    }
}