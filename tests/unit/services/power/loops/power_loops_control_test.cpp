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

#include "power_i.h"           // for power_latest_calcs_t
#include "power_runconfig.h"   // for MIN_PLIMIT, power_service_config_t
#include "power_runconfig_i.h" // for power_runconfig_t
#include "power_hw_int_i.h"  
#include <CMockaWrapper.h> // for CmockaWrapperTest, expect_value, will...
#include <corebits.h>      // for corebits_set_bit
#include <pid_resource.h>  // for pid_config_t
#include <power_loops_i.h> // for _power_ctrl_loop_state_t, _power_ctrl...
#include <power_stub_i.h>  // for _power_pmin_type_t, power_pmin_type_t
#include <scf_power.h>     // for SCF_CORE_POWER_STATE_T
#include <stdint.h>        // for uint32_t, uint64_t, uintptr_t, uint16_t
#include <stdio.h>         // for printf
#include <string.h>        // for memcpy
#include <tx_api.h>        // for ULONG, VOID

/*-- Symbolic Constant Macros (defines) --*/
#define TEST_STATES     2
#define EXIT_TEST_VALUE 0x1234

/*------------- Typedefs -----------------*/
typedef VOID (*entry_function_t)(ULONG entry_input);

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static power_latest_calcs_t* s_local_power = NULL;
static power_latest_calcs_t* s_remote_power = NULL;
static power_loop_context_t* s_loop_context[LOOP_MAX] = {NULL, NULL};
static unsigned int s_context_index = 0;
/*------------- Functions ----------------*/
//
// Mocks
//

uint16_t __wrap_power_vcpu_calc_max_core_voltage_mv(power_runconfig_t* p_runconfig, power_cores_t* p_cores)
{
    FPFW_UNUSED(p_runconfig);
    FPFW_UNUSED(p_cores);    
    function_called();

    return 0;
}

float __wrap_power_vcpu_calc_peak_current_A(power_runconfig_t* p_runconfig, power_ctrl_loop_detail_t* loop_config)
{
    FPFW_UNUSED(p_runconfig);
    FPFW_UNUSED(loop_config);
    function_called();
    return 1.0F;
}

void __wrap_power_loops_init_loop(power_loop_context_t* p_context)
{
    s_loop_context[s_context_index++] = p_context;
    s_context_index = s_context_index > LOOP_MAX ? 0 : s_context_index; // only allow 2 contexts
    check_expected_ptr(p_context);
}

void __wrap_pid_init(const pid_config_t* p_config)
{
    check_expected_ptr(p_config);
}

void __wrap_pid_set_resources(uint32_t resources)
{
    check_expected(resources);
}

void __wrap_pid_reset()
{
    function_called();
}

void __wrap_pid_update_setpoint(float setpoint)
{
    check_expected(setpoint);
}

uint32_t __wrap_pid_calculate_resources(float delta_time, float measured_value)
{
    check_expected(delta_time);
    check_expected(measured_value);
    return mock_type(uint32_t);
}

void __wrap_power_loops_handle_event(power_loop_context_t* context, int event, const void* event_data)
{
    check_expected_ptr(context);
    check_expected(event);
    check_expected_ptr(event_data);
}
void __wrap_power_loops_change_state(power_loop_context_t* context, int state)
{
    check_expected_ptr(context);
    check_expected(state);
}
bool __wrap_power_loops_retry_fail(power_loop_context_t* context, power_loop_retries_t type)
{
    check_expected_ptr(context);
    check_expected(type);
    return mock_type(bool);
}

bool __wrap_power_hw_gpio_rack_limit_asserted()
{
    return mock_type(bool);
}

void __wrap_power_hw_force_pmin(power_pmin_type_t type)
{
    check_expected(type);
}

void __wrap_power_hw_clear_force_pmin(power_pmin_type_t type)
{
    check_expected(type);
}

void __wrap_power_vrs_initiate_vr_reads()
{
    function_called();
}

int __wrap_scf_get_core_state(uint32_t core_num, uintptr_t scf_mhu_base_addr, bool mpam, SCF_CORE_POWER_STATE_T* power_state)
{
    check_expected(core_num);
    check_expected(scf_mhu_base_addr);
    check_expected(mpam);
    check_expected_ptr(power_state);
    return mock_type(int);
}

float __wrap_power_cap_get_vrcpu_cap(bool* p_new_cap, power_latest_calcs_t* p_local_power, power_latest_calcs_t* p_remote_power)
{
    *p_new_cap = mock_type(bool);
    float value = mock_type(float);
    check_expected_ptr(p_local_power);
    check_expected_ptr(p_remote_power);
    if (value > 0.0F)
    {
        // provide mechanism to override measured vcpu power
        p_local_power->vcpu_power = value;
        p_remote_power->vcpu_power = value;
    }
    // save these off for later use
    s_local_power = p_local_power;
    s_remote_power = p_remote_power;
    return mock_type(float);
}

