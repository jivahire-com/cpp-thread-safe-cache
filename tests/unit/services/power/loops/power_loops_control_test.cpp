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
#include "pid_resource.h" // for pid_config_t, pid_context_t
#include "power_hw_int_i.h"
#include "power_i.h"       // for power_latest_calcs_t
#include "power_log.h"     // for POWER_LOG_INFO
#include "power_loops_i.h" // for power_loops_control_init
#include "power_pldm_scp.h"
#include "power_remote_die_i.h"
#include "power_runconfig.h"   // for MIN_PLIMIT, power_service_config_t
#include "power_runconfig_i.h" // for power_runconfig_t

#include <CMockaWrapper.h> // for CmockaWrapperTest, expect_value, will...
#include <corebits.h>      // for corebits_set_bit
#include <idsw_kng.h>
#include <pid_resource.h>  // for pid_config_t
#include <power_loops_i.h> // for _power_ctrl_loop_state_t, _power_ctrl...
#include <scf_power.h>     // for SCF_CORE_POWER_STATE_T
#include <stdint.h>        // for uint32_t, uint64_t, uintptr_t, uint16_t
#include <stdio.h>         // for printf
#include <string.h>        // for memcpy
#include <tx_api.h>        // for ULONG, VOID

/*-- Symbolic Constant Macros (defines) --*/
#define TEST_STATES     2
#define EXIT_TEST_VALUE 0x1234
#define TEST_TIMER_VAL  0x1234
/*------------- Typedefs -----------------*/
typedef VOID (*entry_function_t)(ULONG entry_input);

/*-------- Function Prototypes -----------*/
void __real_power_vrs_initiate_vr_reads();

/*-- Declarations (Statics and globals) --*/
static power_latest_calcs_t* s_local_power = NULL;
static power_latest_calcs_t* s_remote_power = NULL;
static power_loop_context_t* s_loop_context[LOOP_MAX] = {NULL, NULL};
static power_hw_update_cb_t s_update_cb = NULL;
static power_hw_success_cb_t s_success_cb = NULL;
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
    __real_power_vrs_initiate_vr_reads();
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

void __wrap_power_telemetry_message_poll(power_hw_update_cb_t p_update_cb, power_hw_success_cb_t p_success_cb)
{
    check_expected(p_update_cb);
    check_expected(p_success_cb);
    s_success_cb = p_success_cb;
    s_update_cb = p_update_cb;
}

void __wrap_power_distribution_distribute_resources(power_runconfig_t* p_runconfig, power_ctrl_loop_detail_t* p_ctrlloop)
{
    check_expected_ptr(p_runconfig);
    check_expected_ptr(p_ctrlloop);
}

void __wrap_power_distribution_distribute_warmstart_resources(power_runconfig_t* p_runconfig,
                                                              power_ctrl_loop_detail_t* p_ctrlloop)
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

void __wrap_power_remote_die_idle_reset()
{
    function_called();
}

void __wrap_power_remote_die_init(power_runconfig_t* p_runconfig)
{
    check_expected_ptr(p_runconfig);
}

void __wrap_power_remote_die_exchange_inputs(power_runconfig_t* p_runconfig)
{
    check_expected_ptr(p_runconfig);
}

void __wrap_power_remote_die_exchange_complete(power_runconfig_t* p_runconfig)
{
    check_expected_ptr(p_runconfig);
}

void __wrap_power_remote_die_sync_barrier(power_runconfig_t* p_runconfig)
{
    check_expected_ptr(p_runconfig);
}

void __wrap_power_hw_capture_cppc_state(power_hw_update_cb_t p_update_cb)
{
    s_update_cb = p_update_cb;
    function_called();
}

void __wrap_power_control_loop_change_state()
{
    function_called();
}

void __wrap_power_log_cores_ts(uint64_t timestamp, const corebits_t* cores, uint8_t type, power_log_payload_t* payload)
{
    UNUSED(timestamp);
    UNUSED(cores);
    UNUSED(type);
    UNUSED(payload);
    function_called();
}

void __wrap_power_log_core(unsigned int core, uint8_t type, power_log_payload_t* payload)
{
    UNUSED(core);
    UNUSED(type);
    UNUSED(payload);
    function_called();
}

void __wrap_store_remote_soc_power(power_latest_calcs_t* p_remote_power)
{
    assert_non_null(p_remote_power);
    function_called();
}

