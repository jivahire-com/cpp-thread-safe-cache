//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_out_of_band_cmpnt.cpp
 * Test out of band telemetry component
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:telemetry

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {

#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <fpfw_status.h>
#include <out_of_band_tlm_cmpnt.h>
#include <out_of_band_tlm_cmpnt_i.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t
#include <tx_api.h>
}
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
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

TEST_FUNCTION(test_out_of_band_tlm_cmpnt_init, test_setup, test_teardown)
{
    expect_any_always(__wrap__txe_event_flags_create, group_ptr);
    expect_any_always(__wrap__txe_event_flags_create, name_ptr);
    expect_any_always(__wrap__txe_event_flags_create, event_control_block_size);
    will_return(__wrap__txe_event_flags_create, TX_SUCCESS);
    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    for (uint16_t index = 0; index < NUM_PWR_TLM_PLDM_SENSORS; index++)
    {
        will_return(__wrap_fpfw_pldm_service_register_numeric_sensor, FPFW_STATUS_SUCCESS);
        expect_function_calls(__wrap_FpFwAssertWithArgs, 1);
    }

    out_of_band_tlm_cmpnt_init(0);

    out_of_band_tlm_cmpnt_init(1);
}

TEST_FUNCTION(test_on_pwr_tlm_numeric_sensor_get_ext_entry, test_setup, test_teardown)
{
    active_sensor = nullptr;
    active_handler = nullptr;

    will_return(__wrap_exec_tlm_cmpnt_is_oob_data_valid, true);

    expect_function_call(__wrap_exec_tlm_cmpnt_notify_new_out_of_band_pldm_request);
    expect_any(__wrap__txe_event_flags_get, group_ptr);
    expect_value(__wrap__txe_event_flags_get, requested_flags, RELEASE_PLDM_SERVICE);
    expect_value(__wrap__txe_event_flags_get, get_option, TX_OR_CLEAR);
    expect_any(__wrap__txe_event_flags_get, actual_flags_ptr);
    expect_value(__wrap__txe_event_flags_get, wait_option, TX_WAIT_FOREVER);

    will_return(__wrap__txe_event_flags_get, TX_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    on_pwr_tlm_numeric_sensor_get_ext_entry(
        &pwr_tlm_numeric_sensor_ctxts[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_SOC_TMP_MAX_NUM_SENS)],
        (void*)pwr_tlm_oob_get_max_soc_temp);
}

TEST_FUNCTION(test_on_pwr_tlm_numeric_sensor_get_ext_entry_negative, test_setup, test_teardown)
{
    will_return(__wrap_exec_tlm_cmpnt_is_oob_data_valid, false);
    expect_function_call(__wrap_fpfw_pldm_service_numeric_sensor_not_ready);
    on_pwr_tlm_numeric_sensor_get_ext_entry(
        &pwr_tlm_numeric_sensor_ctxts[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_SOC_TMP_MAX_NUM_SENS)],
        (void*)pwr_tlm_oob_get_max_soc_temp);

    will_return(__wrap_exec_tlm_cmpnt_is_oob_data_valid, true);
    active_sensor = (pldm_numeric_sensor_context_t*)0x1234;
    active_handler = nullptr;
    on_pwr_tlm_numeric_sensor_get_ext_entry(
        &pwr_tlm_numeric_sensor_ctxts[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_SOC_TMP_MAX_NUM_SENS)],
        (void*)pwr_tlm_oob_get_max_soc_temp);

    will_return(__wrap_exec_tlm_cmpnt_is_oob_data_valid, true);
    active_sensor = nullptr;
    active_handler = (pwr_tlm_numeric_sensor_get_handler_t)0x1234;
    on_pwr_tlm_numeric_sensor_get_ext_entry(
        &pwr_tlm_numeric_sensor_ctxts[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_SOC_TMP_MAX_NUM_SENS)],
        (void*)pwr_tlm_oob_get_max_soc_temp);
}

