//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_pack_get_apis.cpp
 * This tests the api's used to retrieve data for in band and out of band telemetry reporting.
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <data_proc_tlm_cmpnt.h>
#include <sensor_fifo_service.h> // for QUADWORD_SIZE, sensor_ram_...
#include <stdint.h> // for uint32_t, uint64_t, int32_t
#include <tlm_logger_i.h>
}

/*-- Symbolic Constant Macros (defines) --*/
#define TEST_SNSR_ID_0 (0)
#define TEST_HNF_CHANN_ID_1 (1)
#define TEST_RAIL_ID_2 (2)
#define TEST_DIMM_CHANN_ID_3 (3)
#define TEST_MPAM_ID_4 (4)
#define TEST_CORE_ID_5 (5)

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

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

TEST_FUNCTION(test_get_pwr_core_pstate_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    pwr_core_event_pstate_t pstate_array[NUMBER_OF_PSTATES] = {{0}};
    data_proc_tlm_cmpnt_get_pwr_core_pstate_data(TEST_CORE_ID_5, &pstate_array);
}

TEST_FUNCTION(test_get_pwr_core_cstate_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    pwr_core_event_cstate_t cstate_array[NUMBER_OF_CSTATES] = {{0}};
    data_proc_tlm_cmpnt_get_pwr_core_cstate_data(TEST_CORE_ID_5, &cstate_array);
}

TEST_FUNCTION(test_get_pwr_core_throttle_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    pwr_core_event_throttle_t throttle_array[NUMBER_OF_THROTTLE_TYPES] = {{0}};
    data_proc_tlm_cmpnt_get_pwr_core_throttle_data(TEST_CORE_ID_5, &throttle_array);
}

TEST_FUNCTION(test_get_pwr_core_rack_priority_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    pwr_core_event_rack_priorities_t rack_priority_array[NUMBER_OF_RACK_PRIORITIES] = {{0}};
    data_proc_tlm_cmpnt_get_pwr_core_rack_priority_data(TEST_CORE_ID_5, &rack_priority_array);
}

TEST_FUNCTION(test_get_pwr_core_voltage_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    pwr_core_event_voltage_t voltage_data = {0};
    data_proc_tlm_cmpnt_get_pwr_core_voltage_data(TEST_CORE_ID_5, &voltage_data);
}

TEST_FUNCTION(test_get_pwr_core_current_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    pwr_core_event_current_t current_data = {0};
    data_proc_tlm_cmpnt_get_pwr_core_current_data(TEST_CORE_ID_5, &current_data);
}

TEST_FUNCTION(test_get_pwr_core_temperature_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    pwr_core_event_temperature_t temp_data = {0};
    data_proc_tlm_cmpnt_get_pwr_core_temperature_data(TEST_CORE_ID_5, &temp_data);
}

TEST_FUNCTION(test_get_pwr_core_histogram_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    pwr_core_event_histogram_t histogram_data[NUMBER_OF_HS_VOLTAGE_SCALES][NUMBER_OF_HS_TEMP_SCALES] = {{{0}}};
    data_proc_tlm_cmpnt_get_pwr_core_histogram_data(TEST_CORE_ID_5, &histogram_data);
}

TEST_FUNCTION(test_get_pwr_soc_pc3_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    pwr_soc_event_pc3_t pc3_data = {0};
    data_proc_tlm_cmpnt_get_pwr_soc_pc3_data(&pc3_data);
}

TEST_FUNCTION(test_get_pwr_soc_vr_rail_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    pwr_soc_event_vr_rail_t vr_rail_data = {{0}};
    data_proc_tlm_cmpnt_get_pwr_soc_vr_rail_data(TEST_RAIL_ID_2, &vr_rail_data);
}

TEST_FUNCTION(test_get_pwr_soc_hnf_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    pwr_soc_event_hnf_t hnf_data = {0};
    data_proc_tlm_cmpnt_get_pwr_soc_hnf_data(TEST_HNF_CHANN_ID_1, &hnf_data);
}

TEST_FUNCTION(test_get_pwr_soc_dimm_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    pwr_soc_event_dimm_t dimm_data = {{0}};
    data_proc_tlm_cmpnt_get_pwr_soc_dimm_data(TEST_DIMM_CHANN_ID_3, &dimm_data);
}

TEST_FUNCTION(test_get_pwr_soc_snsr_temp_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    pwr_soc_event_sensor_temp_t snsr_temp_data = {0};
    data_proc_tlm_cmpnt_get_pwr_soc_snsr_temp_data(TEST_SNSR_ID_0, &snsr_temp_data);
}

TEST_FUNCTION(test_get_pwr_mpam_pstate_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    pwr_event_mpam_pstate_t mpam_pstate_array[NUMBER_OF_PSTATES] = {{0}};
    data_proc_tlm_cmpnt_get_pwr_mpam_pstate_data(TEST_MPAM_ID_4, &mpam_pstate_array);
}

TEST_FUNCTION(test_get_pwr_soc_mpam_throttle_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    pwr_event_mpam_throttle_t mpam_throttle_data = {0};
    data_proc_tlm_cmpnt_get_pwr_soc_mpam_throttle_data(TEST_MPAM_ID_4, &mpam_throttle_data);
}

TEST_FUNCTION(test_get_perf_soc_core_summary_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    perf_core_event_summary_t core_summary_data = {{0}};
    data_proc_tlm_cmpnt_get_perf_soc_core_summary_data(TEST_CORE_ID_5, &core_summary_data);
}

TEST_FUNCTION(test_get_perf_soc_rail_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    perf_soc_event_rail_t rail_data = {0};
    data_proc_tlm_cmpnt_get_perf_soc_rail_data(TEST_RAIL_ID_2, &rail_data);
}

TEST_FUNCTION(test_get_perf_soc_dimm_runtime_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    perf_soc_event_dimm_runtime_t dimm_runtime_data = {0};
    data_proc_tlm_cmpnt_get_perf_soc_dimm_runtime_data(TEST_DIMM_CHANN_ID_3, &dimm_runtime_data);
}

TEST_FUNCTION(test_get_perf_soc_dimm_config_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    perf_soc_event_dimm_config_t dimm_config_data = {0};
    data_proc_tlm_cmpnt_get_perf_soc_dimm_config_data(&dimm_config_data);
}

TEST_FUNCTION(test_get_perf_soc_snsr_temp_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    perf_soc_event_sensor_temp_t snsr_temp_data = {0};
    data_proc_tlm_cmpnt_get_perf_soc_snsr_temp_data(TEST_SNSR_ID_0, &snsr_temp_data);
}

TEST_FUNCTION(test_get_perf_core_amu_data, test_setup, test_teardown)
{
    // the api is currently just stubbed out
    // this test will be updated with https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2031663
    perf_core_event_amu_counters_t amu_data = {0};
    data_proc_tlm_cmpnt_get_perf_core_amu_data(TEST_CORE_ID_5, &amu_data);
}
