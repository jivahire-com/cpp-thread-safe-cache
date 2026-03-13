//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file log_core_instantaneous_data_integration_test.cpp
 * Test functionality and flow of instantaneous core data collection and packaging
 *
 * Test Overview:
 * This integration test validates the complete flow of instantaneous core data collection,
 * processing, and packaging. It tests the real-time data capture for core metrics including:
 * - Core voltage, current, temperature, power
 * - Performance states (pstate) and power states (cstate)
 * - Frequency, throttling status, and MPAM configurations
 * - Velocity boost and rack priority settings
 *
 * Test Steps:
 * Step-1: Setup and Initialize
 * - Initialize mock sensor data for multiple cores
 * - Set up current, voltage, temperature, and pstate data
 * - Configure cstate, throttling, and MPAM settings
 * - Enable all cores for instantaneous data collection
 *
 * Step-2: Process Data
 * - Call individual sensor processing functions for targeted data
 * - Process pstate sensor data: data_smpl_process_pstate_sensor_fifo()
 * - Process core current sensor data: data_smpl_process_core_current_sensor_fifo()
 * - Process tile temperature sensor data: data_smpl_process_tile_temperature_sensor_fifo()
 * - Process tile voltage sensor data: data_smpl_process_tile_voltage_sensor_fifo()
 *
 * - Note: Not mocking all sensors with data_proc_tlm_cmpnt_process_input_data() anymore as it is in other tests
 *
 * Step-3: Create Instantaneous Record
 * - Generate instantaneous core summary record using package creation API
 * - Populate all core elements with processed data
 * - Validate record structure and data integrity
 *
 * Step-4: Verify Results
 * - Check all instantaneous core data fields are correctly populated
 * - Validate voltage, current, temperature, and power values
 * - Verify pstate, cstate, frequency, and throttling information
 * - Ensure MPAM, velocity boost, and priority settings are accurate
 *
 * Test Execution Flow (Functions Called):
 * Step 1: reset_pwr_tlm_data() → enable cores → package_create_enable_disable_inst_record()
 * Step 2: Process Sensors (CRITICAL ORDER):
 *    a) data_smpl_process_pstate_sensor_fifo() [data_sampling.c] - Sets throttling_status
 *    b) data_smpl_process_tile_voltage_sensor_fifo() [data_sampling.c] - Voltage telemetry (MUST be before current)
 *    c) data_smpl_process_core_current_sensor_fifo() [data_sampling.c] - Updates pstate_from_current_pkt if throttling, calculates power using P=V×I
 *    d) data_smpl_process_tile_temperature_sensor_fifo() [data_sampling.c] - Temperature telemetry
 * Step 3: data_proc_tlm_cmpnt_get_inst_soc_core_summary_data() [in_band_package_interface.c]
 *    - Validates conditional pstate logic: if throttling → use pstate_from_current_pkt, else → use latest_pstate
 *    → package_create_inst_core_summary_record() - Creates final telemetry structure
 * Step 4: Assert calculations:
 *    - Current: avg * CORE_CURRENT_CONVERSION_FACTOR
 *    - Voltage: DOUT2MILLIVOLTS(vcore0)
 *    - Power: (current_mA * voltage_mV) / 1000 (P = V × I formula)
 *    - Pstate/Cstate: Direct from sensor data (validates throttling-dependent logic)
 *    - Throttling Rack Priority: From configuration (config->throttling_rack_priority)
 *    - Frequency: From DVFS table (Just printing it and not doing assertion)
 *    - Yet to be implemented:
 *    - Velocity Boost Priority: From configuration (config->velocity_boost_priority)
 *
 * Record structs are in telemetry_package_defs.h
 * Test validates correct sensor processing order and throttling state-dependent pstate selection.
 */

// @SSI_integration_Test
// @SSI_integration_Test:power_telemetry
// TEST_CASE_ID:2779239

#include "telemetry_functional.h"

#include <CMockaWrapper.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