int __wrap_power_cap_update(power_cap_completed_callback_t callback, uint16_t new_power_cap, bool source_is_cli)
{
    assert_non_null(callback);
    check_expected(new_power_cap);
    check_expected(source_is_cli);
    return mock_type(int);
}

static bool mock_die_id_1_flag = false;

void set_mock_die_id_flag(bool value)
{
    mock_die_id_1_flag = value;
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    if (mock_die_id_1_flag)
    {
        mock_die_id_1_flag = false;
        return 1;
    }
    return 0;
}

void __wrap_pid_set_context(const pid_context_t* context)
{
    assert_non_null(context);
    function_called();
}

void __wrap_pid_get_context(pid_context_t* context)
{
    assert_non_null(context);
    function_called();
}

void __wrap_power_pldm_service_init(power_runconfig_t* p_runconfig)
{
    check_expected_ptr(p_runconfig);
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

    expect_value(__wrap_power_remote_die_init, p_runconfig, &test_runconfig);

    expect_value(__wrap_power_pldm_service_init, p_runconfig, &test_runconfig);

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
    setup_expectations_for_state_change(POWER_CONTROL_STATE_SYNC_DIES);
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
    // call state handler
    call_handler(POWER_CONTROL_STATE_IDLE, POWER_CTRL_LOOP_SIGNAL_INTERVAL, NULL);
}

POWER_TEST(control_idle_handler__signal_default, NULL, NULL)
{
    // nothing should happen, no expectations to setup
    // will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
    // call state handler
    call_handler(POWER_CONTROL_STATE_IDLE, POWER_CTRL_LOOP_SIGNAL_MAX, NULL);
}

// tests for sync_dies_handler
POWER_TEST(control_sync_dies_handler__signal_entry, NULL, NULL)
{
    power_runconfig_t test_runconfig = {.p_sconfig = NULL};
    will_return(__wrap_power_runconfig_get, &test_runconfig);
    expect_value(__wrap_power_remote_die_sync_barrier, p_runconfig, &test_runconfig);
    // call state handler
    call_handler(POWER_CONTROL_STATE_SYNC_DIES, POWER_CTRL_LOOP_SIGNAL_ENTRY, NULL);
}

POWER_TEST(control_sync_dies_handler__signal_sync_complete, NULL, NULL)
{
    setup_expectations_for_state_change(POWER_CONTROL_STATE_COLLECT_INPUTS);
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
    // call state handler
    call_handler(POWER_CONTROL_STATE_SYNC_DIES, POWER_CTRL_LOOP_SIGNAL_SYNC_COMPLETE, NULL);
}

POWER_TEST(control_sync_dies_handler__signal_interval_retry, NULL, NULL)
{
    power_runconfig_t test_runconfig = {.p_sconfig = NULL};
    will_return(__wrap_power_runconfig_get, &test_runconfig);
    expect_value(__wrap_power_remote_die_sync_barrier, p_runconfig, &test_runconfig);
    setup_expectations_for_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL, false);
    // call state handler
    call_handler(POWER_CONTROL_STATE_SYNC_DIES, POWER_CTRL_LOOP_SIGNAL_INTERVAL, NULL);
}

POWER_TEST(control_sync_dies_handler__signal_interval_error, NULL, NULL)
{
    setup_expectations_for_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL, true);
    setup_expectations_for_state_change(POWER_CONTROL_STATE_ERROR);
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
    // call state handler
    call_handler(POWER_CONTROL_STATE_SYNC_DIES, POWER_CTRL_LOOP_SIGNAL_INTERVAL, NULL);
}

POWER_TEST(control_sync_dies_handler__signal_default, NULL, NULL)
{
    // nothing should happen, no expectations to setup
    // call state handler
    call_handler(POWER_CONTROL_STATE_SYNC_DIES, POWER_CTRL_LOOP_SIGNAL_MAX, NULL);
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
    // expect remote_die_idle_reset
    expect_function_call(__wrap_power_remote_die_idle_reset);
    // call error handler
    call_handler(POWER_CONTROL_STATE_IDLE, POWER_CTRL_LOOP_SIGNAL_ENTRY, NULL);
    // setup error expectation
    loop_context(CTRL)->status.last_state = POWER_CONTROL_STATE_EXCHANGE_COMPLETION;
    expect_value(__wrap_power_hw_clear_force_pmin, type, PM_FW_PMIN_CONTROL);
    // expect remote_die_idle_reset
    expect_function_call(__wrap_power_remote_die_idle_reset);
    call_handler(POWER_CONTROL_STATE_IDLE, POWER_CTRL_LOOP_SIGNAL_ENTRY, NULL);
}

