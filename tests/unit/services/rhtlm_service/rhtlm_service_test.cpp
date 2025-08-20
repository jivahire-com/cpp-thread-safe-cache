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
#include <rhtlm_service.h>
#include <stdint.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
extern void rhtlm_telemtry_function(rhtlm_service_ctx_t* context);

/*-- Declarations (Statics and globals) --*/
static rhtlm_service_ctx_t s_test_context;
// static ULONG thread_input;
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

uint8_t __wrap_config_get_rh_tlm_cca_mode(void)
{
    return mock_type(uint8_t);
}

uint32_t __wrap_config_get_rm_tm_rec_mask(void)
{
    return mock_type(uint32_t);
}

uint8_t __wrap_config_get_erhm_en(void)
{
    return mock_type(uint8_t);
}

uint8_t __wrap_config_get_intu_init_en(void)
{
    return mock_type(uint8_t);
}

uint64_t __wrap_config_get_rh_tlm_service_period_ms(void)
{
    return mock_type(uint64_t);
}

uint8_t __wrap_idsw_get_die_id(void)
{
    return mock_type(uint8_t);
}

int __wrap_ddrss_set_rhm_telemetry_record_mask(void* ptr)
{
    FPFW_UNUSED(ptr);
    return mock_type(int);
}

int __wrap_ddrss_get_telemetry_record(uint32_t mc, uint8_t telemetry_type, void* telemetry_buf, int telemetry_buf_len)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(telemetry_type);
    FPFW_UNUSED(telemetry_buf);
    FPFW_UNUSED(telemetry_buf_len);

    return mock_type(int);
}

void __wrap_prod_ddrss_convert_rh_rec_to_rh_cper(uint32_t mc, uint8_t sample_type, void* p_rh_tel, void* p_rh_cper)
{
    FPFW_UNUSED(mc);
    FPFW_UNUSED(sample_type);
    FPFW_UNUSED(p_rh_tel);
    FPFW_UNUSED(p_rh_cper);
}

void __wrap_hm_submit_cper(uint16_t error_domain_idx, uint8_t err_severity, void* err_record_section, uint32_t err_record_section_size)
{
    FPFW_UNUSED(error_domain_idx);
    FPFW_UNUSED(err_severity);
    FPFW_UNUSED(err_record_section);
    FPFW_UNUSED(err_record_section_size);
}

