//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file out_of_band_mocks.c
 * This file contains mock implementations for the out-of-band telemetry component.
 */

/*------------- Includes -----------------*/
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <fpfw_pldm_service.h>
#include <fpfw_status.h>
#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
bool exec_tlm_cmpnt_is_oob_data_valid(void);

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

bool __wrap_exec_tlm_cmpnt_is_oob_data_valid(void)
{
    return mock_type(bool);
}

void __wrap_exec_tlm_cmpnt_notify_new_out_of_band_pldm_request(void)
{
    function_called();
}

uint16_t __wrap_data_proc_tlm_cmpnt_get_oob_crit_max_soc_temp_dC(void)
{
    return mock_type(uint16_t);
}

uint16_t __wrap_data_proc_tlm_cmpnt_get_oob_crit_max_dimm_temp_dC(void)
{
    return mock_type(uint16_t);
}

uint32_t __wrap_data_proc_tlm_cmpnt_get_oob_soc_pwr_mW(void)
{
    return mock_type(uint32_t);
}

uint32_t __wrap_data_proc_tlm_cmpnt_get_oob_dimm_total_pwr_mW(void)
{
    return mock_type(uint32_t);
}

uint16_t __wrap_data_proc_tlm_cmpnt_get_oob_crit_max_vr_temp_dC(void)
{
    return mock_type(uint16_t);
}

fpfw_status_t __wrap_fpfw_pldm_service_register_numeric_sensor(pldm_numeric_sensor_context_t* p_sensor,
                                                               pldm_numeric_sensor_config_t* p_config)
{
    FPFW_UNUSED(p_sensor);
    FPFW_UNUSED(p_config);
    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_pldm_service_numeric_sensor_set(pldm_numeric_sensor_context_t* p_sensor,
                                                          fpfw_pldm_composite_value_t value)
{
    check_expected_ptr(p_sensor);
    check_expected(value.numeric.u16);

    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_pldm_service_numeric_sensor_not_ready(pldm_numeric_sensor_context_t* p_sensor)
{
    FPFW_UNUSED(p_sensor);
    function_called();
    return FPFW_STATUS_SUCCESS;
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
