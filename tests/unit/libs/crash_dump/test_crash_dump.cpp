//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_crash_dump.cpp
 * Crash dump tests
 */

/*------------- Includes -----------------*/
#include "FpFwUtils.h"                // for FPFW_UNUSED
#include "modules/CdDumpDescriptor.h" // for FPFwCDDumpDescriptorCtx, FPFwC...
#include "modules/CdMemoryPool.h"     // for FPFwCDMemPoolCtx

#include <CMockaWrapper.h> // for check_expected_ptr, expect_fun...
#include <cstdint>         // for uint32_t, uint64_t
#include <stddef.h>        // for NULL

extern "C" {
#include <crash_dump.h> // for crash_dump_init
#include <tx_api.h>     // for UINT, TX_MUTEX, CHAR, ULONG

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
bool __wrap_FPFwCDInitMemoryPool(FPFwCDMemPoolCtx* ctx, uint64_t baseAddr, uint32_t poolSize)
{
    check_expected_ptr(ctx);
    check_expected_ptr(baseAddr);
    check_expected(poolSize);

    function_called();

    return true;
}

bool __wrap_FPFwCDMemPoolOverrideCacheFlush(FPFwCDMemPoolCtx* memPoolCtx, void (*cacheFlush)(uint64_t addr, uint32_t size))
{
    check_expected_ptr(memPoolCtx);
    check_expected_ptr(cacheFlush);

    function_called();

    return true;
}

bool __wrap_FPFwCDMemPoolOverrideCacheInvalidate(FPFwCDMemPoolCtx* memPoolCtx,
                                                 void (*cacheInvalidate)(uint64_t addr, uint32_t size))
{
    check_expected_ptr(memPoolCtx);
    check_expected_ptr(cacheInvalidate);

    function_called();

    return true;
}

bool __wrap_FPFwCDInitDumpDescriptor(FPFwCDDumpDescriptorCtx* ctx, FPFwCDDumpDescriptor* dumpDescriptorArray, uint32_t arraySize)
{
    check_expected_ptr(ctx);
    check_expected_ptr(dumpDescriptorArray);
    check_expected(arraySize);

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpDescriptorSetMutexCtx(FPFwCDDumpDescriptorCtx* dumpDescCtx, void* mutexCtx)
{
    check_expected_ptr(dumpDescCtx);
    check_expected_ptr(mutexCtx);

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpDescriptorOverrideMutexLock(FPFwCDDumpDescriptorCtx* dumpDescCtx, uint32_t (*mutexLock)(void*))
{
    check_expected_ptr(dumpDescCtx);
    check_expected_ptr(mutexLock);

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpDescriptorOverrideMutexUnlock(FPFwCDDumpDescriptorCtx* dumpDescCtx, uint32_t (*mutexUnlock)(void*))
{
    check_expected_ptr(dumpDescCtx);
    check_expected_ptr(mutexUnlock);

    function_called();

    return true;
}

UINT __wrap__txe_mutex_create(TX_MUTEX* mutex_ptr, CHAR* name_ptr, UINT inherit, UINT mutex_control_block_size)
{
    check_expected_ptr(mutex_ptr);
    check_expected(name_ptr);
    FPFW_UNUSED(inherit);
    FPFW_UNUSED(mutex_control_block_size);

    function_called();

    return 0;
}

UINT __wrap__tx_mutex_get(TX_MUTEX* mutex_ptr, ULONG wait_option)
{
    FPFW_UNUSED(mutex_ptr);
    FPFW_UNUSED(wait_option);

    return 0;
}

UINT __wrap__tx_mutex_put(TX_MUTEX* mutex_ptr)
{
    FPFW_UNUSED(mutex_ptr);

    return 0;
}

//
// Tests
//
/**
 * @brief crash dump initialization test
 *
 */
TEST_FUNCTION(test_crash_dump_init, nullptr, nullptr)
{
    // Set up expectations
    expect_function_call(__wrap_FPFwCDInitMemoryPool);
    expect_not_value(__wrap_FPFwCDInitMemoryPool, ctx, NULL);
    expect_any(__wrap_FPFwCDInitMemoryPool, baseAddr);
    expect_any(__wrap_FPFwCDInitMemoryPool, poolSize);

    expect_function_call(__wrap_FPFwCDMemPoolOverrideCacheFlush);
    expect_not_value(__wrap_FPFwCDMemPoolOverrideCacheFlush, memPoolCtx, NULL);
    expect_not_value(__wrap_FPFwCDMemPoolOverrideCacheFlush, cacheFlush, NULL);

    expect_function_call(__wrap_FPFwCDMemPoolOverrideCacheInvalidate);
    expect_not_value(__wrap_FPFwCDMemPoolOverrideCacheInvalidate, memPoolCtx, NULL);
    expect_not_value(__wrap_FPFwCDMemPoolOverrideCacheInvalidate, cacheInvalidate, NULL);

    expect_function_call(__wrap__txe_mutex_create);
    expect_not_value(__wrap__txe_mutex_create, mutex_ptr, NULL);
    expect_string(__wrap__txe_mutex_create, name_ptr, "cd desc mutex");

    expect_function_call(__wrap_FPFwCDInitDumpDescriptor);
    expect_not_value(__wrap_FPFwCDInitDumpDescriptor, ctx, NULL);
    expect_not_value(__wrap_FPFwCDInitDumpDescriptor, dumpDescriptorArray, NULL);
    expect_not_value(__wrap_FPFwCDInitDumpDescriptor, arraySize, 0);

    expect_function_call(__wrap_FPFwCDDumpDescriptorSetMutexCtx);
    expect_not_value(__wrap_FPFwCDDumpDescriptorSetMutexCtx, dumpDescCtx, NULL);
    expect_not_value(__wrap_FPFwCDDumpDescriptorSetMutexCtx, mutexCtx, NULL);

    expect_function_call(__wrap_FPFwCDDumpDescriptorOverrideMutexLock);
    expect_not_value(__wrap_FPFwCDDumpDescriptorOverrideMutexLock, dumpDescCtx, NULL);
    expect_not_value(__wrap_FPFwCDDumpDescriptorOverrideMutexLock, mutexLock, NULL);

    expect_function_call(__wrap_FPFwCDDumpDescriptorOverrideMutexUnlock);
    expect_not_value(__wrap_FPFwCDDumpDescriptorOverrideMutexUnlock, dumpDescCtx, NULL);
    expect_not_value(__wrap_FPFwCDDumpDescriptorOverrideMutexUnlock, mutexUnlock, NULL);

    // Call API under test
    crash_dump_init();
}
}