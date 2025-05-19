//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file in_band_mocks.c
 * Mock functions for in-band telemetry component
 */

/*------------- Includes -----------------*/
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <data_proc_tlm_cmpnt.h>
#include <in_band_tlm_cmpnt_i.h>
#include <message_transfer_service.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
uint8_t _ProviderMetadata_et_msdata_start; // Pointer to the start of the .ProviderMetadata section
uint8_t _ProviderMetadata_et_msdata_end;   // Pointer to the end   of the .ProviderMetadata section
uint8_t _EventMetadata_et_msdata_start;    // Pointer to the start of the .EventMetadata section
uint8_t _EventMetadata_et_msdata_end;      // Pointer to the end   of the .EventMetadata memory section

bool g_die_id_mocked = false;

/*------------- Functions ----------------*/
ULONG __wrap__tx_time_get(VOID)
{
    static ULONG time = 1;
    return time++;
}

bool __wrap_mts_is_primary_instance(void)
{
    return mock_type(bool);
}

void __wrap_mts_client_forward_trp_msg(p_trp_msg_t trp_msg, trp_broadcast_t broadcast_option)
{
    FPFW_UNUSED(trp_msg);
    FPFW_UNUSED(broadcast_option);
    function_called();
}

void __wrap_mts_client_send_trp_response(p_trp_msg_t trp_msg)
{
    FPFW_UNUSED(trp_msg);
    function_called();
}

void __wrap_mts_client_send_new_trp_msg(p_trp_msg_t trp_msg)
{
    FPFW_UNUSED(trp_msg);
    function_called();
}

void __wrap_mts_client_send_dcp_notification(mts_client_id_t client_id, dcp_notification_type_t notification)
{
    FPFW_UNUSED(client_id);
    FPFW_UNUSED(notification);
    function_called();
}

uint8_t __wrap_mts_get_this_die_id(void)
{
    if (g_die_id_mocked)
    {
        return mock_type(uint8_t);
    }
    return 0;
}

mts_platform_core_id_t __wrap_mts_get_this_core_id(void)
{
    return 2;
}

