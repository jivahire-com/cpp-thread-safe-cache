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
#include <CrashDump.h>                // for FPFW_CD_DUMP_CALLBACK
#include <FpFwUtils.h>                // for FPFW_UNUSED
#include <modules/CdDumpDescriptor.h> // for FPFwCDDumpDescriptorCtx, FPFwC...
#include <modules/CdDumpFile.h>       // for FPFwCDDumpFileCtx
#include <modules/CdDumpManager.h>    // for FPFwCrashDumpCtx
#include <modules/CdMemoryPool.h>     // for FPFwCDMemPoolCtx
#include <modules/CdStateManager.h>   // for FPFwCDStateManagerCtx
#include <setjmp.h>                   // for longjmp, jmp_buf
#include <stddef.h>                   // for NULL
#include <stdint.h>                   // for uint32_t, uint64_t, uint8_t
#include <tx_api.h>                   // for UINT, TX_MUTEX, CHAR, ULONG

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
FPFwCrashDumpCtx* mock_crash_dump_ctx;
FPFwCDMemPoolCtx* mock_memPoolCtx;
FPFwCDDumpDescriptorCtx* mock_descCtx;
FPFwCDDumpFileCtx* mock_fileCtx;
FPFwCDStateManagerCtx* mock_stateCtx;

// FPFwCDDumpDescriptor desc_list[];
TX_MUTEX* mock_desc_mutex;

// CD_MSFT_VERSION_INFO cdMsftVersionInfo;
// CD_THREADX_DATA cdThreadXData;

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

int init_crash_dump_context(void** pContext)
{
    FPFW_UNUSED(pContext);

    mock_crash_dump_ctx = NULL;
    mock_memPoolCtx = NULL;
    mock_descCtx = NULL;
    mock_fileCtx = NULL;
    mock_stateCtx = NULL;
    mock_desc_mutex = NULL;

    return 0;
}

bool __wrap_FPFwCDInitMemoryPool(FPFwCDMemPoolCtx* ctx, uint64_t baseAddr, uint32_t poolSize)
{
    assert_null(mock_memPoolCtx); // Ensure this function is called first time
    assert_non_null(ctx);         // Ensure the context is not NULL
    mock_memPoolCtx = ctx;        // Save the context

    check_expected_ptr(baseAddr);
    check_expected(poolSize);

    function_called();

    return true;
}

bool __wrap_FPFwCDMemPoolOverrideCacheFlush(FPFwCDMemPoolCtx* memPoolCtx, void (*cacheFlush)(uint64_t addr, uint32_t size))
{
    assert_ptr_equal(mock_memPoolCtx, memPoolCtx); // Ensure the context is the same as the one initialized
    check_expected_ptr(cacheFlush);

    function_called();

    return true;
}

bool __wrap_FPFwCDMemPoolOverrideCacheInvalidate(FPFwCDMemPoolCtx* memPoolCtx,
                                                 void (*cacheInvalidate)(uint64_t addr, uint32_t size))
{
    assert_ptr_equal(mock_memPoolCtx, memPoolCtx); // Ensure the context is the same as the one initialized
    check_expected_ptr(cacheInvalidate);

    function_called();

    return true;
}