// tests for error_handler
POWER_TEST(control_error_handler__signal_interval, NULL, NULL)
{
    setup_expectations_for_state_change(POWER_CONTROL_STATE_IDLE);
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
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
    expect_not_value(__wrap_power_telemetry_message_poll, p_update_cb, NULL);
    expect_not_value(__wrap_power_telemetry_message_poll, p_success_cb, NULL);
}

// tests for collect_inputs_handler
POWER_TEST(control_collect_inputs_handler__signal_entry, NULL, NULL)
{
#define DEFAULT_MIN_PLIMIT 0x10
#define INVALID_COUNT_DATA 0x12
#define DEFAULT_CORE_ZERO  0 // needs to be 0 for this test
#define TEST_BOOST_PRI     1

    power_service_config_t sconfig = {};
    power_runconfig_t test_runconfig = {
        .derived = {.pnominal = NOMINAL_PLIMIT},
        .p_sconfig = &sconfig,
    };

    // set core 0 to valid for HW reads
    corebits_set_bit(&test_runconfig.fuses.valid_cores, DEFAULT_CORE_ZERO);

    // get internal control loop data
    power_ctrl_loop_detail_t* p_ctrl_loop = power_ctrl_loop_get();
    assert_non_null(p_ctrl_loop);
    // memset priority data
    memset(&p_ctrl_loop->local.pri_counts, 0, sizeof(p_ctrl_loop->local.pri_counts));
    // put a value into the boost count data to prove it get's cleared
    p_ctrl_loop->local.pri_counts.required_for_boost[0][0] = INVALID_COUNT_DATA;
    p_ctrl_loop->local.pri_counts.required_for_boost[NOMINAL_PLIMIT - 1][VM_BOOST_COUNT - 1] = INVALID_COUNT_DATA;

    // define boost priority for core
    p_ctrl_loop->cores.core[DEFAULT_CORE_ZERO].current_boost_priority = TEST_BOOST_PRI;
    // expectations on entry

    // expectations for get_current_state
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

    // check boost data generated
    for (unsigned boost_idx = 0; boost_idx < VM_BOOST_COUNT; boost_idx++)
    {
        for (unsigned pstate_idx = 0; pstate_idx < NUM_PSTATES; pstate_idx++)
        {
            // we had one core, nothing should be limiting min_pstate, so in the test_boost_pri priority, we should have a count of 1 required for boost in all pstates < nominal
            if ((boost_idx == TEST_BOOST_PRI) && (pstate_idx < NOMINAL_PLIMIT) && (pstate_idx >= DEFAULT_MIN_PLIMIT))
            {
                assert_int_equal(p_ctrl_loop->local.pri_counts.required_for_boost[pstate_idx][boost_idx], 1);
            }
            else
            {
                assert_int_equal(p_ctrl_loop->local.pri_counts.required_for_boost[pstate_idx][boost_idx], 0);
            }
        }
    }
}

POWER_TEST(control_collect_inputs_handler__signal_vr_read, NULL, NULL)
{
    power_latest_calcs_t test_calcs = {};

    // expect we change state to exchange inputs on this signal
    setup_expectations_for_state_change(POWER_CONTROL_STATE_EXCHANGE_INPUTS);

    // handle the runtime assert
    expect_value(__wrap_FpFwAssert, expression, true);
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);

    // call state handler
    call_handler(POWER_CONTROL_STATE_COLLECT_INPUTS, POWER_CTRL_LOOP_SIGNAL_VR_READ, &test_calcs);
}

POWER_TEST(control_collect_inputs_handler__signal_interval_error, NULL, NULL)
{
    setup_expectations_for_state_change(POWER_CONTROL_STATE_ERROR);
    setup_expectations_for_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL, true);
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
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
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
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

    //! power logging into DDR is enabled for distribute handler, so expect it to be called
    will_return_maybe(__wrap_power_timer_get_counter, 0);
    expect_function_call(__wrap_power_log_cores_ts);

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
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
    // call state handler
    call_handler(POWER_CONTROL_STATE_SET_VR_AFTER_PLIMIT, POWER_CTRL_LOOP_SIGNAL_VCPU_PENDING, NULL);
}

