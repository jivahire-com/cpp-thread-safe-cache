//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_rtosmon_service.cpp
 * RTOS monitor service unit tests
 */

/*-------------------------------- Includes ---------------------------------*/
#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS
#include <fpfw_status.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <rtosmon_service.h>
#include <tx_api.h>
#include <tx_execution_profile.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

#define TEST_GTIMER_FREQUENCY (100000000U)

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

// rtosmon_lib_init is not static, so we can call it directly
fpfw_status_t rtosmon_lib_init(void);

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Mocks ---------------------------------------*/

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}

KNG_DIE_ID __wrap_idsw_get_die_id(void)
{
    return mock_type(KNG_DIE_ID);
}

fpfw_status_t __wrap_rtosmon_init(void* p_rtosmon_config, void* p_rtosmon_histo_data)
{
    check_expected_ptr(p_rtosmon_config);
    check_expected_ptr(p_rtosmon_histo_data);
    return mock_type(fpfw_status_t);
}

uint32_t __wrap_gtimer_prodfw_get_frequency(void)
{
    return mock_type(uint32_t);
}

uint64_t __wrap_gtimer_prodfw_get_counter(void)
{
    return mock_type(uint64_t);
}

int32_t __wrap__txe_thread_create(TX_THREAD* thread_ptr,
                                  CHAR* name_ptr,
                                  VOID (*entry_function)(ULONG entry_input),
                                  ULONG entry_input,
                                  VOID* stack_start,
                                  ULONG stack_size,
                                  UINT priority,
                                  UINT preempt_threshold,
                                  ULONG time_slice,
                                  UINT auto_start)
{
    check_expected_ptr(thread_ptr);
    check_expected_ptr(name_ptr);
    check_expected_ptr(entry_function);
    check_expected(entry_input);
    check_expected_ptr(stack_start);
    check_expected(stack_size);
    check_expected(priority);
    check_expected(preempt_threshold);
    check_expected(time_slice);
    check_expected(auto_start);
    return mock_type(int32_t);
}

/*----------------------------- Global Functions ----------------------------*/

} // extern "C" {

//
// Setup and teardown
//

static int test_setup(void** state)
{
    FPFW_UNUSED(state);
    return 0;
}

static int test_teardown(void** state)
{
    FPFW_UNUSED(state);
    return 0;
}

//
// Helper to set up expectations for rtosmon_init (called via rtosmon_lib_init)
//
static void expect_rtosmon_lib_init(idsw_cpu_type_t cpu_type, KNG_DIE_ID die_id, fpfw_status_t rtosmon_init_result)
{
    will_return(__wrap_idsw_get_cpu_type, cpu_type);
    will_return(__wrap_idsw_get_die_id, die_id);
    // gtimer_prodfw_get_frequency is called twice: once for tx_execution_time_source_freq
    // and once for monitoring_time_source_freq
    will_return(__wrap_gtimer_prodfw_get_frequency, TEST_GTIMER_FREQUENCY);
    will_return(__wrap_gtimer_prodfw_get_frequency, TEST_GTIMER_FREQUENCY);
    expect_any(__wrap_rtosmon_init, p_rtosmon_config);
    expect_any(__wrap_rtosmon_init, p_rtosmon_histo_data);
    will_return(__wrap_rtosmon_init, rtosmon_init_result);
}

//
// Helper to set up expectations for tx_thread_create
//
static void expect_thread_create(UINT result)
{
    expect_any(__wrap__txe_thread_create, thread_ptr);
    expect_any(__wrap__txe_thread_create, name_ptr);
    expect_any(__wrap__txe_thread_create, entry_function);
    expect_any(__wrap__txe_thread_create, entry_input);
    expect_any(__wrap__txe_thread_create, stack_start);
    expect_any(__wrap__txe_thread_create, stack_size);
    expect_any(__wrap__txe_thread_create, priority);
    expect_any(__wrap__txe_thread_create, preempt_threshold);
    expect_any(__wrap__txe_thread_create, time_slice);
    expect_any(__wrap__txe_thread_create, auto_start);
    will_return(__wrap__txe_thread_create, result);
}

//
// Tests for rtosmon_lib_init
//

// Happy path: SCP on die 0
TEST_FUNCTION(test_rtosmon_lib_init_scp_die0_success, test_setup, test_teardown)
{
    expect_rtosmon_lib_init(CPU_SCP, DIE_0, FPFW_INIT_STATUS_SUCCESS);

    fpfw_status_t status = rtosmon_lib_init();
    assert_int_equal(status, FPFW_INIT_STATUS_SUCCESS);
}

