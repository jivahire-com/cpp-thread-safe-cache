//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_loops_control_test.cpp
 * Power service control loop tests
 */

/*------------- Includes -----------------*/
#include "power_test.h" // for POWER_TEST
#include "power_test_loops.h"

#include <cstddef> // for NULL

extern "C" {
#include "power_hw_int_i.h"
#include "power_i.h"           // for power_latest_calcs_t
#include "power_runconfig.h"   // for MIN_PLIMIT, power_service_config_t
#include "power_runconfig_i.h" // for power_runconfig_t

#include <CMockaWrapper.h>       // for CmockaWrapperTest, expect_value, will...
#include <corebits.h>            // for corebits_set_bit
#include <pid_resource.h>        // for pid_config_t
#include <power_loops_i.h>       // for _power_ctrl_loop_state_t, _power_ctrl...
#include <pvt.h>                 // for pvt_irq_soc_dts_data_t, pvt_irq_soc_vm...
#include <scf_power.h>           // for SCF_CORE_POWER_STATE_T
#include <sensor_fifo_service.h> // for sensor_fifo_svc_add_soc_pvt_t...
#include <stdint.h>              // for uint32_t, uint64_t, uintptr_t, uint16_t
#include <stdio.h>               // for printf
#include <string.h>              // for memcpy
#include <tx_api.h>              // for ULONG, VOID

/*-- Symbolic Constant Macros (defines) --*/
#define TEST_STATES     2
#define EXIT_TEST_VALUE 0x1234
#define MAX_VR_PER_DIE  8

/*------------- Typedefs -----------------*/
typedef VOID (*entry_function_t)(ULONG entry_input);

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static power_vrs_avs_latest_t s_vr_inputs[MAX_VR_PER_DIE] = {{.voltage = 1, .current = 2, .temperature = 3},
                                                             {.voltage = 4, .current = 5, .temperature = 6},
                                                             {.voltage = 7, .current = 8, .temperature = 9},
                                                             {.voltage = 10, .current = 11, .temperature = 12},
                                                             {.voltage = 13, .current = 14, .temperature = 15},
                                                             {.voltage = 16, .current = 17, .temperature = 18},
                                                             {.voltage = 19, .current = 20, .temperature = 21},
                                                             {.voltage = 22, .current = 23, .temperature = 24}};
static pvt_irq_soc_dts_data_t s_dts_samples = {0};
static pvt_irq_soc_vm_data_t s_vm_samples = {0};
static uint16_t s_pvt_teamp[SOC_PVT_TOTAL_CHANNELS_DTS] = {0};
static uint16_t s_pvt_voltage[SOC_PVT_TOTAL_CHANNELS_VM] = {0};

/*------------- Functions ----------------*/
//
// Mocks
//

int __wrap_soc_pvt_handle_dts_irq_exclear(uintptr_t mscp_pvt_top_base_address, pvt_irq_soc_dts_data_t* irq_data, uint32_t clear_status)
{
    check_expected(mscp_pvt_top_base_address);
    assert_non_null(irq_data);
    memcpy(irq_data, mock_ptr_type(pvt_irq_soc_dts_data_t*), sizeof(pvt_irq_soc_dts_data_t));
    check_expected(clear_status);
    return mock_type(int);
}

int __wrap_soc_pvt_handle_vm_irq_exclear(uintptr_t soc_pvt_top_base_address, pvt_irq_soc_vm_data_t* irq_data, uint32_t clear_status)
{
    check_expected(soc_pvt_top_base_address);
    assert_non_null(irq_data);
    memcpy(irq_data, mock_ptr_type(pvt_irq_soc_vm_data_t*), sizeof(pvt_irq_soc_vm_data_t));
    check_expected(clear_status);
    return mock_type(int);
}

void __wrap_sensor_fifo_svc_add_soc_pvt_temperature(soc_pvt_temp_t* pvt_temperature)
{
    assert_non_null(pvt_temperature);
    check_expected(pvt_temperature->timestamp);

    for (int pvt_idx = 0; pvt_idx < SOC_PVT_TOTAL_CHANNELS_DTS; ++pvt_idx)
    {
        assert_true(pvt_temperature->sensor_temp_dC[pvt_idx] == s_pvt_teamp[pvt_idx]);
    }

    function_called();
}

void __wrap_sensor_fifo_svc_add_soc_pvt_voltage(soc_pvt_voltage_t* pvt_voltage)
{
    assert_non_null(pvt_voltage);
    check_expected(pvt_voltage->timestamp);

    for (int pvt_idx = 0; pvt_idx < SOC_PVT_TOTAL_CHANNELS_VM; ++pvt_idx)
    {
        assert_true(pvt_voltage->sensor_voltage_mV[pvt_idx] == s_pvt_voltage[pvt_idx]);
    }

    function_called();
}

void __wrap_sensor_fifo_svc_add_vr_temperature(vr_temp_t* vr_temperature)
{
    assert_non_null(vr_temperature);
    check_expected(vr_temperature->timestamp);

    for (int vr_idx = 0; vr_idx < MAX_NUM_OF_VR_RAILS; ++vr_idx)
    {
        assert_true(vr_temperature->vr_temp_dC[vr_idx] == s_vr_inputs[vr_idx].temperature);
    }

    function_called();
}

void __wrap_sensor_fifo_svc_add_vr_current(vr_current_t* vr_current)
{
    assert_non_null(vr_current);
    check_expected(vr_current->timestamp);

    for (int vr_idx = 0; vr_idx < MAX_NUM_OF_VR_RAILS; ++vr_idx)
    {
        assert_true(vr_current->vr_current_mA[vr_idx] == s_vr_inputs[vr_idx].current * 10);
        assert_true(vr_current->vr_voltage_mV[vr_idx] == s_vr_inputs[vr_idx].voltage);
    }

    function_called();
}

// End mocks

} // extern "C"
/*-------- Function Prototypes -----------*/