POWER_TEST(control_set_vr_after_handler__signal_done, NULL, NULL)
{
    // get internal control loop data to verify current_vcpu
    power_ctrl_loop_detail_t* p_ctrl_loop = power_ctrl_loop_get();
    assert_non_null(p_ctrl_loop);
    // set current_vcpu to a non-zero value to prove it gets overwritten
    p_ctrl_loop->current_vcpu = 1000;

    // expectations on entry
    setup_expectations_for_state_change(POWER_CONTROL_STATE_EXCHANGE_COMPLETION);
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
    // call state handler with NULL event_data (no-VR platform case)
    call_handler(POWER_CONTROL_STATE_SET_VR_AFTER_PLIMIT, POWER_CTRL_LOOP_SIGNAL_VCPU_DONE, NULL);

    // verify current_vcpu is set to 0 when event_data is NULL
    assert_int_equal(p_ctrl_loop->current_vcpu, 0);
}

POWER_TEST(control_set_vr_after_handler__signal_interval_error, NULL, NULL)
{
    setup_expectations_for_state_change(POWER_CONTROL_STATE_ERROR);
    setup_expectations_for_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL, true);
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
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
    // get internal control loop data to verify current_vcpu
    power_ctrl_loop_detail_t* p_ctrl_loop = power_ctrl_loop_get();
    assert_non_null(p_ctrl_loop);
    // set current_vcpu to a non-zero value to prove it gets overwritten
    p_ctrl_loop->current_vcpu = 1000;

    // expectations on entry
    setup_expectations_for_state_change(POWER_CONTROL_STATE_EXCHANGE_COMPLETION);
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
    // call state handler with NULL event_data (no-VR platform case)
    call_handler(POWER_CONTROL_STATE_WAIT_VR_AFTER_PLIMIT, POWER_CTRL_LOOP_SIGNAL_VCPU_DONE, NULL);

    // verify current_vcpu is set to 0 when event_data is NULL
    assert_int_equal(p_ctrl_loop->current_vcpu, 0);
}

POWER_TEST(control_wait_vr_after_handler__signal_interval_error, NULL, NULL)
{
    setup_expectations_for_state_change(POWER_CONTROL_STATE_ERROR);
    setup_expectations_for_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL, true);
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
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
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
    // call state handler
    call_handler(POWER_CONTROL_STATE_SET_VR_BEFORE_PLIMIT, POWER_CTRL_LOOP_SIGNAL_VCPU_PENDING, NULL);
}

POWER_TEST(control_set_vr_before_handler__signal_done, NULL, NULL)
{
    // get internal control loop data to verify current_vcpu
    power_ctrl_loop_detail_t* p_ctrl_loop = power_ctrl_loop_get();
    assert_non_null(p_ctrl_loop);
    // set current_vcpu to a non-zero value to prove it gets overwritten
    p_ctrl_loop->current_vcpu = 1000;

    // expectations on entry
    setup_expectations_for_state_change(POWER_CONTROL_STATE_SET_PLIMIT_AFTER_VR);
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
    // call state handler with NULL event_data (no-VR platform case)
    call_handler(POWER_CONTROL_STATE_SET_VR_BEFORE_PLIMIT, POWER_CTRL_LOOP_SIGNAL_VCPU_DONE, NULL);

    // verify current_vcpu is set to 0 when event_data is NULL
    assert_int_equal(p_ctrl_loop->current_vcpu, 0);
}

POWER_TEST(control_wait_vr_before_handler__signal_done, NULL, NULL)
{
    // get internal control loop data to verify current_vcpu
    power_ctrl_loop_detail_t* p_ctrl_loop = power_ctrl_loop_get();
    assert_non_null(p_ctrl_loop);
    // set current_vcpu to a non-zero value to prove it gets overwritten
    p_ctrl_loop->current_vcpu = 1000;

    // expectations on entry
    setup_expectations_for_state_change(POWER_CONTROL_STATE_SET_PLIMIT_AFTER_VR);
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
    // call state handler with NULL event_data (no-VR platform case)
    call_handler(POWER_CONTROL_STATE_WAIT_VR_BEFORE_PLIMIT, POWER_CTRL_LOOP_SIGNAL_VCPU_DONE, NULL);

    // verify current_vcpu is set to 0 when event_data is NULL
    assert_int_equal(p_ctrl_loop->current_vcpu, 0);
}