// Happy path: MCP on die 0
TEST_FUNCTION(test_rtosmon_lib_init_mcp_die0_success, test_setup, test_teardown)
{
    expect_rtosmon_lib_init(CPU_MCP, DIE_0, FPFW_INIT_STATUS_SUCCESS);

    fpfw_status_t status = rtosmon_lib_init();
    assert_int_equal(status, FPFW_INIT_STATUS_SUCCESS);
}

// Happy path: SCP on die 1
TEST_FUNCTION(test_rtosmon_lib_init_scp_die1_success, test_setup, test_teardown)
{
    expect_rtosmon_lib_init(CPU_SCP, DIE_1, FPFW_INIT_STATUS_SUCCESS);

    fpfw_status_t status = rtosmon_lib_init();
    assert_int_equal(status, FPFW_INIT_STATUS_SUCCESS);
}

// Happy path: MCP on die 1
TEST_FUNCTION(test_rtosmon_lib_init_mcp_die1_success, test_setup, test_teardown)
{
    expect_rtosmon_lib_init(CPU_MCP, DIE_1, FPFW_INIT_STATUS_SUCCESS);

    fpfw_status_t status = rtosmon_lib_init();
    assert_int_equal(status, FPFW_INIT_STATUS_SUCCESS);
}

// Edge case: Unknown die id falls through to "UnknownCore"
TEST_FUNCTION(test_rtosmon_lib_init_unknown_die_success, test_setup, test_teardown)
{
    // die_id = 0xFF is neither DIE_0 nor DIE_1
    expect_rtosmon_lib_init(CPU_SCP, (KNG_DIE_ID)0xFF, FPFW_INIT_STATUS_SUCCESS);

    fpfw_status_t status = rtosmon_lib_init();
    assert_int_equal(status, FPFW_INIT_STATUS_SUCCESS);
}

// Edge case: Known die but unrecognized cpu type (e.g. CPU_AP) on die 0
TEST_FUNCTION(test_rtosmon_lib_init_unknown_cpu_die0_success, test_setup, test_teardown)
{
    expect_rtosmon_lib_init(CPU_AP, DIE_0, FPFW_INIT_STATUS_SUCCESS);

    fpfw_status_t status = rtosmon_lib_init();
    assert_int_equal(status, FPFW_INIT_STATUS_SUCCESS);
}

// Failure: rtosmon_init returns failure
TEST_FUNCTION(test_rtosmon_lib_init_rtosmon_init_fails, test_setup, test_teardown)
{
    expect_rtosmon_lib_init(CPU_MCP, DIE_0, FPFW_STATUS_FAIL);

    fpfw_status_t status = rtosmon_lib_init();
    assert_int_equal(status, FPFW_STATUS_FAIL);
}

//
// Tests for rtosmon_service_init
//

// Happy path: lib init succeeds, thread create succeeds
TEST_FUNCTION(test_rtosmon_service_init_success, test_setup, test_teardown)
{
    expect_rtosmon_lib_init(CPU_MCP, DIE_0, FPFW_INIT_STATUS_SUCCESS);
    expect_thread_create(TX_SUCCESS);

    fpfw_status_t status = rtosmon_service_init();
    assert_int_equal(status, FPFW_INIT_STATUS_SUCCESS);
}

// Failure: lib init fails, thread create should not be called
TEST_FUNCTION(test_rtosmon_service_init_lib_init_fails, test_setup, test_teardown)
{
    expect_rtosmon_lib_init(CPU_SCP, DIE_1, FPFW_STATUS_FAIL);

    // tx_thread_create should NOT be called since rtosmon_lib_init failed
    fpfw_status_t status = rtosmon_service_init();
    assert_int_equal(status, FPFW_STATUS_FAIL);
}

// Failure: lib init succeeds but thread create fails
TEST_FUNCTION(test_rtosmon_service_init_thread_create_fails, test_setup, test_teardown)
{
    expect_rtosmon_lib_init(CPU_MCP, DIE_1, FPFW_INIT_STATUS_SUCCESS);
    expect_thread_create(TX_NO_MEMORY);

    fpfw_status_t status = rtosmon_service_init();
    assert_int_equal(status, FPFW_STATUS_FAIL);
}

// Happy path: SCP die 0 via service_init
TEST_FUNCTION(test_rtosmon_service_init_scp_die0_success, test_setup, test_teardown)
{
    expect_rtosmon_lib_init(CPU_SCP, DIE_0, FPFW_INIT_STATUS_SUCCESS);
    expect_thread_create(TX_SUCCESS);

    fpfw_status_t status = rtosmon_service_init();
    assert_int_equal(status, FPFW_INIT_STATUS_SUCCESS);
}