/*------------- Functions ----------------*/

static int setup_telemetry_fifo(void** state)
{
    UNUSED(state);

    memset(s_pvt_teamp, 0, sizeof(s_pvt_teamp));
    memset(s_pvt_voltage, 0, sizeof(s_pvt_voltage));

    return 0;
}

void call_handler(power_vr_telem_state_t state, power_vr_telem_signal_t signal, const void* data)
{
    call_handler(VR_TELEM, (int)state, (int)signal, data);
}

void call_handler(power_pvt_telem_state_t state, power_pvt_telem_signal_t signal, const void* data)
{
    call_handler(PVT_TELEM, (int)state, (int)signal, data);
}

void setup_expectations_for_state_change(power_vr_telem_state_t state)
{
    setup_expectations_for_state_change((int)state);
}

void setup_expectations_for_state_change(power_pvt_telem_state_t state)
{
    setup_expectations_for_state_change((int)state);
}

POWER_TEST(power_loops_telemetry_init, NULL, NULL)
{
    reset_context(); // ensure we start fresh on context collection
    // this (below wrapper) also captures the context pointer when init function is called
    expect_not_value(__wrap_power_loops_init_loop, p_context, NULL);
    // this (below wrapper) also captures the context pointer when init function is called
    expect_not_value(__wrap_power_loops_init_loop, p_context, NULL);
    // this (below wrapper) also captures the context pointer when init function is called
    expect_not_value(__wrap_power_loops_init_loop, p_context, NULL);

    // call the function
    power_loops_telemetry_init();
}

// test for power_loops_vr_telem_handle_event
POWER_TEST(power_loops_telem_vr_handle_event, NULL, NULL)
{
    power_vr_telem_signal_t test_event = POWER_VR_TELEM_SIGNAL_VR_TEMP;
    const void* test_event_data = (const void*)0x1234;

    // expectations for power_loops_handle_event
    expect_not_value(__wrap_power_loops_handle_event, context, NULL);
    expect_value(__wrap_power_loops_handle_event, event, test_event);
    expect_value(__wrap_power_loops_handle_event, event_data, test_event_data);

    // call the function
    power_loops_vr_telem_handle_event(test_event, test_event_data);
}

