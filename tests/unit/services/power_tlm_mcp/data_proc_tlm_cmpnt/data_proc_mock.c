//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file data_proc_mock.c
 * Provide mock functions for icc mhu transport driver tests
 */

/*------------- Includes -----------------*/
#include "data_proc_mock.h"

#include "telemetry_ut.h"

#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <corebits.h>
#include <data_sampling_i.h>
#include <kng_soc_constants.h>
#include <power_telemetry_common.h>
#include <semaphore_lib.h>
#include <sensor_fifo_service.h> // for QUADWORD_SIZE, sensor_ram_...
#include <stdint.h>              // for uint32_t, uint64_t, int32_t
#include <string.h>
#include <tlm_fuses.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
cstate_instr_timestamp_t test_cstate_buf[(128 * 1024) / sizeof(cstate_instr_timestamp_t)];
/*------------- Functions ----------------*/
void setup_cstate_tfa_mock_buffer()
{

    memset(test_cstate_buf, 0, sizeof(test_cstate_buf));
    cstate_tfa_timestamp_base = test_cstate_buf;
}
void __wrap_sensor_fifo_svc_is_empty(bool (*is_empty)[SENSOR_FIFO_MAX_ID])
{
    bool(*mock_data)[SENSOR_FIFO_MAX_ID] = mock_ptr_type(bool(*)[SENSOR_FIFO_MAX_ID]);
    if (mock_data != NULL && is_empty != NULL)
    {
        memcpy(*is_empty, *mock_data, sizeof(bool) * SENSOR_FIFO_MAX_ID);
    }
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_tile_temperature(tile_temp_t* temperature_data, uint16_t* tile_index)
{
    sensor_ram_poll_status_t status = *(sensor_ram_poll_status_t*)mock();

    if (status.curr_data_is_valid)
    {
        tile_temp_t* mock_data = mock_ptr_type(tile_temp_t*);
        assert_non_null(mock_data);
        assert_non_null(temperature_data);
        memcpy(temperature_data, mock_data, sizeof(tile_temp_t));
        if (tile_index != NULL)
        {
            *tile_index = mock_type(uint16_t);
        }
    }

    return status;
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_tile_voltage(tile_voltage_t* voltage_data, uint16_t* tile_index)
{

    sensor_ram_poll_status_t status = *(sensor_ram_poll_status_t*)mock();

    if (status.curr_data_is_valid)
    {
        tile_voltage_t* mock_data = mock_ptr_type(tile_voltage_t*);
        assert_non_null(mock_data);
        memcpy(voltage_data, mock_data, sizeof(tile_voltage_t));
        *tile_index = mock_type(uint16_t);
    }
    return status;
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_core_current(core_current_t* current_data, uint16_t* core_index)
{
    sensor_ram_poll_status_t status = *(sensor_ram_poll_status_t*)mock();
    if (status.curr_data_is_valid)
    {
        core_current_t* mock_data = mock_ptr_type(core_current_t*);
        assert_non_null(mock_data);
        assert_non_null(current_data);
        memcpy(current_data, mock_data, sizeof(core_current_t));
        *core_index = mock_type(uint16_t);
    }
    return status;
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_core_pstate(pstate_telem_t* state_data)
{
    sensor_ram_poll_status_t status = *(sensor_ram_poll_status_t*)mock();
    if (status.curr_data_is_valid)
    {
        pstate_telem_t* mock_data = mock_ptr_type(pstate_telem_t*);
        assert_non_null(mock_data);
        assert_non_null(state_data);
        memcpy(state_data, mock_data, sizeof(pstate_telem_t));
    }
    return status;
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_vr_temperature(vr_temp_t* vr_temperature)
{
    sensor_ram_poll_status_t status = *(sensor_ram_poll_status_t*)mock();

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
    sensor_ram_poll_status_t status = *(sensor_ram_poll_status_t*)mock();

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
    sensor_ram_poll_status_t status = *(sensor_ram_poll_status_t*)mock();
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
    sensor_ram_poll_status_t status = *(sensor_ram_poll_status_t*)mock();

    if (status.curr_data_is_valid)
    {
        sensor_ram_dimm_info_t* mock_data = mock_ptr_type(sensor_ram_dimm_info_t*);
        assert_non_null(mock_data);
        assert_non_null(dimm_info);
        memcpy(dimm_info, mock_data, sizeof(sensor_ram_dimm_info_t));
    }

    return status;
}

fpfw_status_t __wrap_tlm_fuses_get_dts_coeff_tile(dts_tlm_coeff_t* dts_coeff, uint32_t count)
{
    check_expected(count);
    check_expected_ptr(dts_coeff);
    return mock_type(int32_t);
}

uint64_t __wrap_exec_tlm_cmpnt_get_timestamp_microseconds(void)
{
    return mock_type(uint64_t);
}

uint32_t __wrap_gtimer_prodfw_get_frequency(void)
{
    return mock_type(uint32_t);
}

bool __wrap_in_band_tlm_cmpnt_is_inst_record_enabled(uint32_t inst_record_id)
{
    FPFW_UNUSED(inst_record_id);
    return mock_type(bool);
}

void __wrap_in_band_tlm_cmpnt_notify_sec_mcps_prepare_pwr_pkg(void)
{
    function_called();
}

void __wrap_in_band_tlm_cmpnt_notify_scp_tlm_svc_prepare_pwr_pkg(void)
{
    function_called();
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

void __wrap_pwr_tlm_core_exch_mcp_read_droop_counts(uint64_t (*droop_count_array)[NUM_AP_CORES_PER_DIE])
{
    assert_non_null(droop_count_array);

    // Get the mock data pointer passed in the test
    uint64_t* mock_data = mock_ptr_type(uint64_t*);
    assert_non_null(mock_data);

    memcpy(*droop_count_array, mock_data, sizeof(uint64_t) * NUM_AP_CORES_PER_DIE);
}

uint8_t __wrap_pwr_tlm_core_exch_mcp_read_vmin(uint16_t* vmin_array)
{
    assert_non_null(vmin_array);

    // Get the mock data pointer passed in the test
    uint16_t* mock_data = mock_ptr_type(uint16_t*);
    assert_non_null(mock_data);

    memcpy(vmin_array, mock_data, sizeof(uint16_t) * NUM_AP_CORES_PER_DIE);

    // Return mock sequence number
    return mock_type(uint8_t);
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

// Mock implementations for mesh telemetry hardware APIs
uint32_t __wrap_mesh_get_m1_entry_count(void)
{
    return mock_type(uint32_t);
}

uint32_t __wrap_mesh_get_m2_entry_count(void)
{
    return mock_type(uint32_t);
}

uint32_t __wrap_mesh_get_m0_residency(void)
{
    return mock_type(uint32_t);
}

uint32_t __wrap_mesh_get_m1_residency(void)
{
    return mock_type(uint32_t);
}

uint32_t __wrap_mesh_get_m2_residency(void)
{
    return mock_type(uint32_t);
}

uint32_t __wrap_mesh_get_telemetry_delivered_perf_count(void)
{
    return mock_type(uint32_t);
}

void __wrap_mesh_clock_telemetry(bool enable, uint32_t interval_count)
{
    check_expected(enable);
    check_expected(interval_count);
    function_called();
}

// Mock implementations for D2DSS PMU APIs
int __wrap_d2dss_pmu_init(uint8_t d2dss_index, uint8_t event_number, uint32_t event_count, bool enable)
{
    check_expected(d2dss_index);
    check_expected(event_number);
    check_expected(event_count);
    check_expected(enable);
    return mock_type(int);
}

int __wrap_d2dss_pmu_read(uint8_t d2dss_index, uint8_t event_number, uintptr_t counter_low, uintptr_t counter_high)
{
    check_expected(d2dss_index);
    check_expected(event_number);

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

bool __wrap_in_band_tlm_cmpnt_is_power_record_enabled(pwr_telemetry_element_id_t element_id)
{
    // check_expected_ptr(&element_id);
    FPFW_UNUSED(element_id);
    return mock_type(bool);
}

// Mock functions for power telemetry load line loss
uint32_t __wrap_power_telemetry_loadline_loss_die0(power_telemetry_rail_id_die0_t rail_id, uint16_t current_cA)
{
    check_expected(rail_id);
    check_expected(current_cA);
    return mock_type(uint32_t);
}

uint32_t __wrap_power_telemetry_loadline_loss_die1(power_telemetry_rail_id_die1_t rail_id, uint16_t current_cA)
{
    check_expected(rail_id);
    check_expected(current_cA);
    return mock_type(uint32_t);
}