TEST_FUNCTION(test_out_of_band_tlm_cmpnt_handle_new_pldm_requests, test_setup, test_teardown)
{
    active_sensor =
        &pwr_tlm_numeric_sensor_ctxts[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_SOC_TMP_MAX_NUM_SENS)];
    active_handler = pwr_tlm_oob_get_max_soc_temp;
    active_sensor->sensor_state.config.sensor_id = PLDM_SENSOR_ID_POWER_TLM_SOC_TMP_MAX_NUM_SENS; // fake sensor init

    will_return(__wrap_data_proc_tlm_cmpnt_get_oob_crit_max_soc_temp_dC, 1000);

    expect_value(__wrap_fpfw_pldm_service_numeric_sensor_set,
                 p_sensor,
                 &pwr_tlm_numeric_sensor_ctxts[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_SOC_TMP_MAX_NUM_SENS)]);
    expect_value(__wrap_fpfw_pldm_service_numeric_sensor_set, value.numeric.u16, 1000);
    will_return(__wrap_fpfw_pldm_service_numeric_sensor_set, FPFW_STATUS_SUCCESS);
    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    expect_any_always(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, RELEASE_PLDM_SERVICE);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);
    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);
    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    out_of_band_tlm_cmpnt_handle_new_pldm_requests();
}

TEST_FUNCTION(test_out_of_band_tlm_cmpnt_handle_new_pldm_requests_negative, test_setup, test_teardown)
{
    active_sensor = nullptr;
    active_handler = pwr_tlm_oob_get_max_soc_temp;

    expect_any(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, RELEASE_PLDM_SERVICE);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);
    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);
    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    out_of_band_tlm_cmpnt_handle_new_pldm_requests();

    active_sensor =
        &pwr_tlm_numeric_sensor_ctxts[PWR_TLM_PDR_SENSOR_INDEX(PLDM_SENSOR_ID_POWER_TLM_SOC_TMP_MAX_NUM_SENS)];
    active_handler = nullptr;

    expect_any(__wrap__txe_event_flags_set, group_ptr);
    expect_value(__wrap__txe_event_flags_set, flags_to_set, RELEASE_PLDM_SERVICE);
    expect_value(__wrap__txe_event_flags_set, set_option, TX_OR);
    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);
    expect_function_calls(__wrap_FpFwAssertWithArgs, 1);

    out_of_band_tlm_cmpnt_handle_new_pldm_requests();
}

TEST_FUNCTION(test_pwr_tlm_oob_get_max_soc_temp, test_setup, test_teardown)
{
    fpfw_pldm_composite_value_t sensor_value = {{{0}}};
    will_return(__wrap_data_proc_tlm_cmpnt_get_oob_crit_max_soc_temp_dC, 800);
    pwr_tlm_oob_get_max_soc_temp(PLDM_SENSOR_ID_POWER_TLM_SOC_TMP_MAX_NUM_SENS, &sensor_value);
    assert_int_equal(sensor_value.numeric.u16, 800);
}

TEST_FUNCTION(test_pwr_tlm_oob_get_max_soc_temp_negative, test_setup, test_teardown)
{
    fpfw_pldm_composite_value_t sensor_value = {{{0}}};
    pwr_tlm_oob_get_max_soc_temp(PLDM_SENSOR_ID_POWER_TLM_LAST, &sensor_value);
}

TEST_FUNCTION(test_pwr_tlm_oob_get_max_dimm_temp, test_setup, test_teardown)
{
    fpfw_pldm_composite_value_t sensor_value = {{{0}}};
    will_return(__wrap_data_proc_tlm_cmpnt_get_oob_crit_max_dimm_temp_dC, 1000);
    pwr_tlm_oob_get_max_dimm_temp(PLDM_SENSOR_ID_POWER_TLM_DIMM_TMP_MAX_NUM_SENS, &sensor_value);
    assert_int_equal(sensor_value.numeric.u16, 1000);
}

TEST_FUNCTION(test_pwr_tlm_oob_get_max_dimm_temp_negative, test_setup, test_teardown)
{
    fpfw_pldm_composite_value_t sensor_value = {{{0}}};
    pwr_tlm_oob_get_max_dimm_temp(PLDM_SENSOR_ID_POWER_TLM_LAST, &sensor_value);
}

TEST_FUNCTION(test_pwr_tlm_oob_get_soc_pwr, test_setup, test_teardown)
{
    fpfw_pldm_composite_value_t sensor_value = {{{0}}};
    will_return(__wrap_data_proc_tlm_cmpnt_get_oob_soc_pwr_mW, 0xFF000000);
    pwr_tlm_oob_get_soc_pwr(PLDM_SENSOR_ID_POWER_TLM_SOC_PWR_NUM_SENS, &sensor_value);
    assert_int_equal(sensor_value.numeric.u32, 0xFF000000);
}