// test for power_loops_pvt_telem_handle_event
POWER_TEST(power_loops_telem_pvt_handle_event, NULL, NULL)
{
    power_pvt_telem_signal_t test_event = POWER_PVT_TELEM_SIGNAL_INTERVAL;
    const void* test_event_data = (const void*)0x1234;

    // expectations for power_loops_handle_event
    expect_not_value(__wrap_power_loops_handle_event, context, NULL);
    expect_value(__wrap_power_loops_handle_event, event, test_event);
    expect_value(__wrap_power_loops_handle_event, event_data, test_event_data);

    // call the function
    power_loops_pvt_telem_handle_event(test_event, test_event_data);
}

// test for idle_handler
POWER_TEST(vr_telem_idle_handler__signal_entry, NULL, NULL)
{
    // nothing to setup, no expectations

    // call state handler
    call_handler(POWER_VR_TELEM_STATE_IDLE, POWER_VR_TELEM_SIGNAL_ENTRY, NULL);
}

POWER_TEST(vr_telem_idle_handler__signal_interval, NULL, NULL)
{
    setup_expectations_for_state_change(POWER_VR_TELEM_STATE_CURRENT_TELEMETRY);

    // call state handler
    call_handler(POWER_VR_TELEM_STATE_IDLE, POWER_VR_TELEM_SIGNAL_INTERVAL, NULL);
}

POWER_TEST(vr_telem_idle_handler__signal_default, NULL, NULL)
{
    // nothing should happen, no expectations to setup

    // call state handler
    call_handler(POWER_VR_TELEM_STATE_IDLE, POWER_VR_TELEM_SIGNAL_MAX, NULL);
}

// tests for vr telemetry error handler
POWER_TEST(vr_telem_idle_error_handler, NULL, NULL)
{
    // enter error state
    call_handler(POWER_VR_TELEM_STATE_ERROR, POWER_VR_TELEM_SIGNAL_ENTRY, NULL);
    // exit error state
    setup_expectations_for_state_change(POWER_VR_TELEM_STATE_IDLE);
    call_handler(POWER_VR_TELEM_STATE_ERROR, POWER_VR_TELEM_SIGNAL_INTERVAL, NULL);
}

POWER_TEST(vr_telem_error_handler__signal_default, NULL, NULL)
{
    // nothing should happen, no expectations to setup

    // call state handler
    call_handler(POWER_VR_TELEM_STATE_ERROR, POWER_VR_TELEM_SIGNAL_MAX, NULL);
}

// tests for current telemetry handler
POWER_TEST(vr_telem_current_telemetry_handler__signal_entry, NULL, NULL)
{
    power_service_config_t sconfig = {};
    power_runconfig_t test_runconfig = {.p_sconfig = &sconfig};

    // expectations on entry
    expect_function_call(__wrap_power_vrs_initiate_vr_reads);

    will_return_always(__wrap_power_runconfig_get, &test_runconfig);

    // call state handler
    call_handler(POWER_VR_TELEM_STATE_CURRENT_TELEMETRY, POWER_VR_TELEM_SIGNAL_ENTRY, NULL);
}

// tests for current telemetry handler
POWER_TEST(vr_telem_current_telemetry_handler__signal_vr_current__loop_disabled, NULL, NULL)
{
    power_service_config_t sconfig = {};
    power_runconfig_t test_runconfig = {.p_sconfig = &sconfig};
    // disable the telemetry loop
    test_runconfig.knobs.loops_disable = power_loops_disable_t_VR_TELEM_LOOP;

    // expectations on entry
    setup_expectations_for_state_change(POWER_VR_TELEM_STATE_IDLE);

    will_return(__wrap_power_runconfig_get, &test_runconfig);

    // call state handler
    call_handler(POWER_VR_TELEM_STATE_CURRENT_TELEMETRY, POWER_VR_TELEM_SIGNAL_VR_CURRENT, NULL);
}