bool __wrap_FPFwCDInitDumpDescriptor(FPFwCDDumpDescriptorCtx* ctx, FPFwCDDumpDescriptor* dumpDescriptorArray, uint32_t arraySize)
{
    assert_null(mock_descCtx); // Ensure this function is called first time
    assert_non_null(ctx);      // Ensure the context is not NULL
    mock_descCtx = ctx;        // Save the context

    assert_non_null(dumpDescriptorArray);
    check_expected(arraySize);

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpDescriptorSetMutexCtx(FPFwCDDumpDescriptorCtx* dumpDescCtx, void* mutexCtx)
{
    assert_ptr_equal(mock_descCtx, dumpDescCtx); // Ensure the context is the same as the one initialized
    assert_ptr_equal(mock_desc_mutex, mutexCtx); // Ensure the mutex is the same as the one initialized

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpDescriptorOverrideMutexLock(FPFwCDDumpDescriptorCtx* dumpDescCtx, uint32_t (*mutexLock)(void*))
{
    assert_ptr_equal(mock_descCtx, dumpDescCtx); // Ensure the context is the same as the one initialized
    check_expected_ptr(mutexLock);

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpDescriptorOverrideMutexUnlock(FPFwCDDumpDescriptorCtx* dumpDescCtx, uint32_t (*mutexUnlock)(void*))
{
    assert_ptr_equal(mock_descCtx, dumpDescCtx); // Ensure the context is the same as the one initialized
    check_expected_ptr(mutexUnlock);

    function_called();

    return true;
}

bool __wrap_FPFwCDInitDumpFile(FPFwCDDumpFileCtx* ctx)
{
    assert_null(mock_fileCtx); // Ensure this function is called first time
    assert_non_null(ctx);      // Ensure the context is not NULL
    mock_fileCtx = ctx;        // Save the context

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpFileOverrideInValidMemory(FPFwCDDumpFileCtx* dumpFileCtx,
                                                bool (*inValidMemory)(void* addr, uint32_t size))
{
    assert_ptr_equal(mock_fileCtx, dumpFileCtx); // Ensure the context is the same as the one initialized
    check_expected_ptr(inValidMemory);

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpFileOverrideInValidCsrMemory(FPFwCDDumpFileCtx* dumpFileCtx,
                                                   bool (*inValidCsrMemory)(void* addr, uint32_t size))
{
    assert_ptr_equal(mock_fileCtx, dumpFileCtx); // Ensure the context is the same as the one initialized
    check_expected_ptr(inValidCsrMemory);

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpFileOverrideInValidGlobalMemory(FPFwCDDumpFileCtx* dumpFileCtx,
                                                      bool (*inValidGlobalMemory)(uint64_t address, uint32_t size))
{
    assert_ptr_equal(mock_fileCtx, dumpFileCtx); // Ensure the context is the same as the one initialized
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
    assert_null(mock_crash_dump_ctx); // Ensure this function is called first time
    assert_non_null(ctx);             // Ensure the context is not NULL
    mock_crash_dump_ctx = ctx;        // Save the context

    assert_ptr_equal(mock_memPoolCtx, memPoolCtx); // Ensure the memory pool context is the same as the one initialized
    assert_ptr_equal(mock_descCtx, descCtx); // Ensure the descriptor context is the same as the one initialized
    assert_ptr_equal(mock_fileCtx, fileCtx); // Ensure the file context is the same as the one initialized
    assert_null(stateCtx);                   // Ensure the state context is NULL
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
    assert_ptr_equal(mock_crash_dump_ctx, ctx); // Ensure the context is the same as the one initialized

    check_expected_ptr(preDumpCallback);
    check_expected_ptr(preDumpCtx);

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpManagerSetPostDumpCallback(FPFwCrashDumpCtx* ctx, bool (*postDumpCallback)(void*), void* postDumpCtx)
{
    assert_ptr_equal(mock_crash_dump_ctx, ctx); // Ensure the context is the same as the one initialized

    check_expected_ptr(postDumpCallback);
    check_expected_ptr(postDumpCtx);

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpManagerOverrideGetCurTime(FPFwCrashDumpCtx* ctx, uint64_t (*getCurTime)(void))
{
    assert_ptr_equal(mock_crash_dump_ctx, ctx); // Ensure the context is the same as the one initialized

    check_expected_ptr(getCurTime);

    function_called();

    return true;
}

bool __wrap_CdRegisterMMIORegisterSet(FPFwCrashDumpCtx* ctx, uint32_t regAddress, uint32_t regCount, uint32_t priority)
{
    assert_ptr_equal(mock_crash_dump_ctx, ctx); // Ensure the context is the same as the one initialized

    check_expected(regAddress);
    check_expected(regCount);
    check_expected(priority);

    function_called();

    return true;
}

bool __wrap_CdRegisterAddress32(FPFwCrashDumpCtx* ctx, void* address, uint32_t size, FPFwCdDumpPriority priority)
{
    assert_ptr_equal(mock_crash_dump_ctx, ctx); // Ensure the context is the same as the one initialized

    check_expected_ptr(address);
    check_expected(size);
    check_expected(priority);

    function_called();

    return true;
}

bool __wrap_CdRegisterAddress64(FPFwCrashDumpCtx* ctx, uint64_t address, uint32_t size, FPFwCdDumpPriority priority)
{
    assert_ptr_equal(mock_crash_dump_ctx, ctx); // Ensure the context is the same as the one initialized

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
    assert_ptr_equal(mock_crash_dump_ctx, ctx); // Ensure the context is the same as the one initialized

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
    assert_ptr_equal(mock_crash_dump_ctx, ctx); // Ensure the context is the same as the one initialized

    check_expected_ptr(callback);
    check_expected_ptr(context);
    check_expected(priority);

    function_called();

    return true;
}

bool __wrap_CdRegisterRegisterSet(FPFwCrashDumpCtx* ctx, void* address, uint32_t regIndex, uint32_t regCount, uint32_t priority)
{
    assert_ptr_equal(mock_crash_dump_ctx, ctx); // Ensure the context is the same as the one initialized

    check_expected_ptr(address);
    check_expected(regIndex);
    check_expected(regCount);
    check_expected(priority);

    function_called();

    return true;
}

UINT __wrap__txe_mutex_create(TX_MUTEX* mutex_ptr, CHAR* name_ptr, UINT inherit, UINT mutex_control_block_size)
{
    assert_null(mock_desc_mutex); // Ensure this function is called first time
    assert_non_null(mutex_ptr);   // Ensure the mutex pointer is not NULL
    mock_desc_mutex = mutex_ptr;  // Save the mutex pointer

    check_expected(name_ptr);
    FPFW_UNUSED(inherit);
    FPFW_UNUSED(mutex_control_block_size);

    function_called();

    return 0;
}

UINT __wrap__tx_mutex_get(TX_MUTEX* mutex_ptr, ULONG wait_option)
{
    assert_ptr_equal(mock_desc_mutex, mutex_ptr); // Ensure the mutex pointer is the same as the one initialized

    FPFW_UNUSED(wait_option);

    return 0;
}

UINT __wrap__tx_mutex_put(TX_MUTEX* mutex_ptr)
{
    assert_ptr_equal(mock_desc_mutex, mutex_ptr); // Ensure the mutex pointer is the same as the one initialized

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