void __wrap_power_cap_finalize()
{
    function_called();
}

void __wrap_power_vrs_write_vcpu_voltage(uint16_t voltage_mv)
{
    check_expected(voltage_mv);
}

void __wrap_power_vrs_read_vcpu_voltage()
{
    function_called();
}

uint64_t __wrap_power_timer_get_counter_ticks_us(uint16_t time_in_us)
{
    check_expected(time_in_us);
    return mock_type(uint64_t);
}

uint64_t __wrap_power_timer_get_us_from_counter(uint32_t ticks)
{
    check_expected(ticks);
    return mock_type(uint64_t);
}

void __wrap_power_set_plimit(const power_runconfig_t* p_runconfig, unsigned int core, dvfs_plimit plimit)
{
    UNUSED(plimit);
    check_expected_ptr(p_runconfig);
    check_expected(core);
}

int __wrap_corebits_first(const corebits_t* corebits)
{
    assert_non_null(corebits);
    return mock_type(int);
}

void __wrap_power_telemetry_message_poll(power_cores_t* p_cores, corebits_t* p_success_bits)
{
    check_expected_ptr(p_cores);
    check_expected_ptr(p_success_bits);
}

void __wrap_power_distribution_distribute_resources(power_runconfig_t* p_runconfig, power_ctrl_loop_detail_t* p_ctrlloop)
{
    check_expected_ptr(p_runconfig);
    check_expected_ptr(p_ctrlloop);
}

void __wrap_power_distribution_distribute_warmstart_resources(power_runconfig_t* p_runconfig, power_ctrl_loop_detail_t* p_ctrlloop)
{
    check_expected_ptr(p_runconfig);
    check_expected_ptr(p_ctrlloop);
}

uint8_t __wrap_power_distribution_get_minimum_plimit(power_runconfig_t* p_runconfig, power_cores_t* p_cores, unsigned int core)
{
    check_expected_ptr(p_runconfig);
    check_expected_ptr(p_cores);
    check_expected(core);
    return mock_type(uint8_t);
}

void __wrap_power_distribution_minimum_plimit_updates(power_runconfig_t* p_runconfig, power_ctrl_loop_detail_t* p_ctrlloop)
{
    check_expected_ptr(p_runconfig);
    check_expected_ptr(p_ctrlloop);
}

// End mocks

} // extern "C"
/*-------- Function Prototypes -----------*/

/*------------- Functions ----------------*/
void reset_context()
{
    s_context_index = 0;
    for (unsigned int idx = 0; idx < LOOP_MAX; idx++)
    {
        s_loop_context[idx] = NULL;
    }
}

power_loop_context_t* loop_context(unsigned int index)
{
    assert_true(index < LOOP_MAX);
    return s_loop_context[index];
}
void call_handler(power_ctrl_loop_state_t state, power_ctrl_loop_signal_t signal, const void* data)
{
    call_handler(CTRL, (int)state, (int)signal, data);
}

void call_handler(unsigned int index, int state, int signal, const void* data)
{
    power_loop_context_t* p_context = loop_context(index);
    assert_non_null(p_context);
    p_context->handlers[state](signal, data);
}

POWER_TEST(power_loops_control_init, NULL, NULL)
{
#define TEST_VALID_CORES 3
#define NOMINAL_PLIMIT   21
#define TEST_KPT         1000
#define TEST_KIT         2000
#define TEST_KDT         3000
#define TEST_OFFSET      4000
#define TEST_MAX_WATTS   500

    power_runconfig_t test_runconfig = {};
    pid_config_t test_pid_config = {0};

    // set valid bits for the number of test cores
    for (unsigned int core = 0; core < TEST_VALID_CORES; core++)
    {
        corebits_set_bit(&test_runconfig.fuses.valid_cores, core);
    }
    // provide a nominal plimit to config
    test_runconfig.derived.pnominal = NOMINAL_PLIMIT;
    // provide pid knobs
    test_runconfig.knobs.pid.kpt = TEST_KPT;
    test_runconfig.knobs.pid.kit = TEST_KIT;
    test_runconfig.knobs.pid.kdt = TEST_KDT;
    test_runconfig.knobs.pid.setpoint_offset = TEST_OFFSET;
    // provide max watts
    test_runconfig.derived.soc_maximum_thermal_watts_limit = TEST_MAX_WATTS;

    // calculate resource counts
    unsigned int max_resources = PLIMIT_TO_RESOURCES(MIN_PLIMIT) * TEST_VALID_CORES;
    unsigned int init_resources = PLIMIT_TO_RESOURCES(NOMINAL_PLIMIT) * TEST_VALID_CORES;
    // determine PID expectation
    test_pid_config.setpoint = TEST_MAX_WATTS;
    test_pid_config.setpoint_offset = (float)TEST_OFFSET / 1000;
    test_pid_config.max = max_resources;
    test_pid_config.kp = (float)TEST_KPT / 1000;
    test_pid_config.ki = (float)TEST_KIT / 1000;
    test_pid_config.kd = (float)TEST_KDT / 1000;

    // expectations for power_loops_init_loop
    will_return(__wrap_power_runconfig_get, &test_runconfig);

    reset_context(); // ensure we start fresh on context collection
    // this (below wrapper) also captures the context pointer when init function is called
    expect_not_value(__wrap_power_loops_init_loop, p_context, NULL);

    // expectations for pid_init
    expect_memory(__wrap_pid_init, p_config, &test_pid_config, sizeof(pid_config_t));

    // expectations for pid_set_resources
    expect_value(__wrap_pid_set_resources, resources, init_resources);

    // call the function
    power_loops_control_init();
}

