//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_crash_dump.cpp
 * Crash dump unit tests
 */

/*------------- Includes -----------------*/
#include "test_crash_dump_expect.h" // for __wrap_CdRegisterAddress32, __wrap_CdRegisterCallback...

#include <CMockaWrapper.h>            // for check_expected_ptr, expect_fun...
#include <cstdint>                    // for uint32_t, uint64_t
#include <modules/CdDumpDescriptor.h> // for _FPFwCdDumpPriority
#include <stddef.h>                   // for NULL
#include <tx_api.h>                   // for TX_THREAD, ULONG

extern "C" {
#include <../src/crash_dump_overrides.h> // for inMemoryOverride
#include <../src/crash_dump_payload.h>   // for CD_THREADX_DATA, crash_dump_capture_threadx
#include <crash_dump.h>                  // for crash_dump_init
#include <crash_dump_memory.h>           // for CRASH_DUMP_FULL_SIZE

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
extern int init_crash_dump_context(void** pContext);

/*-- Declarations (Statics and globals) --*/
extern uint8_t __stack_start__;
extern uint8_t __stack_end__;
extern uint8_t _build_id_msdata_start;

extern void* _tx_thread_system_stack_ptr;
extern TX_THREAD* _tx_thread_current_ptr;
extern TX_THREAD* _tx_thread_execute_ptr;
extern TX_THREAD* _tx_thread_created_ptr;
extern ULONG _tx_thread_created_count;
extern ULONG _tx_thread_system_state;

/*------------- Functions ----------------*/
//
// Expectations
//
void set_expectations_init_mem_pool()
{
    expect_function_call(__wrap_FPFwCDInitMemoryPool);
    expect_any(__wrap_FPFwCDInitMemoryPool, baseAddr);
    expect_any(__wrap_FPFwCDInitMemoryPool, poolSize);

    expect_function_call(__wrap_FPFwCDMemPoolOverrideCacheFlush);
    expect_value(__wrap_FPFwCDMemPoolOverrideCacheFlush, cacheFlush, &cacheFlushOverride);

    expect_function_call(__wrap_FPFwCDMemPoolOverrideCacheInvalidate);
    expect_value(__wrap_FPFwCDMemPoolOverrideCacheInvalidate, cacheInvalidate, &cacheInvalidateOverride);
}

void set_expectations_init_dump_desc()
{
    expect_function_call(__wrap__txe_mutex_create);
    expect_string(__wrap__txe_mutex_create, name_ptr, "cd desc mutex");

    expect_function_call(__wrap_FPFwCDInitDumpDescriptor);
    expect_any(__wrap_FPFwCDInitDumpDescriptor, arraySize);

    expect_function_call(__wrap_FPFwCDDumpDescriptorSetMutexCtx);

    expect_function_call(__wrap_FPFwCDDumpDescriptorOverrideMutexLock);
    expect_value(__wrap_FPFwCDDumpDescriptorOverrideMutexLock, mutexLock, &mutexLockOverride);

    expect_function_call(__wrap_FPFwCDDumpDescriptorOverrideMutexUnlock);
    expect_value(__wrap_FPFwCDDumpDescriptorOverrideMutexUnlock, mutexUnlock, &mutexUnlockOverride);
}

void set_expectations_init_dump_file()
{
    expect_function_call(__wrap_FPFwCDInitDumpFile);

    expect_function_call(__wrap_FPFwCDDumpFileOverrideInValidMemory);
    expect_value(__wrap_FPFwCDDumpFileOverrideInValidMemory, inValidMemory, &inMemoryOverride);

    expect_function_call(__wrap_FPFwCDDumpFileOverrideInValidCsrMemory);
    expect_value(__wrap_FPFwCDDumpFileOverrideInValidCsrMemory, inValidCsrMemory, &inMemoryOverride);

    expect_function_call(__wrap_FPFwCDDumpFileOverrideInValidGlobalMemory);
    expect_value(__wrap_FPFwCDDumpFileOverrideInValidGlobalMemory, inValidGlobalMemory, &inGlobalMemoryOverride);
}

void set_expectations_init_dump_manager()
{
    expect_function_call(__wrap_FPFwCDInitDumpManager);
    expect_value(__wrap_FPFwCDInitDumpManager, totalDumpSize, CRASH_DUMP_FULL_SIZE);

    expect_function_call(__wrap_FPFwCDOverridePrintf);
    expect_value(__wrap_FPFwCDOverridePrintf, printOverride, &crash_dump_printf);

    expect_function_call(__wrap_FPFwCDDumpManagerSetPreDumpCallback);
    expect_value(__wrap_FPFwCDDumpManagerSetPreDumpCallback, preDumpCallback, &preDumpCallbackOverride);
    expect_value(__wrap_FPFwCDDumpManagerSetPreDumpCallback, preDumpCtx, NULL);

    expect_function_call(__wrap_FPFwCDDumpManagerSetPostDumpCallback);
    expect_value(__wrap_FPFwCDDumpManagerSetPostDumpCallback, postDumpCallback, &postDumpCallbackOverride);
    expect_value(__wrap_FPFwCDDumpManagerSetPostDumpCallback, postDumpCtx, NULL);

    expect_function_call(__wrap_FPFwCDDumpManagerOverrideGetCurTime);
    expect_value(__wrap_FPFwCDDumpManagerOverrideGetCurTime, getCurTime, &getCurTimeDefault);
}

void set_expectations_crash_dump_register_core_registers()
{
    expect_function_call(__wrap_CdRegisterRegisterSet);
    expect_value(__wrap_CdRegisterRegisterSet, address, &g_core_crash_context);
    expect_value(__wrap_CdRegisterRegisterSet, regIndex, 0);  // CORE_BUILTIN_REG_INDEX
    expect_value(__wrap_CdRegisterRegisterSet, regCount, 16); // CORE_BUILTIN_REG_COUNT
    expect_value(__wrap_CdRegisterRegisterSet, priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);
}

void set_expectations_crash_dump_register_core_stack()
{
    expect_function_call(__wrap_CdRegisterAddress32);
    expect_value(__wrap_CdRegisterAddress32, address, &__stack_start__);
    expect_value(__wrap_CdRegisterAddress32, size, &__stack_end__ - &__stack_start__);
    expect_value(__wrap_CdRegisterAddress32, priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);
}

void set_expectations_crash_dump_register_standard_info()
{
    expect_function_call(__wrap_CdRegisterAddress32);
    expect_not_value(__wrap_CdRegisterAddress32, address, NULL);
    expect_value(__wrap_CdRegisterAddress32, size, sizeof(CD_MSFT_VERSION_INFO));
    expect_value(__wrap_CdRegisterAddress32, priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);
}

void set_expectations_crash_dump_register_threadx()
{
    expect_function_call(__wrap_CdRegisterCallback); // crash_dump_capture_threadx
    expect_not_value(__wrap_CdRegisterCallback, callback, NULL);
    expect_value(__wrap_CdRegisterCallback, context, NULL);
    expect_value(__wrap_CdRegisterCallback, priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);
}

void set_expectations_crash_dump_register_address32(void* address_exp, uint32_t size_exp, FPFwCdDumpPriority priority_exp)
{
    expect_function_call(__wrap_CdRegisterAddress32);
    expect_value(__wrap_CdRegisterAddress32, address, address_exp);
    expect_value(__wrap_CdRegisterAddress32, size, size_exp);
    expect_value(__wrap_CdRegisterAddress32, priority, priority_exp);
}

void set_expectations_crash_dump_register_address32_no_address(uint32_t size_exp, FPFwCdDumpPriority priority_exp)
{
    expect_function_call(__wrap_CdRegisterAddress32);
    expect_not_value(__wrap_CdRegisterAddress32, address, NULL);
    expect_value(__wrap_CdRegisterAddress32, size, size_exp);
    expect_value(__wrap_CdRegisterAddress32, priority, priority_exp);
}

void set_expectations_crash_dump_register_address32_ptr_array(FPFwCdDumpPriority priority_exp,
                                                              uint32_t minimumChunkSize_exp,
                                                              uint32_t maximumRegistrationCount_exp,
                                                              void** pointerArray_exp,
                                                              uint32_t pointerArrayCount_exp)
{
    expect_function_call(__wrap_CdRegisterAddress32PointerArray);
    expect_value(__wrap_CdRegisterAddress32PointerArray, priority, priority_exp);
    expect_value(__wrap_CdRegisterAddress32PointerArray, minimumChunkSize, minimumChunkSize_exp);
    expect_value(__wrap_CdRegisterAddress32PointerArray, maximumRegistrationCount, maximumRegistrationCount_exp);
    expect_value(__wrap_CdRegisterAddress32PointerArray, pointerArray, pointerArray_exp);
    expect_value(__wrap_CdRegisterAddress32PointerArray, pointerArrayCount, pointerArrayCount_exp);
}
//
// Tests
//
/**
 * @brief crash dump initialization test
 *
 */
TEST_FUNCTION(test_crash_dump_init, init_crash_dump_context, nullptr)
{
    // Set up expectations
    set_expectations_gpio_set_output();

    // init_mem_pool()
    set_expectations_init_mem_pool();

    // init_dump_desc()
    set_expectations_init_dump_desc();

    // init_dump_file()
    set_expectations_init_dump_file();

    // init_dump_manager()
    set_expectations_init_dump_manager();

    // crash_dump_register_core_registers()
    set_expectations_crash_dump_register_core_registers();

    // crash_dump_register_default_registers()
    set_expectations_crash_dump_register_default_registers();

    // crash_dump_register_core_stack()
    set_expectations_crash_dump_register_core_stack();

    // crash_dump_register_standard_info()
    set_expectations_crash_dump_register_standard_info();

    // crash_dump_register_threadx()
    set_expectations_crash_dump_register_threadx();

    // Call API under test
    crash_dump_init();
}

TEST_FUNCTION(test_crash_dump_capture_threadx, NULL, NULL)
{
    // Set up test data
    uint8_t stack[1024] = {};
    TX_THREAD threads[2] = {};

    threads[0].tx_thread_created_next = &threads[1];
    threads[0].tx_thread_stack_start = stack;
    threads[0].tx_thread_stack_end = stack + 500;
    threads[0].tx_thread_stack_size = 512;
    threads[0].tx_thread_name = (CHAR*)"thread0";

    threads[1].tx_thread_created_next = NULL;
    threads[1].tx_thread_stack_start = stack + 512;
    threads[1].tx_thread_stack_end = stack + 1012;
    threads[1].tx_thread_stack_size = 512;
    threads[1].tx_thread_name = (CHAR*)"thread1";

    _tx_thread_system_stack_ptr = stack;
    _tx_thread_current_ptr = &threads[1];
    _tx_thread_execute_ptr = &threads[0];
    _tx_thread_created_ptr = &threads[0];
    _tx_thread_created_count = 2;
    _tx_thread_system_state = 0xF0F0F0F0;

    // Set up expectations
    // cdThreadXData
    set_expectations_crash_dump_register_address32_no_address(sizeof(CD_THREADX_DATA), FPFW_CD_DUMP_PRIORITY_CRITICAL);

    // _tx_thread_system_stack_ptr
    set_expectations_crash_dump_register_address32(&_tx_thread_system_stack_ptr, sizeof(void*), FPFW_CD_DUMP_PRIORITY_CRITICAL);

    // _tx_thread_current_ptr
    set_expectations_crash_dump_register_address32(&_tx_thread_current_ptr, sizeof(TX_THREAD*), FPFW_CD_DUMP_PRIORITY_CRITICAL);

    // _tx_thread_execute_ptr
    set_expectations_crash_dump_register_address32(&_tx_thread_execute_ptr, sizeof(TX_THREAD*), FPFW_CD_DUMP_PRIORITY_CRITICAL);

    // _tx_thread_created_ptr
    set_expectations_crash_dump_register_address32(&_tx_thread_created_ptr, sizeof(TX_THREAD*), FPFW_CD_DUMP_PRIORITY_CRITICAL);

    // _tx_thread_created_count
    set_expectations_crash_dump_register_address32(&_tx_thread_created_count, sizeof(ULONG), FPFW_CD_DUMP_PRIORITY_CRITICAL);

    // _tx_thread_system_state
    set_expectations_crash_dump_register_address32((void*)&_tx_thread_system_state, sizeof(ULONG), FPFW_CD_DUMP_PRIORITY_CRITICAL);

    // _tx_thread_created_ptr (pThread)
    for (int i = 0; i < 2; i++)
    {
        set_expectations_crash_dump_register_address32(&threads[i], sizeof(TX_THREAD), FPFW_CD_DUMP_PRIORITY_CRITICAL);
        set_expectations_crash_dump_register_address32(threads[i].tx_thread_stack_start, 500, FPFW_CD_DUMP_PRIORITY_CRITICAL);
        set_expectations_crash_dump_register_address32(threads[i].tx_thread_name, 8, FPFW_CD_DUMP_PRIORITY_CRITICAL);
        set_expectations_crash_dump_register_address32_ptr_array(FPFW_CD_DUMP_PRIORITY_OPPORTUNISTIC,
                                                                 128,
                                                                 128,
                                                                 (void**)threads[i].tx_thread_stack_start,
                                                                 500 / sizeof(void*));
    }

    // Call the function under test
    crash_dump_capture_threadx(NULL);
}

static void preDumpCallback(void* pContext)
{
    check_expected_ptr(pContext);

    function_called();
}

TEST_FUNCTION(test_crash_dump_register_pre_dump_callback_valid, nullptr, nullptr)
{
    uint8_t preDumpCallbackCtx = 0;

    // Call API to register callback
    crash_dump_register_pre_dump_callback(preDumpCallback, &preDumpCallbackCtx);

    // Verify registered callback is called by override
    expect_function_call(preDumpCallback);
    expect_value(preDumpCallback, pContext, &preDumpCallbackCtx);

    preDumpCallbackOverride(&preDumpCallbackCtx);
}

TEST_FUNCTION(test_crash_dump_inMemoryOverride_validAddr, nullptr, nullptr)
{
    // Test valid address and size
    void* addr = (void*)0x40;
    uint32_t size = 8;

    assert_true(inMemoryOverride(addr, size));
}

TEST_FUNCTION(test_crash_dump_inMemoryOverride_inValidAddr, nullptr, nullptr)
{
    // Test invalid address
    void* addr = (void*)0xFFFFFFF0;
    uint32_t size = 6;

    assert_false(inMemoryOverride(addr, size));

    // Test invalid size
    addr = (void*)0x40;
    size = 0x80000;

    assert_false(inMemoryOverride(addr, size));
}
}