
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file telemetry_functional_mocks.c
 * Provide mock functions for telemetry functional tests
 */

/*------------- Includes -----------------*/
#include "sensor_fifo_service.h"
#include "telemetry_functional.h"
#include "telemetry_package_defs.h"

#include <FpFwCMocka.h>
#include <FpFwUtils.h>
#include <compute_metrics_i.h>
#include <corebits.h>
#include <data_proc_tlm_cmpnt.h>
#include <semaphore_lib.h>
#include <sensor_fifo_service.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Droop count mock globals (shared for tests)

int g_enable_mock_pstate = 0;
int g_enable_mock_die_id = 0; // Enable/disable mocking for die_2_die_exch_get_this_die_id
bool test_snsr_fifo_is_empty[SENSOR_FIFO_MAX_ID] = {0};
cstate_instr_timestamp_t test_cstate_buf[(128 * 1024) / sizeof(cstate_instr_timestamp_t)];

// D2D PMU register mock implementation (following sensor FIFO mock pattern)
static uint32_t d2d_mock_counter_values[8][8][2]; // [interface][counter][low/high]
static bool d2d_pmu_initialized = false;

void setup_cstate_tfa_functional_mock_buffer()
{

    memset(test_cstate_buf, 0, sizeof(test_cstate_buf));
    cstate_tfa_timestamp_base = test_cstate_buf;
}

void update_stats(stats_t* stats, uint16_t latest_value)
{
    if (latest_value < stats->min || stats->count == 0)
    {
        stats->min = latest_value;
    }
    if (latest_value > stats->max || stats->count == 0)
    {
        stats->max = latest_value;
    }
    // Integer rounding up: add (count + 1) / 2 to numerator
    stats->avg =
        (uint16_t)(((stats->avg * stats->count) + latest_value + ((stats->count + 1) / 2)) / (stats->count + 1));
    stats->count++;
}