// test for power_control_loop_retry_fail

// test for power_control_loop_handle_event
POWER_TEST(power_loops_control_handle_event, NULL, NULL)
{
    power_ctrl_loop_signal_t test_event = POWER_CTRL_LOOP_SIGNAL_PLIMIT_PENDING;
    const void* test_event_data = (const void*)0x1234;

    // expectations for power_loops_handle_event
    expect_not_value(__wrap_power_loops_handle_event, context, NULL);
    expect_value(__wrap_power_loops_handle_event, event, test_event);
    expect_value(__wrap_power_loops_handle_event, event_data, test_event_data);

    // call the function
    power_loops_control_handle_event(test_event, test_event_data);
}

void setup_expectations_for_state_change(power_ctrl_loop_state_t state)
{
    setup_expectations_for_state_change((int)state);
}

void setup_expectations_for_state_change(int state)
{
    // runtime assert
    expect_value(__wrap_FpFwAssert, expression, true);
    // expectations for power_loops_change_state
    expect_not_value(__wrap_power_loops_change_state, context, NULL);
    expect_value(__wrap_power_loops_change_state, state, state);
}

void setup_expectations_for_retry_fail(power_loop_retries_t type, bool fail)
{
    // expectations for power_loops_retry_fail
    expect_not_value(__wrap_power_loops_retry_fail, context, NULL);
    expect_value(__wrap_power_loops_retry_fail, type, type);
    will_return(__wrap_power_loops_retry_fail, fail);
}

// test for idle_handler
POWER_TEST(control_idle_handler__signal_interval, NULL, NULL)
{
    setup_expectations_for_state_change(POWER_CONTROL_STATE_COLLECT_INPUTS);

    // call state handler
    call_handler(POWER_CONTROL_STATE_IDLE, POWER_CTRL_LOOP_SIGNAL_INTERVAL, NULL);
}

POWER_TEST(control_idle_handler__signal_default, NULL, NULL)
{
    // nothing should happen, no expectations to setup

    // call state handler
    call_handler(POWER_CONTROL_STATE_IDLE, POWER_CTRL_LOOP_SIGNAL_MAX, NULL);
}

POWER_TEST(control_idle_error_handler__enter_exit_error, NULL, NULL)
{
    // simulate entering error state, then entering idle with last state an error, and entering idle with last
    // state not an error setup error expectation
    expect_value(__wrap_power_hw_force_pmin, type, PM_FW_PMIN_CONTROL);
    // call error handler
    call_handler(POWER_CONTROL_STATE_ERROR, POWER_CTRL_LOOP_SIGNAL_ENTRY, NULL);
    // setup error expectation (none, nothing should happen if last state is error)
    loop_context(CTRL)->status.last_state = POWER_CONTROL_STATE_ERROR;
    // call error handler
    call_handler(POWER_CONTROL_STATE_IDLE, POWER_CTRL_LOOP_SIGNAL_ENTRY, NULL);
    // setup error expectation
    loop_context(CTRL)->status.last_state = POWER_CONTROL_STATE_EXCHANGE_COMPLETION;
    expect_value(__wrap_power_hw_clear_force_pmin, type, PM_FW_PMIN_CONTROL);
    call_handler(POWER_CONTROL_STATE_IDLE, POWER_CTRL_LOOP_SIGNAL_ENTRY, NULL);
}