// Tests for VCPU_DONE with a valid (non-NULL) voltage value
#define TEST_VCPU_VOLTAGE_MV 995

POWER_TEST(control_set_vr_after_handler__signal_done_with_voltage, NULL, NULL)
{
    // get internal control loop data to verify current_vcpu
    power_ctrl_loop_detail_t* p_ctrl_loop = power_ctrl_loop_get();
    assert_non_null(p_ctrl_loop);
    p_ctrl_loop->current_vcpu = 0;

    // expectations on entry
    setup_expectations_for_state_change(POWER_CONTROL_STATE_EXCHANGE_COMPLETION);
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
    // call state handler with a real voltage value as event_data
    call_handler(POWER_CONTROL_STATE_SET_VR_AFTER_PLIMIT, POWER_CTRL_LOOP_SIGNAL_VCPU_DONE, (const void*)(uintptr_t)TEST_VCPU_VOLTAGE_MV);

    // verify current_vcpu is set to the voltage from event_data
    assert_int_equal(p_ctrl_loop->current_vcpu, TEST_VCPU_VOLTAGE_MV);
}

POWER_TEST(control_wait_vr_after_handler__signal_done_with_voltage, NULL, NULL)
{
    // get internal control loop data to verify current_vcpu
    power_ctrl_loop_detail_t* p_ctrl_loop = power_ctrl_loop_get();
    assert_non_null(p_ctrl_loop);
    p_ctrl_loop->current_vcpu = 0;

    // expectations on entry
    setup_expectations_for_state_change(POWER_CONTROL_STATE_EXCHANGE_COMPLETION);
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
    // call state handler with a real voltage value as event_data
    call_handler(POWER_CONTROL_STATE_WAIT_VR_AFTER_PLIMIT, POWER_CTRL_LOOP_SIGNAL_VCPU_DONE, (const void*)(uintptr_t)TEST_VCPU_VOLTAGE_MV);

    // verify current_vcpu is set to the voltage from event_data
    assert_int_equal(p_ctrl_loop->current_vcpu, TEST_VCPU_VOLTAGE_MV);
}

POWER_TEST(control_set_vr_before_handler__signal_done_with_voltage, NULL, NULL)
{
    // get internal control loop data to verify current_vcpu
    power_ctrl_loop_detail_t* p_ctrl_loop = power_ctrl_loop_get();
    assert_non_null(p_ctrl_loop);
    p_ctrl_loop->current_vcpu = 0;

    // expectations on entry
    setup_expectations_for_state_change(POWER_CONTROL_STATE_SET_PLIMIT_AFTER_VR);
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
    // call state handler with a real voltage value as event_data
    call_handler(POWER_CONTROL_STATE_SET_VR_BEFORE_PLIMIT, POWER_CTRL_LOOP_SIGNAL_VCPU_DONE, (const void*)(uintptr_t)TEST_VCPU_VOLTAGE_MV);

    // verify current_vcpu is set to the voltage from event_data
    assert_int_equal(p_ctrl_loop->current_vcpu, TEST_VCPU_VOLTAGE_MV);
}

POWER_TEST(control_wait_vr_before_handler__signal_done_with_voltage, NULL, NULL)
{
    // get internal control loop data to verify current_vcpu
    power_ctrl_loop_detail_t* p_ctrl_loop = power_ctrl_loop_get();
    assert_non_null(p_ctrl_loop);
    p_ctrl_loop->current_vcpu = 0;

    // expectations on entry
    setup_expectations_for_state_change(POWER_CONTROL_STATE_SET_PLIMIT_AFTER_VR);
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
    // call state handler with a real voltage value as event_data
    call_handler(POWER_CONTROL_STATE_WAIT_VR_BEFORE_PLIMIT,
                 POWER_CTRL_LOOP_SIGNAL_VCPU_DONE,
                 (const void*)(uintptr_t)TEST_VCPU_VOLTAGE_MV);

    // verify current_vcpu is set to the voltage from event_data
    assert_int_equal(p_ctrl_loop->current_vcpu, TEST_VCPU_VOLTAGE_MV);
}