uint32_t time_t0 = 100;
sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_tile_temperature(tile_temp_t* temperature_data, uint16_t* tile_index)
{
    // Get the expected tile index value and store it in the pointer
    *tile_index = (uint16_t)mock();

    sensor_ram_poll_status_t status;
    status.curr_data_is_valid = mock_type(bool);
    status.more_entries = mock_type(bool);

    if (status.curr_data_is_valid)
    {
        tile_temp_t* mock_data = mock_ptr_type(tile_temp_t*);
        assert_non_null(mock_data);
        assert_non_null(temperature_data);

        memcpy(temperature_data, mock_data, sizeof(tile_temp_t));
        assert_in_range(*tile_index, 0, NUMBER_OF_TILES_PER_DIE - 1);
    }

    return status;
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_tile_voltage(tile_voltage_t* voltage_data, uint16_t* tile_index)
{

    // Check if mock values are available

    // Get the expected tile index value and store it in the pointer
    *tile_index = (uint16_t)mock();

    sensor_ram_poll_status_t status;
    status.curr_data_is_valid = mock_type(bool);
    status.more_entries = mock_type(bool);

    if (status.curr_data_is_valid)
    {
        tile_voltage_t* mock_data = mock_ptr_type(tile_voltage_t*);
        assert_non_null(mock_data);
        assert_non_null(voltage_data);

        memcpy(voltage_data, mock_data, sizeof(tile_voltage_t));
        assert_in_range(*tile_index, 0, NUMBER_OF_TILES_PER_DIE - 1);
    }

    return status;
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_core_current(core_current_t* current_data, uint16_t* core_index)
{
    *core_index = (uint16_t)mock();
    sensor_ram_poll_status_t status;
    status.curr_data_is_valid = mock_type(bool);
    status.more_entries = mock_type(bool);
    if (status.curr_data_is_valid)
    {
        core_current_t* mock_data = mock_ptr_type(core_current_t*);
        assert_non_null(mock_data);
        assert_non_null(current_data);
        memcpy(current_data, mock_data, sizeof(core_current_t));
        assert_in_range(*core_index, 0, NUMBER_OF_TILES_PER_DIE - 1);
    }
    return status;
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_core_pstate(pstate_telem_t* state_data)
{
    sensor_ram_poll_status_t status;

    if (g_enable_mock_pstate)
    {
        status.curr_data_is_valid = mock_type(bool);
        status.more_entries = mock_type(bool);

        if (status.curr_data_is_valid && state_data != NULL)
        {
            pstate_telem_t* mock_data = mock_ptr_type(pstate_telem_t*);
            if (mock_data != NULL)
            {
                memcpy(state_data, mock_data, sizeof(pstate_telem_t));
            }
        }
    }
    else
    {
        // No mock values provided - return "no data" by default
        status.curr_data_is_valid = false;
        status.more_entries = false;
    }

    return status;
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_vr_temperature(vr_temp_t* vr_temperature)
{
    sensor_ram_poll_status_t status;
    status.curr_data_is_valid = mock_type(bool);
    status.more_entries = mock_type(bool);
    if (status.curr_data_is_valid)
    {
        vr_temp_t* mock_data = mock_ptr_type(vr_temp_t*);
        assert_non_null(mock_data);
        assert_non_null(vr_temperature);

        memcpy(vr_temperature, mock_data, sizeof(vr_temp_t));
    }

    return status;
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_vr_current(vr_current_t* vr_current)
{
    sensor_ram_poll_status_t status;
    status.curr_data_is_valid = mock_type(bool);
    status.more_entries = mock_type(bool);
    if (status.curr_data_is_valid)
    {
        vr_current_t* mock_data = mock_ptr_type(vr_current_t*);
        assert_non_null(mock_data);
        assert_non_null(vr_current);

        memcpy(vr_current, mock_data, sizeof(vr_current_t));
    }

    return status;
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_soc_pvt_temperature(soc_pvt_temp_t* pvt_temperature)
{

    sensor_ram_poll_status_t status;
    status.curr_data_is_valid = mock_type(bool);
    status.more_entries = mock_type(bool);

    if (status.curr_data_is_valid)
    {
        soc_pvt_temp_t* mock_data = mock_ptr_type(soc_pvt_temp_t*);
        assert_non_null(mock_data);
        assert_non_null(pvt_temperature);

        memcpy(pvt_temperature, mock_data, sizeof(soc_pvt_temp_t));
    }

    return status;
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_dimm_info(sensor_ram_dimm_info_t* dimm_info)
{
    sensor_ram_poll_status_t status;
    status.curr_data_is_valid = mock_type(bool);
    status.more_entries = mock_type(bool);
    if (status.curr_data_is_valid)
    {
        sensor_ram_dimm_info_t* mock_data = mock_ptr_type(sensor_ram_dimm_info_t*);
        assert_non_null(mock_data);
        assert_non_null(dimm_info);

        memcpy(dimm_info, mock_data, sizeof(sensor_ram_dimm_info_t));
    }

    return status;
}

uint32_t __wrap_gtimer_prodfw_get_frequency(void)
{
    return mock_type(uint32_t);
}

void __wrap_sensor_fifo_svc_is_empty(bool (*is_empty)[SENSOR_FIFO_MAX_ID])
{
    bool(*mock_data)[SENSOR_FIFO_MAX_ID] = mock_ptr_type(bool(*)[SENSOR_FIFO_MAX_ID]);
    if (mock_data != NULL && is_empty != NULL)
    {
        memcpy(*is_empty, *mock_data, sizeof(bool) * SENSOR_FIFO_MAX_ID);
    }
}

// Reset Time
void reset_time(void)
{
    time_t0 = 100;
}
// Reset function for test setup
void reset_pwr_tlm_data(void)
{
    reset_time();
    data_proc_tlm_cmpnt_clear_pwr_tlm_data();
    comp_metrics_reset_local_2_min_metrics();
    fflush(stdout);
    setup_snsr_fifo_is_empty();
}

void setup_snsr_fifo_is_empty(void)
{
    // Set all FIFOs to empty
    for (uint32_t i = 0; i < SENSOR_FIFO_MAX_ID; i++)
    {
        test_snsr_fifo_is_empty[i] = false;
    }
}

// Modifying the timestamp function to return larger intervals
uint64_t __wrap_exec_tlm_cmpnt_get_timestamp_microseconds(void)
{
    time_t0 += MOCK_TIMESTAMP_INCREMENT; // Increment by constant value each time
    return time_t0;
}

void __wrap_initialize_semaphore(SEMAPHORE_ID id)
{
    FPFW_UNUSED(id);
}

uint32_t __wrap_read_semaphore(SEMAPHORE_ID id)
{
    FPFW_UNUSED(id);
    return 0; // Return a dummy value
}

void __wrap_wait_for_semaphore(SEMAPHORE_ID id, uint32_t key)
{
    FPFW_UNUSED(id);
    FPFW_UNUSED(key);
}

void __wrap_release_semaphore(SEMAPHORE_ID id)
{
    FPFW_UNUSED(id);
}

void __wrap_dvfs_c2_pcm_enable_aging_sensor_measurement(const uintptr_t cluster_pex_base_addr, uint8_t ro_index, uint8_t timer_cfg)
{
    FPFW_UNUSED(cluster_pex_base_addr);
    FPFW_UNUSED(ro_index);
    FPFW_UNUSED(timer_cfg);

    function_called();
}

int __wrap_dvfs_c2_pcm_aging_get_sensor_status(const uintptr_t cluster_pex_base_addr)
{
    FPFW_UNUSED(cluster_pex_base_addr);

    return mock_type(int);
}

int __wrap_dvfs_c2_get_pcm_bank_sensor_data(const uintptr_t cluster_pex_base_addr, uint32_t* bank_a_counter, uint32_t* bank_b_counter)
{
    FPFW_UNUSED(cluster_pex_base_addr);
    assert_non_null(bank_a_counter);
    assert_non_null(bank_b_counter);

    *bank_a_counter = mock_type(uint32_t);
    *bank_b_counter = mock_type(uint32_t);

    return mock_type(int);
}

// Mock event trace metadata symbols
// as per the original declarations in dcs_manifest.c
uint8_t _ProviderMetadata_et_msdata_start;
uint8_t _ProviderMetadata_et_msdata_end;
uint8_t _EventMetadata_et_msdata_start;
uint8_t _EventMetadata_et_msdata_end;

// CMocka wrapper for droop count function
void __wrap_pwr_tlm_core_exch_mcp_read_droop_counts(uint64_t* droop_counts)
{
    uint64_t* mock_data = (uint64_t*)mock_ptr_type(uint64_t*);
    if (mock_data && droop_counts)
    {
        for (unsigned int i = 0; i < NUMBER_OF_CORES_PER_DIE; ++i)
        {
            droop_counts[i] = mock_data[i];
        }
    }
}

// Mock storage for die1 max temperature
static uint16_t mock_die1_max_temp = 0;

void __wrap_die_2_die_exch_ib_write_inst_max_die_temp(uint16_t temp)
{
    mock_die1_max_temp = temp;
}

uint16_t __wrap_die_2_die_exch_ib_read_inst_max_die_temp_dC(uint8_t die)
{
    FPFW_UNUSED(die);
    uint16_t temp = mock_type(uint16_t);
    return temp;
}

uint8_t __wrap_die_2_die_exch_get_this_die_id(void)
{
    // only use mock if g_enable_mock_die_id is set in the test.
    // Doing this instead of direct mocking as was introduced in the aging counter tests and shouldn't impact other tests.
    if (g_enable_mock_die_id)
    {
        return mock_type(uint8_t);
    }
    else
    {
        // Default behavior: return PRIMARY_DIE_ID (0), so that other existing tests who calls data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg already doesn't get impacted.
        return 0;
    }
}

void __wrap_in_band_tlm_cmpnt_notify_sec_mcps_prepare_pwr_pkg(void)
{
    function_called();
}

void __wrap_in_band_tlm_cmpnt_notify_scp_tlm_svc_prepare_pwr_pkg(void)
{
    function_called();
}

int __wrap_d2dss_pmu_init(uint8_t d2dss_index, uint8_t event_number, uint32_t event_count, bool enable)
{
    FPFW_UNUSED(d2dss_index);
    FPFW_UNUSED(event_number);
    FPFW_UNUSED(event_count);
    FPFW_UNUSED(enable);

    // Mock register write operation for PMU initialization
    // Following sensor FIFO pattern for register access

    if (d2dss_index < 8 && event_number < 8)
    {
        // Initialize mock counter values with test data
        d2d_mock_counter_values[d2dss_index][event_number][0] = 1000000; // counter_low
        d2d_mock_counter_values[d2dss_index][event_number][1] = 0;       // counter_high
        d2d_pmu_initialized = true;
    }

    return SILIBS_SUCCESS;
}

int __wrap_d2dss_pmu_read(uint8_t d2dss_index, uint8_t event_number, uint32_t* counter_low, uint32_t* counter_high)
{
    assert_non_null(counter_low);
    assert_non_null(counter_high);

    // Mock register read operation returning realistic counter values
    // Following sensor FIFO pattern for memory-mapped register reads

    if (d2dss_index < 8 && d2d_pmu_initialized)
    {
        // Return mock counter data (simulate link activity)
        *counter_low = d2d_mock_counter_values[d2dss_index][event_number][0] + (d2dss_index * 100000);
        *counter_high = d2d_mock_counter_values[d2dss_index][event_number][1];
    }
    else
    {
        // Return zeros for uninitialized or invalid interface
        *counter_low = 0;
        *counter_high = 0;
    }

    return SILIBS_SUCCESS;
}

// D2D PMU mock setup helper - provides exact return value count for D2D function calls
void setup_d2d_pmu_mocks(uint64_t counter_value)
{
    // Setup D2D PMU register mocks with realistic counter values
    // Following sensor FIFO mock pattern for register read/write operations

    // Initialize D2D PMU state
    d2d_pmu_initialized = true;

    // Configure mock counter values for all D2D interfaces
    // Each interface has multiple counters (tx_residency, rx_residency, bw_tx, bw_rx)
    for (uint8_t interface_id = 0; interface_id < NUMBER_OF_D2D_INTERFACES; interface_id++)
    {
        // Set unique counter values per interface and event for testing
        // Base counter_value + offset per interface allows validation of per-interface data
        for (uint8_t event_number = 0; event_number < 8; event_number++)
        {
            d2d_mock_counter_values[interface_id][event_number][0] =
                (uint32_t)(counter_value + (interface_id * 1000) + (event_number * 100));
            d2d_mock_counter_values[interface_id][event_number][1] = (uint32_t)((counter_value >> 32) + interface_id);
        }
    }

    printf("D2D PMU mocks initialized: d2d_pmu_initialized=true, %u interfaces configured\n", NUMBER_OF_D2D_INTERFACES);
}

void reset_d2d_pmu_mocks(void)
{
    // Reset D2D mock state for clean test isolation
    d2d_pmu_initialized = false;
    memset(d2d_mock_counter_values, 0, sizeof(d2d_mock_counter_values));
}

// Mesh telemetry mock functions (hardware interface)
// These follow the same pattern as the unit test mesh mocks
uint32_t __wrap_mesh_get_m0_residency(void)
{
    // Mock mesh M0 residency counter read from hardware
    return mock_type(uint32_t);
}

uint32_t __wrap_mesh_get_m1_residency(void)
{
    // Mock mesh M1 residency counter read from hardware
    return mock_type(uint32_t);
}

uint32_t __wrap_mesh_get_m2_residency(void)
{
    // Mock mesh M2 residency counter read from hardware
    return mock_type(uint32_t);
}

uint32_t __wrap_mesh_get_m1_entry_count(void)
{
    // Mock mesh M1 entry counter read from hardware
    return mock_type(uint32_t);
}

uint32_t __wrap_mesh_get_m2_entry_count(void)
{
    // Mock mesh M2 entry counter read from hardware
    return mock_type(uint32_t);
}

uint32_t __wrap_mesh_get_telemetry_delivered_perf_count(void)
{
    // Mock mesh delivered performance counter read from hardware
    return mock_type(uint32_t);
}

void __wrap_mesh_clock_telemetry(bool enable, uint32_t interval_count)
{
    // Mock mesh telemetry clock configuration (hardware register write)
    check_expected(enable);
    check_expected(interval_count);
    function_called();
}