// tests for error_handler
POWER_TEST(control_error_handler__signal_interval, NULL, NULL)
{
    setup_expectations_for_state_change(POWER_CONTROL_STATE_IDLE);

    // call state handler
    call_handler(POWER_CONTROL_STATE_ERROR, POWER_CTRL_LOOP_SIGNAL_INTERVAL, NULL);
}

POWER_TEST(control_error_handler__signal_default, NULL, NULL)
{
    // nothing should happen, no expectations to setup

    // call state handler
    call_handler(POWER_CONTROL_STATE_ERROR, POWER_CTRL_LOOP_SIGNAL_MAX, NULL);
}

void set_expectations_for_get_core_state(power_runconfig_t* p_runconfig)
{
#define TEST_MHU_BASE 0x1234
    // set core 0 to valid for HW reads
    corebits_set_bit(&p_runconfig->fuses.valid_cores, 0);
    // provide MHU base address
    uintptr_t mhu_base = TEST_MHU_BASE;
    memcpy((void*)&p_runconfig->p_sconfig->scf_mhu_base, &mhu_base, sizeof(uintptr_t));
    // expectations for scf_get_core_state
    expect_value(__wrap_scf_get_core_state, core_num, 0);
    expect_value(__wrap_scf_get_core_state, scf_mhu_base_addr, TEST_MHU_BASE);
    expect_value(__wrap_scf_get_core_state, mpam, false);
    expect_not_value(__wrap_scf_get_core_state, power_state, NULL);
    will_return(__wrap_scf_get_core_state, 0);

    // runtime assert
    expect_value(__wrap_FpFwAssert, expression, true);
}

void setup_message_poll()
{
    expect_not_value(__wrap_power_telemetry_message_poll, p_cores, NULL);
    expect_not_value(__wrap_power_telemetry_message_poll, p_success_bits, NULL);
}

// tests for collect_inputs_handler
POWER_TEST(control_collect_inputs_handler__signal_entry, NULL, NULL)
{
#define DEFAULT_MIN_PLIMIT 0x10
#define DEFAULT_CORE_ZERO 0 // needs to be 0 for this test

    power_service_config_t sconfig = {};
    power_runconfig_t test_runconfig = {.p_sconfig = &sconfig};
    // expectations on entry

    // expectations for get_current_state
    // set core 0 to valid for HW reads
    corebits_set_bit(&test_runconfig.fuses.valid_cores, DEFAULT_CORE_ZERO);

    will_return(__wrap_power_runconfig_get, &test_runconfig);

    set_expectations_for_get_core_state(&test_runconfig);

    setup_message_poll();

    // will get minimum plimit
    expect_value(__wrap_power_distribution_get_minimum_plimit, core, DEFAULT_CORE_ZERO);
    expect_value(__wrap_power_distribution_get_minimum_plimit, p_runconfig, &test_runconfig);
    expect_not_value(__wrap_power_distribution_get_minimum_plimit, p_cores, NULL);
    will_return(__wrap_power_distribution_get_minimum_plimit, DEFAULT_MIN_PLIMIT);

    // call state handler
    call_handler(POWER_CONTROL_STATE_COLLECT_INPUTS, POWER_CTRL_LOOP_SIGNAL_ENTRY, NULL);

}

POWER_TEST(control_collect_inputs_handler__signal_vr_read, NULL, NULL)
{
    power_latest_calcs_t test_calcs = {};

    // expect we change state to exchange inputs on this signal
    setup_expectations_for_state_change(POWER_CONTROL_STATE_EXCHANGE_INPUTS);

    // handle the runtime assert
    expect_value(__wrap_FpFwAssert, expression, true);

    // call state handler
    call_handler(POWER_CONTROL_STATE_COLLECT_INPUTS, POWER_CTRL_LOOP_SIGNAL_VR_READ, &test_calcs);
}

POWER_TEST(control_collect_inputs_handler__signal_interval_error, NULL, NULL)
{
    setup_expectations_for_state_change(POWER_CONTROL_STATE_ERROR);
    setup_expectations_for_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL, true);
    // call state handler
    call_handler(POWER_CONTROL_STATE_COLLECT_INPUTS, POWER_CTRL_LOOP_SIGNAL_INTERVAL, NULL);
}

POWER_TEST(control_collect_inputs_handler__signal_interval_retry, NULL, NULL)
{
    setup_expectations_for_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL, false);
    // call state handler
    call_handler(POWER_CONTROL_STATE_COLLECT_INPUTS, POWER_CTRL_LOOP_SIGNAL_INTERVAL, NULL);
}

POWER_TEST(control_collect_inputs_handler__signal_default, NULL, NULL)
{
    // nothing should happen, no expectations to setup

    // call state handler
    call_handler(POWER_CONTROL_STATE_COLLECT_INPUTS, POWER_CTRL_LOOP_SIGNAL_MAX, NULL);
}

