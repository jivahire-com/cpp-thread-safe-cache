//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crashDump_mocks.c
 * Mock implementation of shared library functions used by crash dump
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for check_expected_ptr, expect_fun...

extern "C" {
#include <FpFwUtils.h>                // for FPFW_UNUSED
#include <modules/CdDumpDescriptor.h> // for FPFwCDDumpDescriptorCtx, FPFwC...
#include <modules/CdDumpManager.h>    // for FPFwCrashDumpCtx
#include <modules/CdMemoryPool.h>     // for FPFwCDMemPoolCtx
#include <stdbool.h>                  // for bool, true
#include <stdint.h>                   // for uint32_t, uint64_t, uint8_t
#include <tx_api.h>                   // for UINT, TX_MUTEX, CHAR, ULONG

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
uint8_t __stack_start__ = 0;
uint8_t __stack_end__ = 0;
uint8_t _build_id_msdata_start = 0;

jmp_buf cd_test_setjmp_context;

/*------------- Functions ----------------*/
//
// Mocks
//
// Override crash_dump_wait_forever to restore test context
void crash_dump_wait_forever()
{
    longjmp(cd_test_setjmp_context, 1);
}

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

bool __wrap_FPFwCDInitDumpFile(FPFwCDDumpFileCtx* ctx)
{
    check_expected_ptr(ctx);

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpFileOverrideInValidMemory(FPFwCDDumpFileCtx* dumpFileCtx,
                                                bool (*inValidMemory)(void* addr, uint32_t size))
{
    check_expected_ptr(dumpFileCtx);
    check_expected_ptr(inValidMemory);

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpFileOverrideInValidCsrMemory(FPFwCDDumpFileCtx* dumpFileCtx,
                                                   bool (*inValidCsrMemory)(void* addr, uint32_t size))
{
    check_expected_ptr(dumpFileCtx);
    check_expected_ptr(inValidCsrMemory);

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpFileOverrideInValidGlobalMemory(FPFwCDDumpFileCtx* dumpFileCtx,
                                                      bool (*inValidGlobalMemory)(uint64_t address, uint32_t size))
{
    check_expected_ptr(dumpFileCtx);
    check_expected_ptr(inValidGlobalMemory);

    function_called();

    return true;
}

bool __wrap_FPFwCDInitDumpManager(FPFwCrashDumpCtx* ctx,
                                  FPFwCDMemPoolCtx* memPoolCtx,
                                  FPFwCDDumpDescriptorCtx* descCtx,
                                  FPFwCDDumpFileCtx* fileCtx,
                                  FPFwCDStateManagerCtx* stateCtx,
                                  uint64_t totalDumpSize)
{
    check_expected_ptr(ctx);
    check_expected_ptr(memPoolCtx);
    check_expected_ptr(descCtx);
    check_expected_ptr(fileCtx);
    check_expected_ptr(stateCtx);
    check_expected(totalDumpSize);

    function_called();

    return true;
}

void __wrap_FPFwCDOverridePrintf(int (*printOverride)(const char* str, ...))
{
    check_expected_ptr(printOverride);

    function_called();
}

bool __wrap_FPFwCDDumpManagerSetPreDumpCallback(FPFwCrashDumpCtx* ctx, bool (*preDumpCallback)(void*), void* preDumpCtx)
{
    check_expected_ptr(ctx);
    check_expected_ptr(preDumpCallback);
    check_expected_ptr(preDumpCtx);

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpManagerSetPostDumpCallback(FPFwCrashDumpCtx* ctx, bool (*postDumpCallback)(void*), void* postDumpCtx)
{
    check_expected_ptr(ctx);
    check_expected_ptr(postDumpCallback);
    check_expected_ptr(postDumpCtx);

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpManagerOverrideGetCurTime(FPFwCrashDumpCtx* ctx, uint64_t (*getCurTime)(void))
{
    check_expected_ptr(ctx);
    check_expected_ptr(getCurTime);

    function_called();

    return true;
}

bool __wrap_CdRegisterMMIORegisterSet(FPFwCrashDumpCtx* ctx, uint32_t regAddress, uint32_t regCount, uint32_t priority)
{
    check_expected_ptr(ctx);
    check_expected(regAddress);
    check_expected(regCount);
    check_expected(priority);

    function_called();

    return true;
}

bool __wrap_CdRegisterAddress32(FPFwCrashDumpCtx* ctx, void* address, uint32_t size, FPFwCdDumpPriority priority)
{
    check_expected_ptr(ctx);
    check_expected_ptr(address);
    check_expected(size);
    check_expected(priority);

    function_called();

    return true;
}

bool __wrap_CdRegisterAddress64(FPFwCrashDumpCtx* ctx, uint64_t address, uint32_t size, FPFwCdDumpPriority priority)
{
    check_expected_ptr(ctx);
    check_expected(address);
    check_expected(size);
    check_expected(priority);

    function_called();

    return true;
}

bool __wrap_CdRegisterAddress32PointerArray(FPFwCrashDumpCtx* ctx,
                                            FPFwCdDumpPriority priority,
                                            uint32_t minimumChunkSize,
                                            uint32_t maximumRegistrationCount,
                                            void** pointerArray,
                                            uint32_t pointerArrayCount)
{
    check_expected_ptr(ctx);
    check_expected(priority);
    check_expected(minimumChunkSize);
    check_expected(maximumRegistrationCount);
    check_expected_ptr(pointerArray);
    check_expected(pointerArrayCount);

    function_called();

    return true;
}

bool __wrap_CdRegisterCallback(FPFwCrashDumpCtx* ctx, FPFW_CD_DUMP_CALLBACK callback, void* context, FPFwCdDumpPriority priority)
{
    check_expected_ptr(ctx);
    check_expected_ptr(callback);
    check_expected_ptr(context);
    check_expected(priority);

    function_called();

    return true;
}

bool __wrap_CdRegisterRegisterSet(FPFwCrashDumpCtx* ctx, void* address, uint32_t regIndex, uint32_t regCount, uint32_t priority)
{
    check_expected_ptr(ctx);
    check_expected_ptr(address);
    check_expected(regIndex);
    check_expected(regCount);
    check_expected(priority);

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

int __wrap_gpio_set_output(uint32_t gpio_pin_id, uint32_t level)
{
    check_expected(gpio_pin_id);
    check_expected(level);

    function_called();

    return 0;
}
} // extern "C"