extern "C" {
#include <FpFwCMocka.h>
#include <FpFwUtils.h>
#include <compute_metrics_i.h>
#include <data_proc_tlm_cmpnt.h>
#include <data_sampling_i.h>
#include <fpfw_status.h>
#include <libs/event_trace/trace/inc/event_trace_providers.h>
#include <package_creation_i.h>
#include <sensor_fifo_service.h>
#include <telemetry_package_defs.h>
extern int g_enable_mock_pstate;
}

// Global variable to control debug printing
static bool g_print_logs = true;

static int32_t test_setup(void** state)
{
    FPFW_UNUSED(state);
    reset_pwr_tlm_data();
    setup_cstate_tfa_functional_mock_buffer();
    // Enable all cores for instantaneous data collection
    for (unsigned int core = 0; core < NUMBER_OF_CORES_PER_DIE; ++core)
    {
        core_is_active[core] = true;
    }

    // Enable instantaneous record for core data
    package_create_enable_disable_inst_record(INST_TELEMETRY_ELEMENT_CORE, true);

    return 0;
}

static int32_t test_teardown(void** state)
{
    FPFW_UNUSED(state);
    return 0;
}

// Test configuration structure for instantaneous data
typedef struct
{
    // Core current data
    uint16_t current_avg_mA;
    uint16_t current_min_mA;
    uint16_t current_max_mA;
    uint16_t voltage_mV;
    uint16_t power_mW;
    uint8_t current_sensor_pstate; // used for current sensor
    uint8_t pstate_sensor_value;   // used for pstate sensor

    // Temperature data
    int32_t tile_temp_dC;

    // Voltage data
    uint16_t tile_voltage_avg_mV;

    // Cstate and timing data
    uint32_t cstate_entry_latency_uS;
    uint32_t cstate_exit_latency_uS;

    // Performance and configuration
    uint16_t frequency_Mhz;
    uint8_t cstate;
    uint8_t plimit;
    uint8_t mpam_id;
    uint8_t velocity_boost_priority;
    uint8_t throttling_type;
    uint8_t throttling_rack_priority;
} instantaneous_test_config_t;

// Test configurations for different scenarios
// Note: To properly test conditional pstate logic, we need different pstate values in each sensor
// Configuration 1: Normal operation with moderate values (no throttling)
// Configuration 2: High performance state with increased power (no throttling)
// Configuration 3: Throttled state with thermal limits (throttling active)
const instantaneous_test_config_t test_configs[] = {
    // Config 1: Normal operation (throttling_type = 0 → uses pstate sensor value)
    // latest_pstate will be from pstate sensor i.e pstate=21 will be used since no throttling
    // velocity_boost_priority=1 (But not being asserted), throttling_rack_priority=0 (low priority, no throttling)
    {100, 90, 110, 800, 220, 10, 21, 650, 820, 1000, 50, 2400, 0, 15, 0, 1, 0, 0}, // current_sensor_pstate=10, pstate_sensor_value=21
    // Config 2: High performance with rack throttling (throttling_type = 3 → RACK_THROTTLING_START)
    // latest_pstate will be from current sensor i.e pstate=5 will be used since we're using rack throttling
    // velocity_boost_priority=3 (But not being asserted), throttling_rack_priority=1 (medium priority, rack throttling active)
    {150, 140, 160, 900, 198, 5, 22, 750, 920, 500, 25, 3200, 0, 20, 1, 3, 3, 1}, // current_sensor_pstate=5, pstate_sensor_value=22
    // Config 3: Rack throttled state (throttling_type = 3 = RACK_THROTTLING_START → uses current sensor value from current sensor)
    // latest_pstate will be from current sensor i.e pstate=15 will be used since rack throttling continues
    // velocity_boost_priority=5 (But not being asserted), throttling_rack_priority=2 (high priority, with rack throttling)
    {80, 70, 90, 700, 154, 15, 23, 850, 720, 2000, 100, 1800, 1, 10, 2, 5, 3, 2}}; // current_sensor_pstate=15, pstate_sensor_value=23