// tests for warmstart entry handler
POWER_TEST(control_warmstart_handler__signal_entry, NULL, NULL)
{
    power_runconfig_t test_runconfig = {.p_sconfig = NULL};
    // expectations on entry
    will_return(__wrap_power_runconfig_get, &test_runconfig);
    expect_function_call(__wrap_power_vcpu_calc_max_core_voltage_mv);
    expect_function_call(__wrap_power_vcpu_calc_peak_current_A);

    // should change state to set VR before PLIMIT after handling fixed warmstart distribution
    setup_expectations_for_state_change(POWER_CONTROL_STATE_SET_VR_BEFORE_PLIMIT);
    
    // setup expectations for warmstart distribution
    expect_value(__wrap_power_distribution_distribute_warmstart_resources, p_runconfig, &test_runconfig);
    expect_not_value(__wrap_power_distribution_distribute_warmstart_resources, p_ctrlloop, NULL);
    
    // call state handler
    call_handler(POWER_CONTROL_STATE_WARMSTART_ENTRY, POWER_CTRL_LOOP_SIGNAL_ENTRY, NULL);
}

void setup_dist_available_signal_entry(bool new_cap, bool rack_limit)
{
#define TEST_VCPU_POWER          100.0F
#define TEST_MEASURED_VCPU_POWER 60.0F

    power_runconfig_t test_runconfig = {.p_sconfig = NULL};
    // expectations on entry
    will_return(__wrap_power_runconfig_get, &test_runconfig);

    // handle cpu cap calculation, new cap, etc
    will_return(__wrap_power_cap_get_vrcpu_cap, new_cap);
    will_return(__wrap_power_cap_get_vrcpu_cap, TEST_MEASURED_VCPU_POWER);
    expect_not_value(__wrap_power_cap_get_vrcpu_cap, p_local_power, NULL);
    expect_not_value(__wrap_power_cap_get_vrcpu_cap, p_remote_power, NULL);
    will_return(__wrap_power_cap_get_vrcpu_cap, TEST_VCPU_POWER);

    expect_value(__wrap_pid_update_setpoint, setpoint, TEST_VCPU_POWER);

    if (new_cap)
    {
        expect_function_call(__wrap_pid_reset);
    }

    // handle rack limit
    will_return(__wrap_power_hw_gpio_rack_limit_asserted, rack_limit);
    if (rack_limit)
    {
        expect_function_call(__wrap_pid_reset);
        expect_value(__wrap_pid_set_resources, resources, 0);
    }
    else
    {
        expect_any(__wrap_pid_calculate_resources, delta_time);
        expect_value(__wrap_pid_calculate_resources, measured_value, (float)TEST_MEASURED_VCPU_POWER + (float)TEST_MEASURED_VCPU_POWER);
        will_return(__wrap_pid_calculate_resources, 0);
    }

    expect_function_call(__wrap_power_vcpu_calc_max_core_voltage_mv);
    expect_function_call(__wrap_power_vcpu_calc_peak_current_A);

    setup_expectations_for_state_change(POWER_CONTROL_STATE_SET_VR_BEFORE_PLIMIT);

    // set expectations for distribute resources
    expect_value(__wrap_power_distribution_distribute_resources, p_runconfig, &test_runconfig);
    expect_not_value(__wrap_power_distribution_distribute_resources, p_ctrlloop, NULL);
    // set expectations for minimum plimit updates
    expect_value(__wrap_power_distribution_minimum_plimit_updates, p_runconfig, &test_runconfig);
    expect_not_value(__wrap_power_distribution_minimum_plimit_updates, p_ctrlloop, NULL);

    // call state handler
    call_handler(POWER_CONTROL_STATE_DISTRIBUTE_AVAILABLE, POWER_CTRL_LOOP_SIGNAL_ENTRY, NULL);
}

// tests for distribute available handler
POWER_TEST(control_distribute_available_handler__signal_entry, NULL, NULL)
{
    setup_dist_available_signal_entry(false, false);
}

POWER_TEST(control_distribute_available_handler__signal_entry__new_cap, NULL, NULL)
{
    setup_dist_available_signal_entry(true, false);
}

POWER_TEST(control_distribute_available_handler__signal_entry__rack_limit, NULL, NULL)
{
    setup_dist_available_signal_entry(false, true);
}

POWER_TEST(control_distribute_available_handler__signal_default, NULL, NULL)
{
    // nothing should happen, no expectations to setup except runconfig read
    power_runconfig_t test_runconfig = {.p_sconfig = NULL};
    // expectations on entry
    will_return(__wrap_power_runconfig_get, &test_runconfig);

    // call state handler
    call_handler(POWER_CONTROL_STATE_DISTRIBUTE_AVAILABLE, POWER_CTRL_LOOP_SIGNAL_MAX, NULL);
}

