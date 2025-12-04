//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_inst_soc_voltage_rails.cpp
 * [IB Telemetry] Log Instantaneous SOC Voltage Rails Integration Test
 *
 * Test functionality and flow of VR current sensor FIFO processing and instantaneous record creation
 *
 * Step-1:
 * Create data set - Initialize test data for VR current sensor FIFO entries
 * - Set up multiple FIFO entries with distinct values for each VR rail (0-7)
 * - Each entry contains current (centi-amps) and voltage (millivolts) data
 * - Convert raw sensor values using CONVERT_CENTIAMPS_TO_MILLIAMPS macro
 * - Final entry values will be retained ("last value wins" behavior because this is testing instantaneous reporting)
 *
 * Step-2:
 * Call data_smpl_process_vr_current_sensor_fifo() to process FIFO entries
 * - Mock sensor FIFO polling to return multiple entries sequentially
 * - Process all entries through VR current sensor FIFO processing pipeline
 * - The value in the last sensor FIFO entry for each rail will be logged in the record
 * - Update soc_rt structure with latest current and voltage values per rail
 *
 * Step-3:
 * Call package_create_inst_soc_rail_record() to create instantaneous record
 * - Package the processed VR current and voltage data into record structure
 * - Record contains instantaneous telemetry for all 8 VR rails
 * - Includes current (mA), voltage (mV), and temperature (dC) per rail
 *
 * Step-4:
 * Verify record contents against expected values
 * - Check that record contains the last FIFO entry values for each rail
 * - Validate record header (timestamp, record number, collections count)
 * - Verify collection headers and element IDs are correctly set
 * - Ensure "last value wins" behavior (because this is testing instantaneous reporting) - record reflects most recent sensor data
 * - Cross-check record data against runtime info (soc_rt) for consistency
 *
 * This test validates the "last value wins" behavior (because this is testing instantaneous reporting),
 * ensuring the record captures the most recent sensor readings from VR current FIFO.
 *
 * Record structs are in telemetry_package_defs.h
 */

// @SSI_integration_Test
// @SSI_integration_Test:power_telemetry
// TEST_CASE_ID:2889384

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
#include <package_creation_i.h>
#include <sensor_fifo_service.h>
#include <telemetry_package_defs.h>
}

#define CONVERT_CENTIAMPS_TO_MILLIAMPS(ca) ((ca) * 10)
#define NO_OF_FIFO_ENTRIES                 3

// Debug flag for controlling prints
static bool g_print_debug = true;

static int32_t test_setup(void** state)
{
    FPFW_UNUSED(state);
    reset_pwr_tlm_data();
    setup_snsr_fifo_is_empty();
    in_band_publishing_active = true;

    // Initialize telemetry component
    data_proc_tlm_cmpnt_init(0, false, 0, true); // die_id=0, is_single_die=false, mpam_vm_mem_fixed_pwr_mW=0, all_zero_filtering_enable=true

    return 0;
}

static int32_t test_teardown(void** state)
{
    FPFW_UNUSED(state);
    reset_pwr_tlm_data();
    return 0;
}

