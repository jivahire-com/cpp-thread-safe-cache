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
#include <in_band_tlm_cmpnt_i.h>
#include <data_proc_tlm_cmpnt.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
ULONG  __wrap__tx_time_get(VOID)
{
    static ULONG time = 1;
    return time++;
}

void data_proc_tlm_cmpnt_get_pwr_core_pstate_data(uint16_t core_id, pwr_core_element_pstate_t (*pstate_array)[NUMBER_OF_PSTATES])
{
    FPFW_UNUSED(core_id);
    memset( (*pstate_array), 0xFF, sizeof(pwr_core_element_pstate_t) * NUMBER_OF_PSTATES);

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_core_cstate_data(uint16_t core_id, pwr_core_element_cstate_t (*cstate_array)[NUMBER_OF_CSTATES])
{
    FPFW_UNUSED(core_id);
    memset( (*cstate_array), 0xFF, sizeof(pwr_core_element_cstate_t) * NUMBER_OF_CSTATES);

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_core_throttle_data(uint16_t core_id,
                                                    pwr_core_element_throttle_t (*throttle_array)[NUMBER_OF_THROTTLE_TYPES])
{
    FPFW_UNUSED(core_id);
    memset( (*throttle_array), 0xFF, sizeof(pwr_core_element_throttle_t) * NUMBER_OF_THROTTLE_TYPES);

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_core_rack_priority_data(uint16_t core_id,
                                                         pwr_core_element_rack_priorities_t (*rack_priority_array)[NUMBER_OF_RACK_PRIORITIES])
{
    FPFW_UNUSED(core_id);
    memset( (*rack_priority_array), 0xFF, sizeof(pwr_core_element_rack_priorities_t) * NUMBER_OF_RACK_PRIORITIES);

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_core_voltage_data(uint16_t core_id, p_pwr_core_element_voltage_t voltage_data)
{
    FPFW_UNUSED(core_id);
    memset( voltage_data, 0xFF, sizeof(pwr_core_element_voltage_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_core_current_data(uint16_t core_id, p_pwr_core_element_current_t current_data)
{
    FPFW_UNUSED(core_id);
    memset( current_data, 0xFF, sizeof(pwr_core_element_current_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_core_temperature_data(uint16_t core_id, p_pwr_core_element_temperature_t temperature_data)
{
    FPFW_UNUSED(core_id);
    memset( temperature_data, 0xFF, sizeof(pwr_core_element_temperature_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_core_histogram_data(
    uint16_t core_id,
    pwr_core_element_histogram_t (*histogram_array)[NUMBER_OF_HS_VOLTAGE_SCALES][NUMBER_OF_HS_TEMP_SCALES])
{
    FPFW_UNUSED(core_id);
    memset( (*histogram_array), 0xFF, sizeof(pwr_core_element_histogram_t) * NUMBER_OF_HS_VOLTAGE_SCALES * NUMBER_OF_HS_TEMP_SCALES);

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_soc_pc3_data(p_pwr_soc_element_pc3_t soc_pc3_data)
{
    memset( soc_pc3_data, 0xFF, sizeof(pwr_soc_element_pc3_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_soc_vr_rail_data(uint16_t rail_id, p_pwr_soc_element_vr_rail_t rail_data)
{
    FPFW_UNUSED(rail_id);
    memset( rail_data, 0xFF, sizeof(pwr_soc_element_vr_rail_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_soc_hnf_data(uint16_t hnf_channel, p_pwr_soc_element_hnf_t hnf_data)
{
    FPFW_UNUSED(hnf_channel);
    memset( hnf_data, 0xFF, sizeof(pwr_soc_element_hnf_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_soc_dimm_data(uint16_t dimm_channel, p_pwr_soc_element_dimm_t dimm_data)
{
    FPFW_UNUSED(dimm_channel);
    memset( dimm_data, 0xFF, sizeof(pwr_soc_element_dimm_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_soc_snsr_temp_data(uint16_t sensor_id, p_pwr_soc_element_sensor_temp_t sensor_temp_data)
{
    FPFW_UNUSED(sensor_id);
    memset( sensor_temp_data, 0xFF, sizeof(pwr_soc_element_sensor_temp_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_mpam_pstate_data(uint16_t mpam_id,
                                                  pwr_element_mpam_pstate_t (*mpam_pstate_array)[NUMBER_OF_PSTATES])
{
    FPFW_UNUSED(mpam_id);
    memset( (*mpam_pstate_array), 0xFF, sizeof(pwr_element_mpam_pstate_t) * NUMBER_OF_PSTATES);

    function_called();
}

void data_proc_tlm_cmpnt_get_pwr_soc_mpam_throttle_data(uint16_t mpam_id, p_pwr_element_mpam_throttle_t mpam_throttle_data)
{
    FPFW_UNUSED(mpam_id);
    memset(mpam_throttle_data, 0xFF, sizeof(pwr_element_mpam_throttle_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_inst_soc_core_summary_data(uint16_t core_id, p_inst_core_element_summary_t core_summary_data)
{
    FPFW_UNUSED(core_id);
    memset( core_summary_data, 0xFF, sizeof(inst_core_element_summary_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_inst_soc_rail_data(uint16_t rail_id, p_inst_soc_element_rail_t rail_data)
{
    FPFW_UNUSED(rail_id);
    memset( rail_data, 0xFF, sizeof(inst_soc_element_rail_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_inst_soc_dimm_runtime_data(uint16_t dimm_module, p_inst_soc_element_dimm_runtime_t dimm_data)
{
    FPFW_UNUSED(dimm_module);
    memset( dimm_data, 0xFF, sizeof(inst_soc_element_dimm_runtime_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_inst_soc_dimm_config_data(p_inst_soc_element_dimm_config_t dimm_cfg)
{
    memset(dimm_cfg, 0xFF, sizeof(inst_soc_element_dimm_config_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_inst_soc_snsr_temp_data(uint16_t sensor_id, p_inst_soc_element_sensor_temp_t sensor_temp_data)
{
    FPFW_UNUSED(sensor_id);
    memset(sensor_temp_data, 0xFF, sizeof(inst_soc_element_sensor_temp_t));

    function_called();
}

void data_proc_tlm_cmpnt_get_inst_core_amu_data(uint16_t core_id, p_inst_core_element_amu_counters_t amu_data)
{
    FPFW_UNUSED(core_id);
    memset(amu_data, 0xFF, sizeof(inst_core_element_amu_counters_t) );

    function_called();
}