TEST_FUNCTION(test_core_instantaneous_data_integration, test_setup, test_teardown)
{
    const uint32_t target_core = 0; // Focus on core 0 for detailed testing
    const uint32_t num_iterations = sizeof(test_configs) / sizeof(test_configs[0]);

    if (g_print_logs)
    {
        printf("=== Starting Core Instantaneous Data Integration Test ===\n");
        printf("Testing %u different configurations on core %u\n\n", num_iterations, target_core);
    }

    // Set up mock gtimer frequency values for all iterations (used by pstate sensor timestamp conversion)
    // Only pstate sensor processing calls gtimer via data_util_convert_systick_to_microseconds()
    will_return_always(__wrap_gtimer_prodfw_get_frequency, 1000000);

    for (uint32_t iteration = 0; iteration < num_iterations; iteration++)
    {
        const instantaneous_test_config_t* config = &test_configs[iteration];

        if (g_print_logs)
        {
            printf("--- Iteration %u: %s ---\n",
                   iteration + 1,
                   iteration == 0   ? "Normal Operation"
                   : iteration == 1 ? "High Performance"
                                    : "Throttled State");
        }

        // Re-enable instantaneous record for core data
        package_create_enable_disable_inst_record(INST_TELEMETRY_ELEMENT_CORE, true);

        // Create mock current data
        core_current_t mock_current_data = {0};
        mock_current_data.timestamp = __wrap_exec_tlm_cmpnt_get_timestamp_microseconds();
        mock_current_data.data.avg = config->current_avg_mA;
        mock_current_data.data.min = config->current_min_mA;
        mock_current_data.data.max = config->current_max_mA;
        mock_current_data.data.volt = config->voltage_mV;
        mock_current_data.data.pwr = (uint16_t)((config->power_mW + 16) / 32); // Round to nearest integer for conversion
        // Current sensor pstate - used when throttling is active
        mock_current_data.data.pstate = config->current_sensor_pstate;
        mock_current_data.data.mpam_id_low = config->mpam_id;
        mock_current_data.data.change = 0;

        // Create mock temperature data
        tile_temp_t mock_temp_data = {0};
        mock_temp_data.timestamp = __wrap_exec_tlm_cmpnt_get_timestamp_microseconds();
        mock_temp_data.temp1.temp0 = config->tile_temp_dC;

        // Create mock voltage data
        tile_voltage_t mock_voltage_data = {0};
        mock_voltage_data.timestamp = __wrap_exec_tlm_cmpnt_get_timestamp_microseconds();
        mock_voltage_data.data.vcore0 = config->tile_voltage_avg_mV;
        mock_voltage_data.data.vcore1 = config->tile_voltage_avg_mV;
        mock_voltage_data.data.vcpu = config->tile_voltage_avg_mV + 20;
        mock_voltage_data.data.vsys = config->tile_voltage_avg_mV + 50;

        // Create mock pstate data
        pstate_telem_t mock_pstate_data = {0};
        mock_pstate_data.timestamp = __wrap_exec_tlm_cmpnt_get_timestamp_microseconds();
        // Use explicit pstate_sensor_value from test config for mock_pstate_data
        mock_pstate_data.data.pstate = config->pstate_sensor_value;
        mock_pstate_data.data.cstate = config->cstate;
        mock_pstate_data.data.plimit = config->plimit;
        mock_pstate_data.data.core = target_core;
        mock_pstate_data.data.throttle_status = config->throttling_type;
        mock_pstate_data.data.vm_throttle_pri = config->throttling_rack_priority;

        // Process only the specific sensor data we need
        if (g_print_logs)
            printf("Processing sensor data...\n");

        // Process pstate sensor data FIRST - to set up throttling status
        if (g_print_logs)
            printf("  - Processing pstate sensor data\n");
        g_enable_mock_pstate = 1;                                    // Enable pstate mocking
        will_return(__wrap_sensor_fifo_svc_poll_core_pstate, true);  // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_core_pstate, false); // more_entries
        will_return(__wrap_sensor_fifo_svc_poll_core_pstate, &mock_pstate_data);
        data_smpl_process_pstate_sensor_fifo();
        g_enable_mock_pstate = 0; // Disable pstate mocking

        // Process tile voltage sensor data SECOND - MUST be before current to calculate power correctly
        if (g_print_logs)
            printf("  - Processing tile voltage sensor data\n");
        will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, target_core);
        will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, true);  // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, false); // more_entries
        will_return(__wrap_sensor_fifo_svc_poll_tile_voltage, &mock_voltage_data);
        data_smpl_process_tile_voltage_sensor_fifo();

        // Process core current sensor data THIRD - uses voltage to calculate power (P=V*I)
        if (g_print_logs)
            printf("  - Processing core current sensor data\n");
        will_return(__wrap_sensor_fifo_svc_poll_core_current, target_core);
        will_return(__wrap_sensor_fifo_svc_poll_core_current, true);  // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_core_current, false); // more_entries
        will_return(__wrap_sensor_fifo_svc_poll_core_current, &mock_current_data);
        data_smpl_process_core_current_sensor_fifo();

        // Process tile temperature sensor data
        if (g_print_logs)
            printf("  - Processing tile temperature sensor data\n");
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, target_core);
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, true);  // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, false); // more_entries
        will_return(__wrap_sensor_fifo_svc_poll_tile_temperature, &mock_temp_data);
        data_smpl_process_tile_temperature_sensor_fifo();

        // Get instantaneous core summary data
        inst_core_element_summary_t core_summary_data;
        data_proc_tlm_cmpnt_get_inst_soc_core_summary_data(target_core, &core_summary_data);

        // Create instantaneous core summary record
        inst_core_record_summary_t inst_record;
        uint32_t record_size = package_create_inst_core_summary_record(&inst_record);

        if (g_print_logs)
            printf("Created instantaneous record (size: %u bytes)\n", record_size);

        // Verify the instantaneous data for the target core
        const inst_core_element_summary_t* core_element =
            &inst_record.inst_core_summary_collection[target_core].summary_element;

        // Calculate expected values based on actual implementation in in_band_package_interface.c
        // Current: core_rt[core_id].latest_current_mA = (uint16_t)(core_current_entry->data.avg * CORE_CURRENT_CONVERSION_FACTOR)
        int32_t expected_current_mA = config->current_avg_mA * CORE_CURRENT_CONVERSION_FACTOR;
        // Voltage: core_rt[core_id].latest_voltage_mV = DOUT2MILLIVOLTS(tile_voltage_entry->data.vcore0)
        int32_t expected_voltage_mV = DOUT2MILLIVOLTS(config->tile_voltage_avg_mV);
        // Temperature: core_rt[core_id].latest_max_value_dC (appears to be 0 for instantaneous data)
        int32_t expected_temp_dC = 0;
        // Power: Now calculated using P = V × I formula
        // Power (mW) = (current_mA * voltage_mV) / 1000
        uint16_t expected_power_mW = ((uint32_t)expected_current_mA * expected_voltage_mV) / 1000;
        // Frequency: dvfs_get_freq_from_plimit(current_pstate) - calls external DVFS function
        // So skipping exact frequency assertion against expected value.
        // Pstate: Conditional logic based on throttling status
        // Pstate source logic:
        // - If NO_THROTTLING: use pstate value from pstate sensor packet
        // - If RACK_THROTTLING or CURRENT_THROTTLING: use pstate value from current sensor packet
        uint8_t expected_pstate;
        if (config->throttling_type == 0) // NO_THROTTLING
        {
            expected_pstate = config->pstate_sensor_value;
        }
        else if (config->throttling_type == 1 || config->throttling_type == 3) // True for CURRENT_THROTTLING or RACK_THROTTLING
        {
            // For throttling scenarios, expected_pstate comes from current sensor value i.e: config->current_sensor_pstate
            expected_pstate = config->current_sensor_pstate;
        }
        else
        {
            expected_pstate = config->pstate_sensor_value;
        }

        // Cstate: Based on data_smpl_parse_cstate(), core_rt[core_id].latest_cstate = packet_cstate
        // Should match the cstate from our mock pstate_telem_t data
        uint8_t expected_cstate = config->cstate;
        // MPAM ID: Should match the value from current sensor data
        uint8_t expected_mpam_id = config->mpam_id;
        // Velocity boost priority: Should match the configuration value
        uint8_t expected_velocity_boost_priority = config->velocity_boost_priority;
        // Throttling rack priority: Should match the configuration value
        uint8_t expected_throttling_rack_priority = config->throttling_rack_priority;
        if (g_print_logs)
        {
            printf("\nValidating instantaneous core data:\n");
            printf("Core %u Summary Data:\n", target_core);
            printf("  Voltage: %u mV (expected: %d mV)\n", core_element->voltage_mV, expected_voltage_mV);
            printf("  Current: %u mA (expected: %d mA)\n", core_element->current_mA, expected_current_mA);
            printf("  Temperature: %u dC (expected: %d dC)\n", core_element->temperature_dC, expected_temp_dC);
            printf("  Power: %u mW (expected: %u mW, config input: %u mW)\n",
                   core_element->power_mW,
                   expected_power_mW,
                   config->power_mW);
            printf("  Frequency: %u MHz (DVFS-determined - Skipping expected value)\n", core_element->frequency_Mhz);

            // Show conditional pstate logic validation
            printf("  === Conditional Pstate Logic Validation ===\n");
            printf("  Throttling Status: %u (%s)\n",
                   config->throttling_type,
                   config->throttling_type == 0 ? "NO_THROTTLE" : "THROTTLING");
            printf("  Pstate Source: %s\n",
                   config->throttling_type == 0 ? "pstate sensor (latest_pstate)" : "current sensor (pstate_from_current_pkt)");
            printf("  Pstate from pstate sensor: %u\n", mock_pstate_data.data.pstate);
            printf("  Pstate from current sensor: %u\n", mock_current_data.data.pstate);
            printf("  Expected pstate (conditional): %u\n", expected_pstate);
            printf("  Actual pstate: %u\n", core_element->pstate);
            printf("  ==========================================\n");

            printf("  Cstate: %u (expected: %u, config input: %u)\n",
                   core_element->cstate,
                   expected_cstate,
                   config->cstate);
            printf("  Cstate Entry Latency: %u μs\n", core_element->cstate_entry_latency_uS);
            printf("  Cstate Exit Latency: %u μs\n", core_element->cstate_exit_latency_uS);
            printf("  MPAM ID: %u (expected: %u)\n", core_element->mpam_id, expected_mpam_id);
            printf("  Velocity Boost Priority: %u (expected: %u, not validated or asserted)\n",
                   core_element->velocity_boost_priority,
                   expected_velocity_boost_priority);
            printf("  Throttling Rack Priority: %u (expected: %u)\n",
                   core_element->throttling_rack_priority,
                   expected_throttling_rack_priority);

            // Debug: Print configuration for comparison
            printf("  Config Input - Current: %u mA, Voltage: %u mV, Power: %u mW, Pstate: %u, Cstate: %u\n",
                   config->current_avg_mA,
                   config->voltage_mV,
                   config->power_mW,
                   config->current_sensor_pstate,
                   config->cstate);
        }

        // Core assertions for instantaneous data validation with expected vs actual
        // All values should match our calculated expectations based on mock data and system behavior
        assert_int_equal(core_element->current_mA, expected_current_mA);
        assert_int_equal(core_element->voltage_mV, expected_voltage_mV);
        assert_int_equal(core_element->temperature_dC, expected_temp_dC);
        assert_int_equal(core_element->power_mW, expected_power_mW);
        assert_int_equal(core_element->pstate, expected_pstate);
        assert_int_equal(core_element->cstate, expected_cstate);
        assert_int_equal(core_element->throttling_rack_priority, expected_throttling_rack_priority);
        assert_int_equal(core_element->mpam_id, expected_mpam_id);
        // Validate velocity boost, and priority settings
        // Will wait for dev to confirm and then uncomment below assert statement.
        // Created a task for velocity boost.
        // TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2878540
        // assert_int_equal(core_element->velocity_boost_priority, expected_velocity_boost_priority);

        // Validate record structure integrity
        assert_true(record_size > 0);
        assert_int_equal(inst_record.record_header.number_of_collections, NUMBER_OF_CORES_PER_DIE);

        if (g_print_logs)
        {
            printf("All instantaneous data validation passed for iteration %u\n", iteration + 1);
            printf("-------------------------------------------\n\n");
        }
    }

    if (g_print_logs)
        printf("=== Core Instantaneous Data Integration Test Completed Successfully ===\n");
}
