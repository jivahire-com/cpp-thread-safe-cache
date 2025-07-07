/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 */

/**
 * @file bitrot_service_test.cpp
 *
 * Provides the mocked version of tx api.
 */

/*------------- Includes -----------------*/

#include <CMockaWrapper.h>
#include <cstddef>
#include <cstdint>

extern "C" {
#include <ErrorHandler.h>
#include <FpFwCli.h>
#include <FpFwUtils.h>
#include <bitrot_service.h>
#include <stdint.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
extern void bitrot_walk_function(mscp_bitrotservice_ctx_t* context);

/*-- Declarations (Statics and globals) --*/

static mscp_mem_table_t memTable;
static mscp_bitrotservice_ctx_t s_test_context;
static mscp_mem_type dummy_mem[16];

/*------------- Functions ----------------*/

//
// Mocks

void __wrap_FpFwCliPrint(const char* format, ...)
{
    FPFW_UNUSED(format);
}

void __wrap_FPFwErrorRaise(uint32_t error, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    FPFW_UNUSED(a);
    FPFW_UNUSED(b);
    FPFW_UNUSED(c);
    FPFW_UNUSED(d);
    check_expected(error);
}

uint64_t __wrap_config_get_bitrot_TCM_delay(void)
{
    return mock_type(uint64_t);
}

UINT _txe_thread_create(TX_THREAD* thread_ptr,
                        CHAR* name_ptr,
                        VOID (*entry_function)(ULONG id),
                        ULONG entry_input,
                        VOID* stack_start,
                        ULONG stack_size,
                        UINT priority,
                        UINT preempt_threshold,
                        ULONG time_slice,
                        UINT auto_start,
                        UINT thread_control_block_size)
{
    check_expected_ptr(thread_ptr);
    check_expected_ptr(name_ptr);
    check_expected_ptr(stack_start);
    check_expected(stack_size);
    check_expected(priority);
    FPFW_UNUSED(entry_function);
    FPFW_UNUSED(preempt_threshold);
    FPFW_UNUSED(time_slice);
    FPFW_UNUSED(auto_start);
    FPFW_UNUSED(entry_input);
    FPFW_UNUSED(thread_control_block_size);
    return mock_type(UINT);
}

UINT tx_thread_sleep(ULONG ticks)
{
    check_expected(ticks);
    return mock_type(UINT);
}

//
// Tests
//

TEST_FUNCTION(test_bitrot_thread_init_success, nullptr, nullptr)
{
    char* name = (char*)"Bitrot Thread";
    will_return(__wrap_config_get_bitrot_TCM_delay, 1000); // 1 second

    expect_value(_txe_thread_create, thread_ptr, &s_test_context.s_bitrot_thread);
    expect_value(_txe_thread_create, name_ptr, name);
    expect_value(_txe_thread_create, stack_start, s_test_context.s_stack_pool_memory);
    expect_value(_txe_thread_create, stack_size, sizeof(s_test_context.s_stack_pool_memory));
    expect_value(_txe_thread_create, priority, (TX_MAX_PRIORITIES - 2));
    will_return(_txe_thread_create, TX_SUCCESS);

    bitrot_thread_init(&s_test_context);
    assert_int_equal(s_test_context.sleep_period, 100);
}

TEST_FUNCTION(test_bitrot_thread_init_failure, nullptr, nullptr)
{
    will_return(__wrap_config_get_bitrot_TCM_delay, 1000);

    expect_value(_txe_thread_create, thread_ptr, &s_test_context.s_bitrot_thread);
    expect_value(_txe_thread_create, name_ptr, "Bitrot Thread");
    expect_value(_txe_thread_create, stack_start, s_test_context.s_stack_pool_memory);
    expect_value(_txe_thread_create, stack_size, sizeof(s_test_context.s_stack_pool_memory));
    expect_value(_txe_thread_create, priority, (TX_MAX_PRIORITIES - 2));
    will_return(_txe_thread_create, TX_NOT_AVAILABLE);
    expect_value(__wrap_FPFwErrorRaise, error, TX_NOT_AVAILABLE);

    bitrot_thread_init(&s_test_context);
}

TEST_FUNCTION(test_bitrot_walk_function_zero_delay, nullptr, nullptr)
{

    s_test_context.mem_table = &memTable;
    s_test_context.mem_table_len = 0;
    memTable.len = 0;
    memTable.start_addr = 0;
    memTable.p_mem_type_name = (char*)"DummyMem";

    will_return(__wrap_config_get_bitrot_TCM_delay, 0);
    expect_value(_tx_thread_sleep, ticks, 100);
    will_return(_tx_thread_sleep, 0);

    bitrot_walk_function(&s_test_context);

    assert_int_equal(s_test_context.sleep_period, 0);
}

TEST_FUNCTION(test_bitrot_walk_function_with_memory, nullptr, nullptr)
{

    s_test_context.mem_table = &memTable;
    s_test_context.mem_table_len = 1;

    memTable.len = sizeof(dummy_mem);
    memTable.start_addr = dummy_mem;
    memTable.p_mem_type_name = (char*)"DummyMem";

    will_return(__wrap_config_get_bitrot_TCM_delay, 10000);
    expect_value(_tx_thread_sleep, ticks, 1000);
    will_return(_tx_thread_sleep, 0);

    bitrot_walk_function(&s_test_context);

    assert_int_equal(s_test_context.sleep_period, 1000);
}
}
