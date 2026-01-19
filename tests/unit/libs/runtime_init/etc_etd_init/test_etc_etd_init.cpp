//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_etc_etd_init.cpp
 * ETC and ETD init test.
 * This also tests the Event Trace MTS Client as a part of the ETC tests.
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:etc_etd_init

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <IFpFwEventTracing.h>        // for FPFwETGetController, FPFwETControllerFlushBuffer
#include <crash_dump.h>               // for crash_dump_register_address32
#include <event_trace_collector.h>    // for etc_service_config_t, etc_service...
#include <event_trace_decoder.h>      // for etd_service_config_t, etd_service...
#include <fpfw_init.h>                // for fpfw_init_component_t
#include <idsw_kng.h>                 // for CPU_MCP, CPU_SCP
#include <message_transfer_service.h> // for mts_client_id_t
#include <stddef.h>                   // for NULL
#include <stdnoreturn.h>              // for _Noreturn
#include <tx_api.h>                   // for TX_SUCCESS

/*-- Symbolic Constant Macros (defines) --*/
#define BUGCHECK_MOCK_RETURN   (setjmp(mock_jump_buf))
#define bugcheck_mock_return() BUGCHECK_MOCK_RETURN

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern fpfw_init_component_t _fpfw_component_etc;
extern fpfw_init_component_t _fpfw_component_etd;
extern fpfw_init_component_t _fpfw_component_et_mts_clnt;

static jmp_buf mock_jump_buf;

/*------------- Functions ----------------*/

//
// Mocks
//

_Noreturn void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    FPFW_UNUSED(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);

    longjmp(mock_jump_buf, 1);
}

void __wrap_etc_initialize(etc_service_context_t* p_service, const etc_service_config_t* p_config)
{
    check_expected(p_service);
    check_expected(p_config);
}

void __wrap_etd_initialize(etd_service_context_t* p_service, const etd_service_config_t* p_config)
{
    check_expected(p_service);
    check_expected(p_config);
}

UINT __wrap__txe_queue_create(TX_QUEUE* queue_ptr, CHAR* name_ptr, UINT message_size, VOID* queue_start, ULONG queue_size, UINT queue_control_block_size)
{
    FPFW_UNUSED(queue_ptr);
    FPFW_UNUSED(name_ptr);
    FPFW_UNUSED(message_size);
    FPFW_UNUSED(queue_start);
    FPFW_UNUSED(queue_size);
    FPFW_UNUSED(queue_control_block_size);
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
    return mock_type(UINT);
}

UINT __wrap__txe_thread_create(TX_THREAD* thread_ptr,
                               CHAR* name_ptr,
                               VOID (*thread_entry)(ULONG),
                               ULONG thread_input,
                               VOID* stack_start,
                               ULONG stack_size,
                               UINT priority,
                               UINT preempt_threshold,
                               ULONG time_slice,
                               UINT auto_start)
{
    FPFW_UNUSED(thread_ptr);
    FPFW_UNUSED(name_ptr);
    FPFW_UNUSED(thread_entry);
    FPFW_UNUSED(thread_input);
    FPFW_UNUSED(stack_start);
    FPFW_UNUSED(stack_size);
    FPFW_UNUSED(priority);
    FPFW_UNUSED(preempt_threshold);
    FPFW_UNUSED(time_slice);
    FPFW_UNUSED(auto_start);
    return mock_type(UINT);
}

void __wrap_mts_client_register(mts_client_id_t id, p_mts_client_t client)
{
    check_expected(id);
    FPFW_UNUSED(client);
}

void __wrap_FPFwETControllerFlushBuffer(PFPFW_ET_CONTROLLER pTraceController, const uint64_t bufferTimeThreshold)
{
    FPFW_UNUSED(pTraceController);
    FPFW_UNUSED(bufferTimeThreshold);

    function_called();
}

void __wrap_crash_dump_register_address32(void* p_address, size_t byte_count, uint32_t priority)
{
    FPFW_UNUSED(p_address);
    FPFW_UNUSED(byte_count);
    FPFW_UNUSED(priority);

    function_called();
}

void __wrap_crash_dump_register_pre_dump_callback(void cb(void*), void* ctx, uint32_t dump_type)
{
    FPFW_UNUSED(cb);
    FPFW_UNUSED(ctx);
    FPFW_UNUSED(dump_type);

    cb(ctx);
}

uint8_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(uint8_t);
}

//
// Tests
//

TEST_FUNCTION(test_etc_init_mcp, nullptr, nullptr)
{
    // Set up expectations
    expect_not_value(__wrap_etc_initialize, p_service, NULL);
    expect_not_value(__wrap_etc_initialize, p_config, NULL);

    expect_function_calls(__wrap_crash_dump_register_address32, ETC_SERVICE_CORE_BUFFER_COUNT);
    expect_function_call(__wrap_FPFwETControllerFlushBuffer);

    will_return_always(__wrap_idsw_get_cpu_type, CPU_MCP);

    // Call API under test
    _fpfw_component_etc.init_fn();
}

TEST_FUNCTION(test_etc_init_scp, nullptr, nullptr)
{
    // Set up expectations
    expect_not_value(__wrap_etc_initialize, p_service, NULL);
    expect_not_value(__wrap_etc_initialize, p_config, NULL);

    expect_function_calls(__wrap_crash_dump_register_address32, ETC_SERVICE_CORE_BUFFER_COUNT);
    expect_function_call(__wrap_FPFwETControllerFlushBuffer);

    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);

    // Call API under test
    _fpfw_component_etc.init_fn();
}

TEST_FUNCTION(test_etd_init, nullptr, nullptr)
{
    // Set up expectations
    expect_not_value(__wrap_etd_initialize, p_service, NULL);
    expect_not_value(__wrap_etd_initialize, p_config, NULL);

    // Call API under test
    _fpfw_component_etd.init_fn();
}

TEST_FUNCTION(test_et_mts_clnt_init, nullptr, nullptr)
{
    // Set up expectations
    will_return(__wrap__txe_queue_create, TX_SUCCESS);
    will_return(__wrap__txe_block_pool_create, TX_SUCCESS);
    will_return(__wrap__txe_thread_create, TX_SUCCESS);
    expect_value(__wrap_mts_client_register, id, MTS_CLIENT_ID_EVENT_TRACE);

    // Call API under test
    if (!bugcheck_mock_return())
    {
        _fpfw_component_et_mts_clnt.init_fn();
    }
}
}