// tests for current telemetry handler
POWER_TEST(vr_telem_current_telemetry_handler__signal_vr_current, NULL, NULL)
{
#define TELEMETRY_DIVIDER 2
    power_service_config_t sconfig = {};
    power_runconfig_t test_runconfig = {.p_sconfig = &sconfig};

    // set the temp_telemetry divider to 2
    test_runconfig.knobs.temp_telemetry_divider = TELEMETRY_DIVIDER;

    for (int entry = 0; entry < (TELEMETRY_DIVIDER - 1); entry++)
    {
        will_return(__wrap_power_runconfig_get, &test_runconfig);

        // expectations for the hw_send_telemetry (PM_TELEMETRY_VR_CURRENT)
        will_return(__wrap_power_timer_get_counter, 100);
        expect_value(__wrap_FpFwAssert, expression, true);
        expect_value(__wrap_sensor_fifo_svc_add_vr_current, vr_current->timestamp, 100);
        expect_function_call(__wrap_sensor_fifo_svc_add_vr_current);

        // every entry except the last one should not send temp telemetry
        setup_expectations_for_state_change(POWER_VR_TELEM_STATE_IDLE);
        // call state handler
        call_handler(POWER_VR_TELEM_STATE_CURRENT_TELEMETRY, POWER_VR_TELEM_SIGNAL_VR_CURRENT, s_vr_inputs);
    }

    will_return(__wrap_power_runconfig_get, &test_runconfig);

    // expectations for the hw_send_telemetry (PM_TELEMETRY_VR_CURRENT)
    will_return(__wrap_power_timer_get_counter, 100);
    expect_value(__wrap_FpFwAssert, expression, true);
    expect_value(__wrap_sensor_fifo_svc_add_vr_current, vr_current->timestamp, 100);
    expect_function_call(__wrap_sensor_fifo_svc_add_vr_current);

    // last entry should send temp telemetry
    setup_expectations_for_state_change(POWER_VR_TELEM_STATE_TEMP_TELEMETRY);
    // call state handler
    call_handler(POWER_VR_TELEM_STATE_CURRENT_TELEMETRY, POWER_VR_TELEM_SIGNAL_VR_CURRENT, s_vr_inputs);
}

POWER_TEST(vr_telem_current_telemetry_handler__signal_interval_error, NULL, NULL)
{
    power_service_config_t sconfig = {};
    power_runconfig_t test_runconfig = {.p_sconfig = &sconfig};

    will_return(__wrap_power_runconfig_get, &test_runconfig);

    setup_expectations_for_state_change(POWER_VR_TELEM_STATE_ERROR);
    setup_expectations_for_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL, true);
    // call state handler
    call_handler(POWER_VR_TELEM_STATE_CURRENT_TELEMETRY, POWER_VR_TELEM_SIGNAL_INTERVAL, NULL);
}

POWER_TEST(vr_telem_current_telemetry_handler__signal_interval_retry, NULL, NULL)
{
    power_service_config_t sconfig = {};
    power_runconfig_t test_runconfig = {.p_sconfig = &sconfig};

    will_return(__wrap_power_runconfig_get, &test_runconfig);

    setup_expectations_for_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL, false);
    // call state handler
    call_handler(POWER_VR_TELEM_STATE_CURRENT_TELEMETRY, POWER_VR_TELEM_SIGNAL_INTERVAL, NULL);
}

POWER_TEST(vr_telem_current_telemetry_handler__signal_default, NULL, NULL)
{
    // nothing should happen, no expectations to setup (except for the runconfig get)
    power_service_config_t sconfig = {};
    power_runconfig_t test_runconfig = {.p_sconfig = &sconfig};

    will_return(__wrap_power_runconfig_get, &test_runconfig);

    // call state handler
    call_handler(POWER_VR_TELEM_STATE_CURRENT_TELEMETRY, POWER_VR_TELEM_SIGNAL_MAX, NULL);
}

// tests for temp telemetry handler
POWER_TEST(vr_telem_temp_telemetry_handler__signal_entry, NULL, NULL)
{
    // expectations for the hw_send_telemetry (PM_TELEMETRY_VR_TEMP)
    will_return(__wrap_power_timer_get_counter, 100);
    expect_value(__wrap_FpFwAssert, expression, true);
    expect_value(__wrap_sensor_fifo_svc_add_vr_temperature, vr_temperature->timestamp, 100);
    expect_function_call(__wrap_sensor_fifo_svc_add_vr_temperature);

    // expect state change to idle
    setup_expectations_for_state_change(POWER_VR_TELEM_STATE_IDLE);
    // call state handler
    call_handler(POWER_VR_TELEM_STATE_TEMP_TELEMETRY, POWER_VR_TELEM_SIGNAL_ENTRY, NULL);
}