void data_proc_tlm_cmpnt_get_pwr_core_pstate_data(uint16_t core_id, pwr_core_element_pstate_t (*pstate_array)[NUMBER_OF_PSTATES])
{
    FPFW_UNUSED(core_id);
    memset((*pstate_array), 0xFF, sizeof(pwr_core_element_pstate_t) * NUMBER_OF_PSTATES);

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_core_cstate_data(uint16_t core_id, pwr_core_element_cstate_t (*cstate_array)[NUMBER_OF_CSTATES])
{
    FPFW_UNUSED(core_id);
    memset((*cstate_array), 0xFF, sizeof(pwr_core_element_cstate_t) * NUMBER_OF_CSTATES);

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_core_throttle_data(uint16_t core_id,
                                                    pwr_core_element_throttle_t (*throttle_array)[NUMBER_OF_THROTTLE_TYPES])
{
    FPFW_UNUSED(core_id);
    memset((*throttle_array), 0xFF, sizeof(pwr_core_element_throttle_t) * NUMBER_OF_THROTTLE_TYPES);

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_core_rack_priority_data(uint16_t core_id,
                                                         pwr_core_element_rack_priorities_t (*rack_priority_array)[NUMBER_OF_RACK_PRIORITIES])
{
    FPFW_UNUSED(core_id);
    memset((*rack_priority_array), 0xFF, sizeof(pwr_core_element_rack_priorities_t) * NUMBER_OF_RACK_PRIORITIES);

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_core_voltage_data(uint16_t core_id, p_pwr_core_element_voltage_t voltage_data)
{
    FPFW_UNUSED(core_id);
    memset(voltage_data, 0xFF, sizeof(pwr_core_element_voltage_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_core_current_data(uint16_t core_id, p_pwr_core_element_current_t current_data)
{
    FPFW_UNUSED(core_id);
    memset(current_data, 0xFF, sizeof(pwr_core_element_current_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_core_temperature_data(uint16_t core_id, p_pwr_core_element_temperature_t temperature_data)
{
    FPFW_UNUSED(core_id);
    memset(temperature_data, 0xFF, sizeof(pwr_core_element_temperature_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_core_power_data(uint16_t core_id, p_pwr_core_element_power_t power_data)
{
    FPFW_UNUSED(core_id);
    memset(power_data, 0xFF, sizeof(pwr_core_element_power_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_core_histogram_data(
    uint16_t core_id,
    pwr_core_element_histogram_t (*histogram_array)[NUMBER_OF_HS_VOLTAGE_SCALES][NUMBER_OF_HS_TEMP_SCALES])
{
    FPFW_UNUSED(core_id);
    memset((*histogram_array), 0xFF, sizeof(pwr_core_element_histogram_t) * NUMBER_OF_HS_VOLTAGE_SCALES * NUMBER_OF_HS_TEMP_SCALES);

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_soc_pkg_mon_data(p_pwr_soc_element_pkg_monitor_t soc_pkg_mon_data)
{
    memset(soc_pkg_mon_data, 0xFF, sizeof(pwr_soc_element_pkg_monitor_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_soc_vr_rail_data(uint16_t rail_id, p_pwr_soc_element_vr_rail_t rail_data)
{
    FPFW_UNUSED(rail_id);
    memset(rail_data, 0xFF, sizeof(pwr_soc_element_vr_rail_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_soc_hnf_data(uint16_t hnf_channel, p_pwr_soc_element_hnf_t hnf_data)
{
    FPFW_UNUSED(hnf_channel);
    memset(hnf_data, 0xFF, sizeof(pwr_soc_element_hnf_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_soc_temp_dimm_data(uint16_t dimm_channel, p_pwr_soc_element_dimm_temp_t dimm_data)
{
    FPFW_UNUSED(dimm_channel);
    memset(dimm_data, 0xFF, sizeof(pwr_soc_element_dimm_temp_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_soc_snsr_temp_data(uint16_t sensor_id, p_pwr_soc_element_sensor_temp_t sensor_temp_data)
{
    FPFW_UNUSED(sensor_id);
    memset(sensor_temp_data, 0xFF, sizeof(pwr_soc_element_sensor_temp_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_mpam_pstate_data(uint16_t mpam_id,
                                                  pwr_soc_element_mpam_pstate_t (*mpam_pstate_array)[NUMBER_OF_PSTATES])
{
    FPFW_UNUSED(mpam_id);
    memset((*mpam_pstate_array), 0xFF, sizeof(pwr_soc_element_mpam_pstate_t) * NUMBER_OF_PSTATES);

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_soc_mpam_throttle_data(uint16_t mpam_id, p_pwr_soc_element_mpam_throttle_t mpam_throttle_data)
{
    FPFW_UNUSED(mpam_id);
    memset(mpam_throttle_data, 0xFF, sizeof(pwr_soc_element_mpam_throttle_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_inst_soc_core_summary_data(uint16_t core_id, p_inst_core_element_summary_t core_summary_data)
{
    FPFW_UNUSED(core_id);
    memset(core_summary_data, 0xFF, sizeof(inst_core_element_summary_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_inst_soc_rail_data(uint16_t rail_id, p_inst_soc_element_rail_t rail_data)
{
    FPFW_UNUSED(rail_id);
    memset(rail_data, 0xFF, sizeof(inst_soc_element_rail_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_inst_soc_dimm_runtime_data(uint16_t dimm_module, p_inst_soc_element_dimm_runtime_t dimm_data)
{
    FPFW_UNUSED(dimm_module);
    memset(dimm_data, 0xFF, sizeof(inst_soc_element_dimm_runtime_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_inst_soc_snsr_temp_data(uint16_t sensor_id, p_inst_soc_element_die_temp_t sensor_temp_data)
{
    FPFW_UNUSED(sensor_id);
    memset(sensor_temp_data, 0xFF, sizeof(inst_soc_element_die_temp_t));

    function_called();
}

bool exec_tlm_cmpnt_is_telemetry_publishing_enabled(void)
{
    return mock_type(bool);
}

void exec_tlm_cmpnt_change_telemetry_mode(tlm_operating_mode_t new_mode)
{
    FPFW_UNUSED(new_mode);

    function_called();
}

void exec_tlm_cmpnt_notify_new_in_band_mts_message(void)
{
    function_called();
}

void __wrap_data_proc_tlm_cmpnt_pwr_pkg_completed(void)
{
    function_called();
}

void __wrap_data_proc_tlm_cmpnt_24hr_pkg_completed(void)
{
    function_called();
}

uint64_t __wrap_exec_tlm_cmpnt_get_timestamp_microseconds(void)
{
    return 0x48;
}

void __wrap_mts_client_flush_incoming_queue(mts_client_id_t id)
{
    FPFW_UNUSED(id);
    function_called();
}

void __wrap_FpFwAssertWithArgs(int expression, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3)
{
    FPFW_UNUSED(expression);
    FPFW_UNUSED(arg0);
    FPFW_UNUSED(arg1);
    FPFW_UNUSED(arg2);
    FPFW_UNUSED(arg3);

    function_called();
}

UINT __wrap__txe_queue_flush(TX_QUEUE* queue_ptr)
{
    check_expected_ptr(queue_ptr);

    return mock_type(UINT);
}

UINT __wrap__txe_queue_create(TX_QUEUE* queue_ptr, CHAR* name_ptr, UINT message_size, VOID* queue_start, ULONG queue_size, UINT queue_control_block_size)
{
    check_expected_ptr(queue_ptr);
    check_expected_ptr(name_ptr);
    check_expected(message_size);
    check_expected_ptr(queue_start);
    check_expected(queue_size);
    check_expected(queue_control_block_size);
    return mock_type(UINT);
}

UINT __wrap__txe_queue_send(TX_QUEUE* queue_ptr, VOID* source_ptr, ULONG wait_option)
{
    check_expected_ptr(queue_ptr);
    check_expected_ptr(source_ptr);
    check_expected(wait_option);
    return mock_type(UINT);
}

UINT __wrap__txe_queue_receive(TX_QUEUE* queue_ptr, VOID* destination_ptr, ULONG wait_option)
{
    check_expected_ptr(queue_ptr);
    check_expected_ptr(destination_ptr);
    check_expected(wait_option);

    size_t mockSize = mock_type(size_t);
    void* pMockData = mock_ptr_type(void*);

    if (destination_ptr && pMockData)
    {
        memcpy(destination_ptr, pMockData, mockSize); // NOLINT memcpy ok for mock
    }

    return mock_type(UINT);
}

UINT __wrap__txe_block_pool_create(TX_BLOCK_POOL* pool_ptr, CHAR* name_ptr, ULONG block_size, VOID* pool_start, ULONG pool_size, UINT pool_control_block_size)
{
    FPFW_UNUSED(pool_ptr);
    FPFW_UNUSED(name_ptr);
    FPFW_UNUSED(block_size);
    FPFW_UNUSED(pool_start);
    FPFW_UNUSED(pool_size);
    FPFW_UNUSED(pool_control_block_size);

    function_called();

    return mock_type(UINT);
}

UINT __wrap__txe_block_release(VOID* block_ptr)
{
    FPFW_UNUSED(block_ptr);
    function_called();

    return mock_type(UINT);
}

fpfw_status_t __wrap_mts_client_register(mts_client_id_t id, p_mts_client_t client)
{
    FPFW_UNUSED(id);
    FPFW_UNUSED(client);

    return FPFW_STATUS_SUCCESS;
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_tile_temperature(tile_temp_t* temperature_data, uint16_t* tile_index)
{
    FPFW_UNUSED(temperature_data);
    FPFW_UNUSED(tile_index);
    return *(sensor_ram_poll_status_t*)mock();
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_tile_voltage(tile_voltage_t* voltage_data, uint16_t* tile_index)
{
    FPFW_UNUSED(voltage_data);
    FPFW_UNUSED(tile_index);
    return *(sensor_ram_poll_status_t*)mock();
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_core_current(core_current_t* current_data, uint16_t* core_index)
{
    FPFW_UNUSED(current_data);
    FPFW_UNUSED(core_index);
    return *(sensor_ram_poll_status_t*)mock();
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_core_pstate(pstate_telem_t* state_data)
{
    FPFW_UNUSED(state_data);
    return *(sensor_ram_poll_status_t*)mock();
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_vr_temperature(vr_temp_t* vr_temperature)
{
    FPFW_UNUSED(vr_temperature);
    return *(sensor_ram_poll_status_t*)mock();
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_vr_current(vr_current_t* vr_current)
{
    FPFW_UNUSED(vr_current);
    return *(sensor_ram_poll_status_t*)mock();
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_soc_pvt_temperature(soc_pvt_temp_t* pvt_temperature)
{
    FPFW_UNUSED(pvt_temperature);
    return *(sensor_ram_poll_status_t*)mock();
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_soc_pvt_voltage(soc_pvt_voltage_t* pvt_voltage)
{
    FPFW_UNUSED(pvt_voltage);
    return *(sensor_ram_poll_status_t*)mock();
}

sensor_ram_poll_status_t __wrap_sensor_fifo_svc_poll_dimm_info(sensor_ram_dimm_info_t* dimm_info)
{
    FPFW_UNUSED(dimm_info);
    return *(sensor_ram_poll_status_t*)mock();
}

void __wrap_sensor_fifo_svc_is_empty(bool (*is_empty)[SENSOR_FIFO_MAX_ID])
{
    bool* mocked_array = (bool*)mock();

    // Copy the mocked array into the provided pointer
    for (size_t i = 0; i < SENSOR_FIFO_MAX_ID; i++)
    {
        (*is_empty)[i] = mocked_array[i];
    }
}