// tests for set VR after PLIMIT handler

POWER_TEST(control_set_vr_after_handler__signal_entry, NULL, NULL)
{
    // expectations on entry
    expect_any(__wrap_power_vrs_write_vcpu_voltage, voltage_mv);

    // call state handler
    call_handler(POWER_CONTROL_STATE_SET_VR_AFTER_PLIMIT, POWER_CTRL_LOOP_SIGNAL_ENTRY, NULL);
}

POWER_TEST(control_set_vr_after_handler__signal_fail, NULL, NULL)
{
    // expectations on entry
    expect_any(__wrap_power_vrs_write_vcpu_voltage, voltage_mv);

    // call state handler
    call_handler(POWER_CONTROL_STATE_SET_VR_AFTER_PLIMIT, POWER_CTRL_LOOP_SIGNAL_VCPU_SET_FAIL, NULL);
}

POWER_TEST(control_set_vr_after_handler__signal_pending, NULL, NULL)
{
    // expectations on entry
    setup_expectations_for_state_change(POWER_CONTROL_STATE_WAIT_VR_AFTER_PLIMIT);

    // call state handler
    call_handler(POWER_CONTROL_STATE_SET_VR_AFTER_PLIMIT, POWER_CTRL_LOOP_SIGNAL_VCPU_PENDING, NULL);
}

POWER_TEST(control_set_vr_after_handler__signal_done, NULL, NULL)
{
    // expectations on entry
    setup_expectations_for_state_change(POWER_CONTROL_STATE_EXCHANGE_COMPLETION);

    // call state handler
    call_handler(POWER_CONTROL_STATE_SET_VR_AFTER_PLIMIT, POWER_CTRL_LOOP_SIGNAL_VCPU_DONE, NULL);
}

POWER_TEST(control_set_vr_after_handler__signal_interval_error, NULL, NULL)
{
    setup_expectations_for_state_change(POWER_CONTROL_STATE_ERROR);
    setup_expectations_for_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL, true);
    // call state handler
    call_handler(POWER_CONTROL_STATE_SET_VR_AFTER_PLIMIT, POWER_CTRL_LOOP_SIGNAL_INTERVAL, NULL);
}

POWER_TEST(control_set_vr_after_handler__signal_interval_retry, NULL, NULL)
{
    setup_expectations_for_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL, false);
    // call state handler
    call_handler(POWER_CONTROL_STATE_SET_VR_AFTER_PLIMIT, POWER_CTRL_LOOP_SIGNAL_INTERVAL, NULL);
}

POWER_TEST(control_set_vr_after_handler__signal_default, NULL, NULL)
{
    // nothing should happen, no expectations to setup

    // call state handler
    call_handler(POWER_CONTROL_STATE_SET_VR_AFTER_PLIMIT, POWER_CTRL_LOOP_SIGNAL_MAX, NULL);
}

// tests for wait VR after PLIMIT handler
POWER_TEST(control_wait_vr_after_handler__signal_entry, NULL, NULL)
{
    // expectations on entry
    expect_function_call(__wrap_power_vrs_read_vcpu_voltage);

    // call state handler
    call_handler(POWER_CONTROL_STATE_WAIT_VR_AFTER_PLIMIT, POWER_CTRL_LOOP_SIGNAL_ENTRY, NULL);
}

POWER_TEST(control_wait_vr_after_handler__signal_fail, NULL, NULL)
{
    // expectations on entry
    expect_function_call(__wrap_power_vrs_read_vcpu_voltage);

    // call state handler
    call_handler(POWER_CONTROL_STATE_WAIT_VR_AFTER_PLIMIT, POWER_CTRL_LOOP_SIGNAL_VCPU_SET_FAIL, NULL);
}

POWER_TEST(control_wait_vr_after_handler__signal_pending, NULL, NULL)
{
    // expectations on entry
    expect_function_call(__wrap_power_vrs_read_vcpu_voltage);

    // call state handler
    call_handler(POWER_CONTROL_STATE_WAIT_VR_AFTER_PLIMIT, POWER_CTRL_LOOP_SIGNAL_VCPU_PENDING, NULL);
}

POWER_TEST(control_wait_vr_after_handler__signal_done, NULL, NULL)
{
    // expectations on entry
    setup_expectations_for_state_change(POWER_CONTROL_STATE_EXCHANGE_COMPLETION);

    // call state handler
    call_handler(POWER_CONTROL_STATE_WAIT_VR_AFTER_PLIMIT, POWER_CTRL_LOOP_SIGNAL_VCPU_DONE, NULL);
}