void setup_write_plimits()
{
#define TEST_CORE 5

    // expectations for power_set_plimit
    expect_any(__wrap_power_set_plimit, p_runconfig);
    expect_value(__wrap_power_set_plimit, core, TEST_CORE);
    //! Power logging into DDR is enabled for plimit writes, so expect it to be called
    expect_function_call(__wrap_power_log_core);
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
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
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
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
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
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
    // call state handler
    call_handler(POWER_CONTROL_STATE_SET_PLIMIT_BEFORE_VR, POWER_CTRL_LOOP_SIGNAL_PLIMIT_PENDING, NULL);
}

#define TEST_RUNCONFIG_FOR_EXCHANGE 0x98765432

// exchange input tests
POWER_TEST(control_exchange_inputs_handler__signal_entry, NULL, NULL)
{
    // expectations on entry
    will_return(__wrap_power_runconfig_get, TEST_RUNCONFIG_FOR_EXCHANGE);
    expect_value(__wrap_power_remote_die_exchange_inputs, p_runconfig, TEST_RUNCONFIG_FOR_EXCHANGE);

    // call state handler
    call_handler(POWER_CONTROL_STATE_EXCHANGE_INPUTS, POWER_CTRL_LOOP_SIGNAL_ENTRY, NULL);
}

POWER_TEST(control_exchange_inputs_handler__signal_exchange_inputs_done, NULL, NULL)
{
    power_d2d_data_ex_input_t input_data = {};
    input_data.vrcpu_cap_die0 = 400;

    // expectations for exchanged signal
    setup_expectations_for_state_change(POWER_CONTROL_STATE_DISTRIBUTE_AVAILABLE);

    // expectations on entry
    expect_value(__wrap_FpFwAssert, expression, true);

    //! expectations on exchange inputs
    expect_function_call(__wrap_store_remote_soc_power); //! applicable for both dies
    mock_die_id_1_flag = true;                           // set flag to call real function
    expect_value(__wrap_power_cap_update, new_power_cap, input_data.vrcpu_cap_die0); //! update for die 1
    expect_value(__wrap_power_cap_update, source_is_cli, false);                     //! update for die 1
    will_return(__wrap_power_cap_update, MP_POWER_CAP_PENDING);                      //! update for die 1

    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
    // call state handler
    call_handler(POWER_CONTROL_STATE_EXCHANGE_INPUTS, POWER_CTRL_LOOP_SIGNAL_EXCHANGE_INPUTS, &input_data);
}

POWER_TEST(control_exchange_inputs_handler__signal_interval_error, NULL, NULL)
{
    // expectations on entry
    setup_expectations_for_state_change(POWER_CONTROL_STATE_ERROR);
    setup_expectations_for_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL, true);
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
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
    will_return(__wrap_power_runconfig_get, TEST_RUNCONFIG_FOR_EXCHANGE);
    expect_value(__wrap_power_remote_die_exchange_complete, p_runconfig, TEST_RUNCONFIG_FOR_EXCHANGE);

    // call state handler
    call_handler(POWER_CONTROL_STATE_EXCHANGE_COMPLETION, POWER_CTRL_LOOP_SIGNAL_ENTRY, NULL);
}

POWER_TEST(control_exchange_completion_handler__signal_exchange_complete_done, NULL, NULL)
{
    power_d2d_data_ex_complete_t complete_data = {};
    //! dummy value to indicate pid ctx mismatch between dies
    complete_data.pid_context.available_resources = 0x12345678;
    // expectations on exchange done
    setup_expectations_for_state_change(POWER_CONTROL_STATE_IDLE);

    // expectations on entry
    expect_value(__wrap_FpFwAssert, expression, true);

    //! expectations for exchanged signal done
    expect_function_call(__wrap_pid_get_context); //! applicable for both dies
    mock_die_id_1_flag = true;                    // set flag to call real function
    expect_function_call(__wrap_pid_set_context); //! applicable for only die 1
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
    // call state handler
    call_handler(POWER_CONTROL_STATE_EXCHANGE_COMPLETION, POWER_CTRL_LOOP_SIGNAL_EXCHANGE_COMPLETE, &complete_data);
}

POWER_TEST(control_exchange_completion_handler__signal_interval_error, NULL, NULL)
{
    // expectations on entry
    setup_expectations_for_state_change(POWER_CONTROL_STATE_ERROR);
    setup_expectations_for_retry_fail(POWER_LOOP_RETRY_TYPE_INTERVAL, true);
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);
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

