
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

// Modifing the timestamp function to return larger intervals
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

corebits_t mock_cores_in_die;
corebits_t* __wrap_core_info_get_enable_cores_result()
{
    for (uint32_t i = 0; i < BITTYPE_COUNT; i++)
    {
        mock_cores_in_die.bits[i] = mock_type(uint32_t);
    }
    return (&mock_cores_in_die);
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

int __wrap_d2dss_pmu_read(uint8_t d2dss_index, uint8_t event_number, uintptr_t counter_low, uintptr_t counter_high)
{
    FPFW_UNUSED(d2dss_index);
    FPFW_UNUSED(event_number);

    // Cast uintptr_t to uint32_t* and check they are valid pointers
    uint32_t* counter_low_ptr = (uint32_t*)counter_low;
    uint32_t* counter_high_ptr = (uint32_t*)counter_high;

    assert_non_null(counter_low_ptr);
    assert_non_null(counter_high_ptr);

    // Set the counter values from mock data
    *counter_low_ptr = mock_type(uint32_t);
    *counter_high_ptr = mock_type(uint32_t);

    return mock_type(int);
}