// test for pvt idle_handler
POWER_TEST(pvt_telem_idle_handler__signal_entry, NULL, NULL)
{
    // nothing to setup, no expectations

    // call state handler
    call_handler(POWER_PVT_TELEM_STATE_IDLE, POWER_PVT_TELEM_SIGNAL_ENTRY, NULL);
}

POWER_TEST(pvt_telem_idle_handler__signal_interval, NULL, NULL)
{
    setup_expectations_for_state_change(POWER_PVT_TELEM_STATE_READ_PVT);

    // call state handler
    call_handler(POWER_PVT_TELEM_STATE_IDLE, POWER_PVT_TELEM_SIGNAL_INTERVAL, NULL);
}

POWER_TEST(pvt_telem_idle_handler__signal_default, NULL, NULL)
{
    // no expectations on default signal

    // call state handler
    call_handler(POWER_PVT_TELEM_STATE_IDLE, POWER_PVT_TELEM_SIGNAL_MAX, NULL);
}

void setup_expectations_for_read_pvt(bool max_temp)
{
#define TEST_PVT_BASE 0x1234
#define TEST_TEMP_DC  900
#define TEST_VOLTS_MV 1100
#define TEST_VOLTS_F  (((float)TEST_VOLTS_MV) / 1000)
    // read pvts needs runconfig for the service config
    static power_service_config_t sconfig = {.soc_pvt_base = TEST_PVT_BASE};
    static power_runconfig_t test_runconfig = {.p_sconfig = &sconfig};

    // generate DTS data, we can validate it via the __wrap_power_hw_check_io_temp_force_pmin
    s_dts_samples.valid_bits = 0;
    for (unsigned int pvt_idx = 0; pvt_idx < SOC_PVT_TOTAL_CHANNELS_DTS; ++pvt_idx)
    {
        test_runconfig.fuses.dts_coeff_soctop[pvt_idx].k_val = (uint16_t)(DTS_K_COEFFICIENT * -1.0F);
        test_runconfig.fuses.dts_coeff_soctop[pvt_idx].y_val = (uint16_t)DTS_Y_COEFFICIENT;

        s_dts_samples.valid_bits |= (max_temp ? 0 : 1 << pvt_idx); // mark entries valid if not testing max temp
        s_dts_samples.sample_data[pvt_idx] = TEMP2DOUT_FUSED((TEST_TEMP_DC / 10),
                                                             test_runconfig.fuses.dts_coeff_soctop[0].k_val,
                                                             test_runconfig.fuses.dts_coeff_soctop[0].y_val); // div by 10, since PVT only expect temp in degrees C

        // Convert raw to temperature to check against telemetry
        // If conversion logic (in power_hw_dts_pvt_raw_to_temp_dC) changed, this will need to be updated
        s_pvt_teamp[pvt_idx] =
            (uint16_t)FLOAT_TO_UNSIGNED((DOUT2TEMP_FUSED(s_dts_samples.sample_data[pvt_idx],
                                                         test_runconfig.fuses.dts_coeff_soctop[pvt_idx].k_val,
                                                         test_runconfig.fuses.dts_coeff_soctop[pvt_idx].y_val)) *
                                        10);
    }

    s_vm_samples.valid_bits = 0;
    for (unsigned int pvt_idx = 0; pvt_idx < SOC_PVT_TOTAL_CHANNELS_VM; ++pvt_idx)
    {
        s_vm_samples.valid_bits |= 1 << pvt_idx; // mark entries valid
        s_vm_samples.sample_data[pvt_idx] = VOLTS2DOUT(TEST_VOLTS_F);

        // Convert raw to voltage to check against telemetry
        // If conversion logic (in process_pvt_vm_samples) changed, this will need to be updated
        const bool div_by_2 = ((sconfig.soc_vm[pvt_idx].flags & VM_FLAGS_DIV2) != 0);
        s_pvt_voltage[pvt_idx] =
            (uint16_t)FLOAT_TO_UNSIGNED((DOUT2VOLTS(s_vm_samples.sample_data[pvt_idx])) * (div_by_2 ? 2000 : 1000));
    }

    will_return(__wrap_power_runconfig_get, &test_runconfig);

    // expectations for soc_pvt_handle_dts_irq_exclear
    expect_value(__wrap_soc_pvt_handle_dts_irq_exclear, mscp_pvt_top_base_address, TEST_PVT_BASE);
    expect_value(__wrap_soc_pvt_handle_dts_irq_exclear, clear_status, PVT_IRQ_ALARMA | PVT_IRQ_DONE);
    will_return(__wrap_soc_pvt_handle_dts_irq_exclear, (uintptr_t)&s_dts_samples);
    will_return(__wrap_soc_pvt_handle_dts_irq_exclear, 0);

    // expectations for soc_pvt_handle_vm_irq_exclear
    expect_value(__wrap_soc_pvt_handle_vm_irq_exclear, soc_pvt_top_base_address, TEST_PVT_BASE);
    expect_value(__wrap_soc_pvt_handle_vm_irq_exclear, clear_status, PVT_IRQ_DONE);
    will_return(__wrap_soc_pvt_handle_vm_irq_exclear, (uintptr_t)&s_vm_samples);
    will_return(__wrap_soc_pvt_handle_vm_irq_exclear, 0);
}