POWER_TEST(control_post_core_init, NULL, NULL)
{
    // expectations on entry
    expect_function_call(__wrap_power_hw_capture_cppc_state);

    // call the function
    power_loops_control_post_core_init();
}

static void create_nominal_count_data(power_ctrl_loop_detail_t* p_ctrl_loop, unsigned core, unsigned nominal, unsigned throt_pri, unsigned boost_pri)
{
    unsigned pnominal = NOMINAL_PLIMIT;
    p_ctrl_loop->cores.core[core].current_boost_priority = boost_pri;
    p_ctrl_loop->cores.core[core].current_throt_priority = throt_pri;

    // nominal has to be pnominal or higher
    nominal = (nominal > pnominal) ? nominal : pnominal;

    p_ctrl_loop->cores.core[core].current_base_pstate = nominal;

    p_ctrl_loop->local.pri_counts.throt_pri_count[throt_pri]++;
    p_ctrl_loop->local.pri_counts.required_nominal_for_throttle[nominal][throt_pri]++;
    p_ctrl_loop->local.pri_counts.required_for_boost[nominal][boost_pri]++;
}

POWER_TEST(control_update_message_cb, NULL, NULL)
{
#define TEST_CORE0           0
#define TEST_CORE1           1
#define TEST_CORE0_THROT_PRI 2
#define TEST_CORE_BOOST_PRI  1
#define TEST_CORE1_THROT_PRI 1
#define TEST_BASE_PSTATE     (NOMINAL_PLIMIT + 2)
#define TEST_DESIRED_PSTATE0 (NOMINAL_PLIMIT - 1)
#define TEST_DESIRED_PSTATE1 (NOMINAL_PLIMIT - 2)

    assert_non_null(s_update_cb);

    power_ctrl_loop_detail_t* p_ctrl_loop = power_ctrl_loop_get();
    assert_non_null(p_ctrl_loop);

    // clear all of the internal priority counts, etc.
    memset(p_ctrl_loop->local.pri_counts.required_nominal_for_throttle,
           0,
           sizeof(p_ctrl_loop->local.pri_counts.required_nominal_for_throttle));
    memset(p_ctrl_loop->local.pri_counts.required_for_boost, 0, sizeof(p_ctrl_loop->local.pri_counts.required_for_boost));
    memset(p_ctrl_loop->local.pri_counts.throt_pri_count, 0, sizeof(p_ctrl_loop->local.pri_counts.throt_pri_count));

    // create some nominal count data
    create_nominal_count_data(p_ctrl_loop, TEST_CORE0, NOMINAL_PLIMIT, 0, 0);
    create_nominal_count_data(p_ctrl_loop, TEST_CORE1, NOMINAL_PLIMIT, 0, 0);

    static power_runconfig_t test_runconfig = {
        .knobs = {.capping_mode = power_capping_mode_t_PER_VM, .power_enable_velocity_boost = true},
        .derived = {.pnominal = NOMINAL_PLIMIT}};

    // expectations on entry
    expect_value_count(__wrap_FpFwAssert, expression, true, 2);
    will_return_count(__wrap_power_runconfig_get, &test_runconfig, 2);

    s_update_cb(TEST_CORE0, TEST_DESIRED_PSTATE0, TEST_BASE_PSTATE, TEST_CORE0_THROT_PRI, TEST_CORE_BOOST_PRI);
    s_update_cb(TEST_CORE1, TEST_DESIRED_PSTATE1, TEST_BASE_PSTATE, TEST_CORE1_THROT_PRI, TEST_CORE_BOOST_PRI);

    assert_int_equal(p_ctrl_loop->cores.core[TEST_CORE0].current_base_pstate, TEST_BASE_PSTATE);
    assert_int_equal(p_ctrl_loop->cores.core[TEST_CORE1].current_base_pstate, TEST_BASE_PSTATE);
    assert_int_equal(p_ctrl_loop->cores.core[TEST_CORE0].current_throt_priority, TEST_CORE0_THROT_PRI);
    assert_int_equal(p_ctrl_loop->cores.core[TEST_CORE1].current_throt_priority, TEST_CORE1_THROT_PRI);
    assert_int_equal(p_ctrl_loop->cores.core[TEST_CORE0].current_boost_priority, TEST_CORE_BOOST_PRI);
    assert_int_equal(p_ctrl_loop->cores.core[TEST_CORE1].current_boost_priority, TEST_CORE_BOOST_PRI);
    assert_int_equal(p_ctrl_loop->cores.core[TEST_CORE0].desired_pstate, TEST_DESIRED_PSTATE0);
    assert_int_equal(p_ctrl_loop->cores.core[TEST_CORE1].desired_pstate, TEST_DESIRED_PSTATE1);

    for (unsigned int pri_idx = 0; pri_idx < VM_PRI_COUNT; pri_idx++)
    {
        for (unsigned int pstate_idx = 0; pstate_idx < NUM_PSTATES; pstate_idx++)
        {
            // we put one core in two different throttling priorities
            if ((pstate_idx == TEST_BASE_PSTATE) && ((pri_idx == TEST_CORE0_THROT_PRI) || (pri_idx == TEST_CORE1_THROT_PRI)))
            {
                assert_int_equal(p_ctrl_loop->local.pri_counts.required_nominal_for_throttle[pstate_idx][pri_idx], 1);
            }
            else
            {
                assert_int_equal(p_ctrl_loop->local.pri_counts.required_nominal_for_throttle[pstate_idx][pri_idx], 0);
            }
            // we put two cores into the same boost priority
            if ((pstate_idx == TEST_BASE_PSTATE) && (pri_idx == TEST_CORE_BOOST_PRI))
            {
                assert_int_equal(p_ctrl_loop->local.pri_counts.required_for_boost[pstate_idx][pri_idx], 2);
            }
            else
            {
                assert_int_equal(p_ctrl_loop->local.pri_counts.required_for_boost[pstate_idx][pri_idx], 0);
            }
        }
    }

    for (unsigned int idx = 0; idx < VM_PRI_COUNT; idx++)
    {
        // again, one core in two different throttling priorities
        if ((idx != TEST_CORE0_THROT_PRI) && (idx != TEST_CORE1_THROT_PRI))
        {
            assert_int_equal(p_ctrl_loop->local.pri_counts.throt_pri_count[idx], 0);
        }
        else
        {
            assert_int_equal(p_ctrl_loop->local.pri_counts.throt_pri_count[idx], 1);
        }
    }
}