POWER_TEST(control_wait_vr_after_handler__signal_interval_error, NULL, NULL)
{
    setup_expectations_for_state_change(POWER_CONTROL_STATE_ERROR);
    setup_expectations_for_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL, true);
    // call state handler
    call_handler(POWER_CONTROL_STATE_WAIT_VR_AFTER_PLIMIT, POWER_CTRL_LOOP_SIGNAL_INTERVAL, NULL);
}

POWER_TEST(control_wait_vr_after_handler__signal_interval_retry, NULL, NULL)
{
    setup_expectations_for_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL, false);
    // call state handler
    call_handler(POWER_CONTROL_STATE_WAIT_VR_AFTER_PLIMIT, POWER_CTRL_LOOP_SIGNAL_INTERVAL, NULL);
}

POWER_TEST(control_wait_vr_after_handler__signal_default, NULL, NULL)
{
    // nothing should happen, no expectations to setup

    // call state handler
    call_handler(POWER_CONTROL_STATE_WAIT_VR_AFTER_PLIMIT, POWER_CTRL_LOOP_SIGNAL_MAX, NULL);
}

// simple tests for VR before handler
POWER_TEST(control_set_vr_before_handler__signal_pending, NULL, NULL)
{
    // expectations on entry
    setup_expectations_for_state_change(POWER_CONTROL_STATE_WAIT_VR_BEFORE_PLIMIT);

    // call state handler
    call_handler(POWER_CONTROL_STATE_SET_VR_BEFORE_PLIMIT, POWER_CTRL_LOOP_SIGNAL_VCPU_PENDING, NULL);
}

POWER_TEST(control_set_vr_before_handler__signal_done, NULL, NULL)
{
    // expectations on entry
    setup_expectations_for_state_change(POWER_CONTROL_STATE_SET_PLIMIT_AFTER_VR);

    // call state handler
    call_handler(POWER_CONTROL_STATE_SET_VR_BEFORE_PLIMIT, POWER_CTRL_LOOP_SIGNAL_VCPU_DONE, NULL);
}

POWER_TEST(control_wait_vr_before_handler__signal_done, NULL, NULL)
{
    // expectations on entry
    setup_expectations_for_state_change(POWER_CONTROL_STATE_SET_PLIMIT_AFTER_VR);

    // call state handler
    call_handler(POWER_CONTROL_STATE_WAIT_VR_BEFORE_PLIMIT, POWER_CTRL_LOOP_SIGNAL_VCPU_DONE, NULL);
}

void setup_write_plimits()
{
#define TEST_CORE 5

    // expectations for power_set_plimit
    expect_any(__wrap_power_set_plimit, p_runconfig);
    expect_value(__wrap_power_set_plimit, core, TEST_CORE);
    will_return(__wrap_corebits_first, TEST_CORE);
    will_return(__wrap_corebits_first, -1);
}

// set plimit after tests
void setup_plimit_handler()
{
    static power_runconfig_t test_runconfig = {.p_sconfig = NULL};
    // expectations on entry
    will_return(__wrap_power_runconfig_get, &test_runconfig);

    will_return(__wrap_power_timer_get_counter_ticks_us, 1000); // some default value for now
    expect_any(__wrap_power_timer_get_counter_ticks_us, time_in_us);

    will_return(__wrap_power_timer_get_counter, 0); // start counter at 0
}

POWER_TEST(control_set_plimit_after_handler__signal_entry, NULL, NULL)
{
    // expectations on entry
    setup_plimit_handler();
    setup_write_plimits();

    // expectations for power_loops_handle_event (entry case)
    expect_not_value(__wrap_power_loops_handle_event, context, NULL);
    expect_value(__wrap_power_loops_handle_event, event, POWER_CTRL_LOOP_SIGNAL_PLIMIT_PENDING);
    expect_value(__wrap_power_loops_handle_event, event_data, NULL);

    // call state handler
    call_handler(POWER_CONTROL_STATE_SET_PLIMIT_AFTER_VR, POWER_CTRL_LOOP_SIGNAL_ENTRY, NULL);
}

POWER_TEST(control_set_plimit_after_handler__signal_plimit, NULL, NULL)
{
    // expectations on entry
    setup_plimit_handler();
    setup_message_poll();

    // expectations for pending case
    expect_any(__wrap_power_timer_get_us_from_counter, ticks);
    will_return(__wrap_power_timer_get_us_from_counter, 1000); // some default value for now

    // expect cap finalization
    expect_function_call(__wrap_power_cap_finalize);

    setup_expectations_for_state_change(POWER_CONTROL_STATE_EXCHANGE_COMPLETION);

    // call state handler
    call_handler(POWER_CONTROL_STATE_SET_PLIMIT_AFTER_VR, POWER_CTRL_LOOP_SIGNAL_PLIMIT_PENDING, NULL);
}