// tests for pvt_telem_pvt_telemetry_handler
POWER_TEST(pvt_telem_pvt_telemetry_handler, setup_telemetry_fifo, NULL)
{
    // expectations for the hw_send_telemetry (PM_TELEMETRY_SOC_TOP_TEMP)
    will_return(__wrap_power_timer_get_counter, 100);
    expect_value(__wrap_sensor_fifo_svc_add_soc_pvt_temperature, pvt_temperature->timestamp, 100);
    expect_function_call(__wrap_sensor_fifo_svc_add_soc_pvt_temperature);

    // expectations for the hw_send_telemetry (PM_TELEMETRY_SOC_TOP_VOLT)
    will_return(__wrap_power_timer_get_counter, 200);
    expect_value(__wrap_sensor_fifo_svc_add_soc_pvt_voltage, pvt_voltage->timestamp, 200);
    expect_function_call(__wrap_sensor_fifo_svc_add_soc_pvt_voltage);

    // at the end we should go back to idle state
    setup_expectations_for_state_change(POWER_PVT_TELEM_STATE_IDLE);

    // expectations for the read pvt call
    setup_expectations_for_read_pvt(false);

    // call state handler
    call_handler(POWER_PVT_TELEM_STATE_READ_PVT, POWER_PVT_TELEM_SIGNAL_ENTRY, NULL);
}

// tests for pvt_telem_pvt_telemetry_handler
POWER_TEST(pvt_telem_pvt_telemetry_handler__max_temp, setup_telemetry_fifo, NULL)
{
    // expectations for the hw_send_telemetry (PM_TELEMETRY_SOC_TOP_TEMP)
    will_return(__wrap_power_timer_get_counter, 100);
    expect_value(__wrap_sensor_fifo_svc_add_soc_pvt_temperature, pvt_temperature->timestamp, 100);
    expect_function_call(__wrap_sensor_fifo_svc_add_soc_pvt_temperature);

    // expectations for the hw_send_telemetry (PM_TELEMETRY_SOC_TOP_VOLT)
    will_return(__wrap_power_timer_get_counter, 200);
    expect_value(__wrap_sensor_fifo_svc_add_soc_pvt_voltage, pvt_voltage->timestamp, 200);
    expect_function_call(__wrap_sensor_fifo_svc_add_soc_pvt_voltage);

    // at the end we should go back to idle state
    setup_expectations_for_state_change(POWER_PVT_TELEM_STATE_IDLE);

    // expectations for the read pvt call
    setup_expectations_for_read_pvt(true);

    // call state handler
    call_handler(POWER_PVT_TELEM_STATE_READ_PVT, POWER_PVT_TELEM_SIGNAL_ENTRY, NULL);
}