TEST_FUNCTION(test_pwr_tlm_oob_get_soc_pwr_negative, test_setup, test_teardown)
{
    fpfw_pldm_composite_value_t sensor_value = {{{0}}};
    pwr_tlm_oob_get_soc_pwr(PLDM_SENSOR_ID_POWER_TLM_LAST, &sensor_value);
}

TEST_FUNCTION(test_pwr_tlm_oob_get_dimm_total_pwr, test_setup, test_teardown)
{
    fpfw_pldm_composite_value_t sensor_value = {{{0}}};
    will_return(__wrap_data_proc_tlm_cmpnt_get_oob_dimm_total_pwr_mW, 0xFF000000);
    pwr_tlm_oob_get_dimm_total_pwr(PLDM_SENSOR_ID_POWER_TLM_DIMM_TOTAL_PWR_NUM_SENS, &sensor_value);
    assert_int_equal(sensor_value.numeric.u32, 0xFF000000);
}

TEST_FUNCTION(test_pwr_tlm_oob_get_dimm_total_pwr_negative, test_setup, test_teardown)
{
    fpfw_pldm_composite_value_t sensor_value = {{{0}}};
    pwr_tlm_oob_get_dimm_total_pwr(PLDM_SENSOR_ID_POWER_TLM_LAST, &sensor_value);
}

TEST_FUNCTION(test_pwr_tlm_oob_get_max_vr_temp, test_setup, test_teardown)
{
    fpfw_pldm_composite_value_t sensor_value = {{{0}}};
    will_return(__wrap_data_proc_tlm_cmpnt_get_oob_crit_max_vr_temp_dC, 1000);
    pwr_tlm_oob_get_max_vr_temp(PLDM_SENSOR_ID_POWER_TLM_VR_TMP_MAX_NUM_SENS, &sensor_value);
    assert_int_equal(sensor_value.numeric.u16, 1000);
}

TEST_FUNCTION(test_pwr_tlm_oob_get_max_vr_temp_negative, test_setup, test_teardown)
{
    fpfw_pldm_composite_value_t sensor_value = {{{0}}};
    pwr_tlm_oob_get_max_vr_temp(PLDM_SENSOR_ID_POWER_TLM_LAST, &sensor_value);
}

TEST_FUNCTION(test_pwr_tlm_oob_get_soc_avg_freq, test_setup, test_teardown)
{
    fpfw_pldm_composite_value_t sensor_value = {{{0}}};
    will_return(__wrap_data_proc_tlm_cmpnt_get_oob_soc_avg_freq_MHz, 3600);
    pwr_tlm_oob_get_soc_avg_freq(PLDM_SENSOR_ID_POWER_TLM_SOC_AVG_FREQ_NUM_SENS, &sensor_value);
    assert_int_equal(sensor_value.numeric.u16, 3600);
}

TEST_FUNCTION(test_pwr_tlm_oob_get_soc_avg_freq_negative, test_setup, test_teardown)
{
    fpfw_pldm_composite_value_t sensor_value = {{{0}}};
    pwr_tlm_oob_get_soc_avg_freq(PLDM_SENSOR_ID_POWER_TLM_LAST, &sensor_value);
}

TEST_FUNCTION(test_out_of_band_tlm_cmpnt_print_sensors, test_setup, test_teardown)
{
    will_return(__wrap_data_proc_tlm_cmpnt_get_oob_crit_max_soc_temp_dC, 800);
    will_return(__wrap_data_proc_tlm_cmpnt_get_oob_soc_pwr_mW, 0xFF000000);
    will_return(__wrap_data_proc_tlm_cmpnt_get_oob_crit_max_dimm_temp_dC, 1000);
    will_return(__wrap_data_proc_tlm_cmpnt_get_oob_dimm_total_pwr_mW, 0xFF000000);
    will_return(__wrap_data_proc_tlm_cmpnt_get_oob_soc_avg_freq_MHz, 3600);
    out_of_band_tlm_cmpnt_print_sensors();
}