static void success_test(bool expected)
{
#define TEST_SELECTED_PSTATE 0x10
#define TEST_DESIRED_PSTATE  0x11

    assert_non_null(s_success_cb);

    power_ctrl_loop_detail_t* p_ctrl_loop = power_ctrl_loop_get();
    assert_non_null(p_ctrl_loop);

    // clear plimits_successful
    memset(&p_ctrl_loop->plimits_successful, 0, sizeof(p_ctrl_loop->plimits_successful));
    // put different values into desired/current pstates
    p_ctrl_loop->cores.core[TEST_CORE].desired_pstate = TEST_DESIRED_PSTATE + 1;
    p_ctrl_loop->cores.core[TEST_CORE].current_plimit = TEST_SELECTED_PSTATE + 1;
    // setup expected plimit
    p_ctrl_loop->cores.core[TEST_CORE].selected_plimit = expected ? TEST_SELECTED_PSTATE : TEST_SELECTED_PSTATE + 1;

    // expectations on entry
    expect_value(__wrap_FpFwAssert, expression, true);

    s_success_cb(TEST_CORE, TEST_DESIRED_PSTATE, TEST_SELECTED_PSTATE);

    assert_int_equal(p_ctrl_loop->cores.core[TEST_CORE].desired_pstate, TEST_DESIRED_PSTATE);
    assert_int_equal(p_ctrl_loop->cores.core[TEST_CORE].current_plimit,
                     expected ? TEST_SELECTED_PSTATE : TEST_SELECTED_PSTATE + 1);
    if (expected)
    {
        assert_true(corebits_is_bit_set(&p_ctrl_loop->plimits_successful, TEST_CORE));
    }
    else
    {
        assert_false(corebits_is_bit_set(&p_ctrl_loop->plimits_successful, TEST_CORE));
    }
}

POWER_TEST(control_success_message_cb, NULL, NULL)
{
    success_test(true);
    success_test(false);
}

POWER_TEST(power_loops_warmstart, NULL, NULL)
{
    setup_expectations_for_state_change(POWER_CONTROL_STATE_WARMSTART_ENTRY);
    will_return(__wrap_power_timer_get_counter, TEST_TIMER_VAL);

    power_loops_warmstart_entry();
}