POWER_TEST(control_set_plimit_after_handler__signal_default, NULL, NULL)
{
    // expectations on entry
    setup_plimit_handler();

    // call state handler
    call_handler(POWER_CONTROL_STATE_SET_PLIMIT_AFTER_VR, POWER_CTRL_LOOP_SIGNAL_MAX, NULL);
}

POWER_TEST(control_set_plimit_after_handler__signal_interval_error, NULL, NULL)
{
    // expectations on entry
    setup_plimit_handler();

    setup_expectations_for_state_change(POWER_CONTROL_STATE_ERROR);
    setup_expectations_for_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL, true);
    // call state handler
    call_handler(POWER_CONTROL_STATE_SET_PLIMIT_AFTER_VR, POWER_CTRL_LOOP_SIGNAL_INTERVAL, NULL);
}

POWER_TEST(control_set_plimit_after_handler__signal_interval_retry, NULL, NULL)
{
    // expectations on entry
    setup_plimit_handler();

    setup_expectations_for_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL, false);
    // call state handler
    call_handler(POWER_CONTROL_STATE_SET_PLIMIT_AFTER_VR, POWER_CTRL_LOOP_SIGNAL_INTERVAL, NULL);
}

// simple set plimit before tests
POWER_TEST(control_set_plimit_before_handler__signal_plimit, NULL, NULL)
{
    // expectations on entry
    setup_plimit_handler();
    setup_message_poll();

    // expectations for pending case
    expect_any(__wrap_power_timer_get_us_from_counter, ticks);
    will_return(__wrap_power_timer_get_us_from_counter, 1000); // some default value for now

    // expect cap finalization
    expect_function_call(__wrap_power_cap_finalize);

    setup_expectations_for_state_change(POWER_CONTROL_STATE_SET_VR_AFTER_PLIMIT);

    // call state handler
    call_handler(POWER_CONTROL_STATE_SET_PLIMIT_BEFORE_VR, POWER_CTRL_LOOP_SIGNAL_PLIMIT_PENDING, NULL);
}

// exchange input tests
POWER_TEST(control_exchange_inputs_handler__signal_entry, NULL, NULL)
{
    // expectations on entry
    setup_expectations_for_state_change(POWER_CONTROL_STATE_DISTRIBUTE_AVAILABLE);

    // call state handler
    call_handler(POWER_CONTROL_STATE_EXCHANGE_INPUTS, POWER_CTRL_LOOP_SIGNAL_ENTRY, NULL);
}

POWER_TEST(control_exchange_inputs_handler__signal_interval_error, NULL, NULL)
{
    // expectations on entry
    setup_expectations_for_state_change(POWER_CONTROL_STATE_ERROR);
    setup_expectations_for_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL, true);
    // call state handler
    call_handler(POWER_CONTROL_STATE_EXCHANGE_INPUTS, POWER_CTRL_LOOP_SIGNAL_INTERVAL, NULL);
}

POWER_TEST(control_exchange_inputs_handler__signal_interval_retry, NULL, NULL)
{
    // expectations on entry
    setup_expectations_for_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL, false);
    // call state handler
    call_handler(POWER_CONTROL_STATE_EXCHANGE_INPUTS, POWER_CTRL_LOOP_SIGNAL_INTERVAL, NULL);
}

// exchange completion tests
POWER_TEST(control_exchange_completion_handler__signal_entry, NULL, NULL)
{
    // expectations on entry
    setup_expectations_for_state_change(POWER_CONTROL_STATE_IDLE);

    // call state handler
    call_handler(POWER_CONTROL_STATE_EXCHANGE_COMPLETION, POWER_CTRL_LOOP_SIGNAL_ENTRY, NULL);
}

POWER_TEST(control_exchange_completion_handler__signal_interval_error, NULL, NULL)
{
    // expectations on entry
    setup_expectations_for_state_change(POWER_CONTROL_STATE_ERROR);
    setup_expectations_for_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL, true);
    // call state handler
    call_handler(POWER_CONTROL_STATE_EXCHANGE_COMPLETION, POWER_CTRL_LOOP_SIGNAL_INTERVAL, NULL);
}

POWER_TEST(control_exchange_completion_handler__signal_interval_retry, NULL, NULL)
{
    // expectations on entry
    setup_expectations_for_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL, false);
    // call state handler
    call_handler(POWER_CONTROL_STATE_EXCHANGE_COMPLETION, POWER_CTRL_LOOP_SIGNAL_INTERVAL, NULL);
}