UINT __wrap__txe_thread_create(TX_THREAD* thread_ptr,
                               CHAR* name_ptr,
                               VOID (*entry_function)(ULONG entry_input),
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

UINT __wrap__tx_thread_sleep(ULONG ticks)
{
    FPFW_UNUSED(ticks);
    return 0;
}

//
// Tests
//

TEST_FUNCTION(test_rhtlm_thread_init_success, nullptr, nullptr)
{
    char* name = (char*)"RHTLM Thread";
    will_return_always(__wrap_config_get_rh_tlm_service_period_ms, 1000); // 1 second
    will_return(__wrap_config_get_rh_tlm_cca_mode, 0);
    will_return(__wrap_config_get_erhm_en, 1);
    will_return(__wrap_config_get_erhm_en, 1);

    expect_value(__wrap__txe_thread_create, thread_ptr, &s_test_context.s_rhtlm_thread);
    expect_value(__wrap__txe_thread_create, name_ptr, name);
    expect_value(__wrap__txe_thread_create, stack_start, s_test_context.s_stack_pool_memory);
    expect_value(__wrap__txe_thread_create, stack_size, sizeof(s_test_context.s_stack_pool_memory));
    expect_value(__wrap__txe_thread_create, priority, (TX_MAX_PRIORITIES - 2));
    will_return(__wrap__txe_thread_create, TX_SUCCESS);
    rhtlm_thread_init(&s_test_context);

    assert_int_equal(s_test_context.sleep_period, 100);
    assert_true(s_test_context.active);
}

TEST_FUNCTION(test_rhtlm_thread_init_service_disabled, nullptr, nullptr)
{
    will_return_always(__wrap_config_get_rh_tlm_service_period_ms, 1000);
    will_return(__wrap_config_get_erhm_en, 0);
    will_return(__wrap_config_get_erhm_en, 0);

    rhtlm_thread_init(&s_test_context);
    assert_int_equal(s_test_context.sleep_period, 0);
    assert_false(s_test_context.active);
}

TEST_FUNCTION(test_rhtlm_telemtry_function_die0, nullptr, nullptr)
{
    will_return_always(__wrap_config_get_rh_tlm_service_period_ms, 1000);
    will_return_always(__wrap_idsw_get_die_id, 0);
    will_return_always(__wrap_ddrss_get_telemetry_record, -2);
    will_return_always(__wrap_config_get_intu_init_en, 0);

    rhtlm_telemtry_function(&s_test_context);
}

TEST_FUNCTION(test_rhtlm_telemtry_function_die1, nullptr, nullptr)
{
    will_return_always(__wrap_config_get_rh_tlm_service_period_ms, 1000);
    will_return_always(__wrap_idsw_get_die_id, 1);
    will_return_always(__wrap_ddrss_get_telemetry_record, 0);
    will_return_always(__wrap_config_get_intu_init_en, 0);

    rhtlm_telemtry_function(&s_test_context);
}

TEST_FUNCTION(test_rhtlm_telemtry_function_dint_enable, nullptr, nullptr)
{
    will_return_always(__wrap_config_get_rh_tlm_service_period_ms, 1000);
    will_return_always(__wrap_idsw_get_die_id, 0);
    will_return_always(__wrap_config_get_intu_init_en, 1);

    rhtlm_telemtry_function(&s_test_context);
}

TEST_FUNCTION(test_rhtlm_telemtry_function_dummy, nullptr, nullptr)
{
    will_return_always(__wrap_config_get_rh_tlm_service_period_ms, 0);

    rhtlm_telemtry_function(&s_test_context);
}

TEST_FUNCTION(test_rhtlm_thread_init_service_disabled1, nullptr, nullptr)
{
    will_return_always(__wrap_config_get_rh_tlm_service_period_ms, 0);
    will_return(__wrap_config_get_erhm_en, 0);

    rhtlm_thread_init(&s_test_context);
    assert_int_equal(s_test_context.sleep_period, 0);
    assert_false(s_test_context.active);
}

TEST_FUNCTION(test_rhtlm_thread_init_cca_success, nullptr, nullptr)
{
    char* name = (char*)"RHTLM Thread";
    will_return(__wrap_config_get_erhm_en, 1);
    will_return_always(__wrap_config_get_rh_tlm_service_period_ms, 1000); // 1 second
    will_return(__wrap_config_get_erhm_en, 1);

    will_return(__wrap_config_get_rh_tlm_cca_mode, 1);
    will_return(__wrap_config_get_rm_tm_rec_mask, 0xFFFFFFFF);

    will_return(__wrap_ddrss_set_rhm_telemetry_record_mask, 0);

    expect_value(__wrap__txe_thread_create, thread_ptr, &s_test_context.s_rhtlm_thread);
    expect_value(__wrap__txe_thread_create, name_ptr, name);
    expect_value(__wrap__txe_thread_create, stack_start, s_test_context.s_stack_pool_memory);
    expect_value(__wrap__txe_thread_create, stack_size, sizeof(s_test_context.s_stack_pool_memory));
    expect_value(__wrap__txe_thread_create, priority, (TX_MAX_PRIORITIES - 2));
    will_return(__wrap__txe_thread_create, TX_SUCCESS);
    rhtlm_thread_init(&s_test_context);
    assert_int_equal(s_test_context.sleep_period, 100);
    assert_true(s_test_context.active);
}

TEST_FUNCTION(test_rhtlm_thread_init_cca_failed, nullptr, nullptr)
{
    char* name = (char*)"RHTLM Thread";
    will_return(__wrap_config_get_erhm_en, 1);
    will_return_always(__wrap_config_get_rh_tlm_service_period_ms, 1000); // 1 second
    will_return(__wrap_config_get_erhm_en, 1);

    will_return(__wrap_config_get_rh_tlm_cca_mode, 1);
    will_return(__wrap_config_get_rm_tm_rec_mask, 0xFFFFFFFF);

    will_return(__wrap_ddrss_set_rhm_telemetry_record_mask, -2);

    expect_value(__wrap__txe_thread_create, thread_ptr, &s_test_context.s_rhtlm_thread);
    expect_value(__wrap__txe_thread_create, name_ptr, name);
    expect_value(__wrap__txe_thread_create, stack_start, s_test_context.s_stack_pool_memory);
    expect_value(__wrap__txe_thread_create, stack_size, sizeof(s_test_context.s_stack_pool_memory));
    expect_value(__wrap__txe_thread_create, priority, (TX_MAX_PRIORITIES - 2));
    will_return(__wrap__txe_thread_create, TX_SUCCESS);
    rhtlm_thread_init(&s_test_context);
    assert_int_equal(s_test_context.sleep_period, 100);
    assert_true(s_test_context.active);
}

TEST_FUNCTION(test_rhtlm_thread_init_failure, nullptr, nullptr)
{
    will_return_always(__wrap_config_get_rh_tlm_service_period_ms, 1000); // 1 second
    will_return(__wrap_config_get_erhm_en, 1);
    will_return(__wrap_config_get_erhm_en, 1);

    expect_value(__wrap__txe_thread_create, thread_ptr, &s_test_context.s_rhtlm_thread);
    expect_value(__wrap__txe_thread_create, name_ptr, "RHTLM Thread");
    expect_value(__wrap__txe_thread_create, stack_start, s_test_context.s_stack_pool_memory);
    expect_value(__wrap__txe_thread_create, stack_size, sizeof(s_test_context.s_stack_pool_memory));
    expect_value(__wrap__txe_thread_create, priority, (TX_MAX_PRIORITIES - 2));
    will_return(__wrap__txe_thread_create, TX_NOT_AVAILABLE);
    expect_value(__wrap_FPFwErrorRaise, error, TX_NOT_AVAILABLE);

    rhtlm_thread_init(&s_test_context);
}
}