TEST_FUNCTION(test_inst_soc_voltage_rails, test_setup, test_teardown)
{
    if (g_print_debug)
    {
        printf("\n=== [IB Telemetry] Log Instantaneous SOC Voltage Rails Integration Test ===\n");
    }

    // Step 1: Create data set
    if (g_print_debug)
    {
        printf("Step 1: Creating VR current sensor FIFO test data set\n");
    }

    // VR current and voltage test data for multiple FIFO entries with EXPLICIT expected values
    // "Last value wins" behavior (because this is testing instantaneous reporting) - final entry (Entry 2) values should appear in the record
    // Expected values are explicitly defined alongside input data to prevent false positives

    // FIFO Entry 2 - FINAL VALUES (these will appear in record after "last value wins" processing because this is testing instantaneous reporting)
    uint16_t final_entry_current_cA[MAX_NUM_OF_VR_RAILS] = {200, 211, 222, 233, 244, 255, 266, 277};
    uint16_t final_entry_voltage_mV[MAX_NUM_OF_VR_RAILS] = {1200, 1225, 1250, 1275, 1300, 1325, 1350, 1375};
    uint64_t final_entry_timestamp_us = 3000000;

    // EXPLICIT EXPECTED VALUES - manually calculated from final entry to prevent false positives
    uint32_t expected_current_mA[MAX_NUM_OF_VR_RAILS] = {
        2000, // Rail 0: 200 cA × 10 = 2000 mA
        2110, // Rail 1: 211 cA × 10 = 2110 mA
        2220, // Rail 2: 222 cA × 10 = 2220 mA
        2330, // Rail 3: 233 cA × 10 = 2330 mA
        2440, // Rail 4: 244 cA × 10 = 2440 mA
        2550, // Rail 5: 255 cA × 10 = 2550 mA
        2660, // Rail 6: 266 cA × 10 = 2660 mA
        2770  // Rail 7: 277 cA × 10 = 2770 mA
    };

    uint16_t expected_voltage_mV[MAX_NUM_OF_VR_RAILS] = {
        1200, // Rail 0: direct copy from final entry
        1225, // Rail 1: direct copy from final entry
        1250, // Rail 2: direct copy from final entry
        1275, // Rail 3: direct copy from final entry
        1300, // Rail 4: direct copy from final entry
        1325, // Rail 5: direct copy from final entry
        1350, // Rail 6: direct copy from final entry
        1375  // Rail 7: direct copy from final entry
    };

    uint16_t expected_temperature_dC[MAX_NUM_OF_VR_RAILS] = {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0 // VR current processing doesn't set temperature
    };

    // Complete FIFO test data structure (includes intermediate entries that will be overwritten)
    struct
    {
        uint16_t vr_current_cA[MAX_NUM_OF_VR_RAILS];
        uint16_t vr_voltage_mV[MAX_NUM_OF_VR_RAILS];
        uint64_t timestamp;
    } vr_fifo_entries[NO_OF_FIFO_ENTRIES] = {
        // FIFO Entry 0 - Initial values (will be overwritten) - Pattern: 50+rail*5
        {{50, 55, 60, 65, 70, 75, 80, 85}, {800, 810, 820, 830, 840, 850, 860, 870}, 1000000},
        // FIFO Entry 1 - Intermediate values (will be overwritten) - Pattern: 100+rail*7
        {{100, 107, 114, 121, 128, 135, 142, 149}, {900, 915, 930, 945, 960, 975, 990, 1005}, 2000000},
        // FIFO Entry 2 - FINAL VALUES (copy from explicit arrays above)
        {{0}, {0}, 0} // Will be populated from explicit arrays below
    };

    // Populate final FIFO entry from explicit arrays to maintain clear traceability
    for (int32_t rail = 0; rail < MAX_NUM_OF_VR_RAILS; rail++)
    {
        vr_fifo_entries[2].vr_current_cA[rail] = final_entry_current_cA[rail];
        vr_fifo_entries[2].vr_voltage_mV[rail] = final_entry_voltage_mV[rail];
    }
    vr_fifo_entries[2].timestamp = final_entry_timestamp_us;
    if (g_print_debug)
    {
        printf("Input data and explicit expected values defined together:\n");
        printf("Final FIFO entry (Entry 2) input data:\n");
        for (int32_t rail = 0; rail < MAX_NUM_OF_VR_RAILS; rail++)
        {
            printf("  Rail[%d]: Input=%dcA/%dmV → Expected=%dmA/%dmV/%ddC\n",
                   rail,
                   final_entry_current_cA[rail],
                   final_entry_voltage_mV[rail],
                   expected_current_mA[rail],
                   expected_voltage_mV[rail],
                   expected_temperature_dC[rail]);
        }
    }

    // Step 2: Call data_smpl_process_vr_current_sensor_fifo()
    if (g_print_debug)
    {
        printf("\nStep 2: Processing VR current sensor FIFO entries\n");
        printf("Calling data_smpl_process_vr_current_sensor_fifo() with %d entries...\n", NO_OF_FIFO_ENTRIES);
    }

    // Mock multiple FIFO entries
    for (int32_t entry = 0; entry < NO_OF_FIFO_ENTRIES; entry++)
    {
        vr_current_t mock_vr_data = {0};

        // Set up mock data for current FIFO entry
        mock_vr_data.timestamp = vr_fifo_entries[entry].timestamp;
        for (int32_t rail = 0; rail < MAX_NUM_OF_VR_RAILS; rail++)
        {
            mock_vr_data.vr_current_cA[rail] = vr_fifo_entries[entry].vr_current_cA[rail];
            mock_vr_data.vr_voltage_mV[rail] = vr_fifo_entries[entry].vr_voltage_mV[rail];
        }

        if (g_print_debug)
        {
            printf("  Processing Entry %d: Rail[0]=%dcA/%dmV, Rail[7]=%dcA/%dmV\n",
                   entry,
                   mock_vr_data.vr_current_cA[0],
                   mock_vr_data.vr_voltage_mV[0],
                   mock_vr_data.vr_current_cA[7],
                   mock_vr_data.vr_voltage_mV[7]);
        }

        // Mock sensor FIFO polling to return this entry
        // Order: curr_data_is_valid, more_entries, then data pointer if valid
        will_return(__wrap_sensor_fifo_svc_poll_vr_current, true); // curr_data_is_valid
        will_return(__wrap_sensor_fifo_svc_poll_vr_current, (entry < NO_OF_FIFO_ENTRIES - 1)); // more_entries
        will_return(__wrap_sensor_fifo_svc_poll_vr_current, &mock_vr_data);                    // data pointer
    }

    // Process all VR current FIFO entries
    data_smpl_process_vr_current_sensor_fifo();

    // Verify that runtime info contains the last FIFO entry values
    if (g_print_debug)
    {
        printf("\nVerifying runtime info contains last FIFO entry values:\n");
    }
    for (int32_t rail = 0; rail < MAX_NUM_OF_VR_RAILS; rail++)
    {
        // Extract actual values from runtime with unit-suffixed variables for clarity
        uint32_t actual_current_mA = soc_rt.latest_rail_current_mA[rail];
        uint16_t actual_voltage_mV = soc_rt.latest_rail_voltage_mV[rail];

        if (g_print_debug)
        {
            printf("  Rail[%d]: RT Current=%dmA (expected %dmA), RT Voltage=%dmV (expected %dmV)\n",
                   rail,
                   actual_current_mA,
                   expected_current_mA[rail],
                   actual_voltage_mV,
                   expected_voltage_mV[rail]);
        }

        // Verify with clearly named variables showing units
        assert_int_equal(actual_current_mA, expected_current_mA[rail]);
        assert_int_equal(actual_voltage_mV, expected_voltage_mV[rail]);
    }

    // Step 3: Call package_create_inst_soc_rail_record() to create record
    if (g_print_debug)
    {
        printf("\nStep 3: Creating instantaneous SoC rail record\n");
        printf("Calling package_create_inst_soc_rail_record()...\n");
    }

    inst_soc_record_rail_t rail_record = {{0}};
    uint32_t record_size_bytes = package_create_inst_soc_rail_record(&rail_record);

    if (g_print_debug)
    {
        printf("Created instantaneous SoC rail record, size: %u bytes\n", record_size_bytes);
    }
    assert_int_not_equal(record_size_bytes, 0);

    // Step 4: Verify record contents against expected values
    if (g_print_debug)
    {
        printf("\nStep 4: Verifying record contents against expected values\n");

        // Verify record header
        printf("Verifying record header...\n");
    }

    assert_int_not_equal(rail_record.record_header.timestamp_uS, 0);
    assert_int_not_equal(rail_record.record_header.record_number, 0);
    assert_int_equal(rail_record.record_header.number_of_collections, MAX_NUM_OF_VR_RAILS);

    if (g_print_debug)
    {
        printf("  ✓ Record header validation passed\n");
    }

    // Verify each rail collection contains the last FIFO entry values
    if (g_print_debug)
    {
        printf("Verifying rail data matches last FIFO entry values:\n");
    }
    for (int32_t rail = 0; rail < MAX_NUM_OF_VR_RAILS; rail++)
    {
        // Extract actual record values with unit-suffixed variables for clarity
        uint32_t actual_record_current_mA = rail_record.rail_collection[rail].rail_element.current_mA;
        uint16_t actual_record_voltage_mV = rail_record.rail_collection[rail].rail_element.voltage_mV;
        uint16_t actual_record_temperature_dC = rail_record.rail_collection[rail].rail_element.temperature_dC;

        if (g_print_debug)
        {
            printf("  Rail[%d] Record: Current=%dmA, Voltage=%dmV, Temperature=%ddC\n",
                   rail,
                   actual_record_current_mA,
                   actual_record_voltage_mV,
                   actual_record_temperature_dC);
        }

        // Verify current and voltage match the last FIFO entry values - units clearly visible
        assert_int_equal(actual_record_current_mA, expected_current_mA[rail]);
        assert_int_equal(actual_record_voltage_mV, expected_voltage_mV[rail]);

        // Temperature should be 0 (VR current processing doesn't set temperature) - use explicit expected value
        assert_int_equal(actual_record_temperature_dC, expected_temperature_dC[rail]);

        // Verify collection header
        assert_int_equal(rail_record.rail_collection[rail].collection_header.collection_id, rail);
        assert_int_equal(rail_record.rail_collection[rail].collection_header.element_id,
                         INST_TELEMETRY_ELEMENT_SOC_VOLTAGE_RAILS);

        if (g_print_debug)
        {
            printf("    ✓ Rail[%d] data integrity verified\n", rail);
        }
    }

    // Final validation: Cross-check record against runtime info
    if (g_print_debug)
    {
        printf("\nFinal validation - cross-checking record against runtime info:\n");
    }
    for (int32_t rail = 0; rail < MAX_NUM_OF_VR_RAILS; rail++)
    {
        // Cross-check with unit-explicit variables for maximum clarity
        uint32_t record_current_mA = rail_record.rail_collection[rail].rail_element.current_mA;
        uint32_t runtime_current_mA = soc_rt.latest_rail_current_mA[rail];
        uint16_t record_voltage_mV = rail_record.rail_collection[rail].rail_element.voltage_mV;
        uint16_t runtime_voltage_mV = soc_rt.latest_rail_voltage_mV[rail];

        assert_int_equal(record_current_mA, runtime_current_mA);
        assert_int_equal(record_voltage_mV, runtime_voltage_mV);
    }

    if (g_print_debug)
    {
        printf("  ✓ Record perfectly matches runtime info\n");
        printf("\n=== [IB Telemetry] Log Instantaneous SOC Voltage Rails Integration Test Summary ===\n");
        printf("✓ Processed %d VR current FIFO entries\n", NO_OF_FIFO_ENTRIES);
        printf("✓ Verified 'last value wins' behavior (because this is testing instantaneous reporting) - "
               "record contains final FIFO entry values\n");
        printf("✓ Created instantaneous SoC rail record (%u bytes)\n", record_size_bytes);
        printf("✓ Validated record structure and data integrity for all %d rails\n", MAX_NUM_OF_VR_RAILS);
        printf("Test end: test_inst_soc_voltage_rails\n");
    }
}