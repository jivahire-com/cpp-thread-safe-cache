//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file telemetry_functional_mocks.c
 * Provide mock functions for telemetry functional tests
 */

/*------------- Includes -----------------*/
#include "telemetry_functional.h"

#include <FpFwCMocka.h>
#include <FpFwUtils.h>
#include <data_proc_tlm_cmpnt.h>
#include <semaphore_lib.h>
#include <sensor_fifo_service.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int g_enable_mock_pstate = 0;
bool test_snsr_fifo_is_empty[SENSOR_FIFO_MAX_ID] = {0};

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
    time_t0 += 1000; // Increment by 1000μs or 1ms  each time
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

// Mock event trace metadata symbols
#ifdef __cplusplus
extern "C" {
#endif

// as per the original declarations in dcs_manifest.c
uint8_t _ProviderMetadata_et_msdata_start;
uint8_t _ProviderMetadata_et_msdata_end;
uint8_t _EventMetadata_et_msdata_start;
uint8_t _EventMetadata_et_msdata_end;

#ifdef __cplusplus
}
#endif