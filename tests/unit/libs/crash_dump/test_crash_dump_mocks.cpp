//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crashDump_mocks.c
 * Mock implementation of shared library functions used by crash dump
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for check_expected_ptr, expect_fun...
#include <map>

extern "C" {
#include <CrashDump.h>                // for FPFW_CD_DUMP_CALLBACK
#include <FpFwUtils.h>                // for FPFW_UNUSED
#include <crash_dump.h>               // for crash_dump_config_t, GetCrashDu...
#include <crash_dump_dfwk.h>          // for crash_dump_interface_t
#include <fpfw_icc_base.h>            // for icc_base_recv_complete_notify
#include <idsw.h>                     // for idsw_plat_id_t
#include <modules/CdDumpDescriptor.h> // for FPFwCDDumpDescriptorCtx, FPFwC...
#include <modules/CdDumpFile.h>       // for FPFwCDDumpFileCtx
#include <modules/CdDumpManager.h>    // for FPFwCrashDumpCtx
#include <modules/CdMemoryPool.h>     // for FPFwCDMemPoolCtx
#include <modules/CdStateManager.h>   // for FPFwCDStateManagerCtx
#include <nvic.h>                     // for nvic_status_t
#include <setjmp.h>                   // for longjmp, jmp_buf
#include <stddef.h>                   // for NULL
#include <stdint.h>                   // for uint32_t, uint64_t, uint8_t
#include <string.h>                   // for memcpy
#include <tx_api.h>                   // for UINT, TX_MUTEX, CHAR, ULONG

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void* __real_memcpy(void* __a, const void* __b, size_t __c);

/*-- Declarations (Statics and globals) --*/
jmp_buf cd_test_setjmp_context;

std::map<uint32_t, icc_base_recv_complete_notify> s_icc_recv_cb;
std::map<uint32_t, void*> s_cb_ctx;
bool memcpy_mock = false;

DFWK_ASYNC_REQUEST_DISPATCH static_dispatch_routine = NULL;
VOID (*static_timer_cb)(ULONG id) = NULL;

/*------------- Functions ----------------*/
//
// Mocks
//
// Override crash_dump_wait_forever to restore test context
void crash_dump_wait_forever()
{
    longjmp(cd_test_setjmp_context, 1);
}

crash_dump_context_t* __wrap_crash_dump_context()
{
    return mock_type(crash_dump_context_t*);
}

void __wrap_DfwkAsyncRequestComplete(PDFWK_ASYNC_REQUEST_HEADER Request)
{
    check_expected_ptr(Request);

    function_called();
}

void __wrap_DfwkDeviceInitialize(PDFWK_DEVICE_HEADER Device, PDFWK_SCHEDULE Schedule)
{
    check_expected_ptr(Device);
    check_expected_ptr(Schedule);

    function_called();
}

void __wrap_DfwkQueueInitialize(PDFWK_QUEUE Queue,
                                PDFWK_DEVICE_HEADER Device,
                                DFWK_ASYNC_REQUEST_DISPATCH DispatchRoutine,
                                void* DispatchContext,
                                DFWK_QUEUE_TYPE QueueType)
{
    check_expected_ptr(Queue);
    check_expected_ptr(Device);
    check_expected_ptr(DispatchRoutine);
    check_expected_ptr(DispatchContext);
    check_expected(QueueType);

    static_dispatch_routine = DispatchRoutine;

    function_called();
}

void __wrap_DfwkInterfaceInitialize(PDFWK_INTERFACE_HEADER Interface,
                                    PDFWK_DEVICE_HEADER Device,
                                    PDFWK_QUEUE DispatchQueue,
                                    DFWK_REQUEST_DISPATCH_SYNC DispatchSync)
{
    check_expected_ptr(Interface);
    check_expected_ptr(Device);
    check_expected_ptr(DispatchQueue);
    check_expected_ptr(DispatchSync);

    function_called();
}

void __wrap_DfwkAsyncRequestInitialize(PDFWK_ASYNC_REQUEST_HEADER Request, size_t RequestSize)
{
    assert_non_null(Request);     // Ensure the request is not NULL
    assert_true(RequestSize > 0); // Ensure the request size is valid

    function_called();
}

DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE static_cd_dfwk_CompletionRoutine = NULL;

void __wrap_DfwkAsyncRequestSetCompletionRoutine(PDFWK_ASYNC_REQUEST_HEADER Request,
                                                 DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
                                                 void* CompletionContext)
{
    assert_non_null(Request);
    assert_non_null(CompletionRoutine);
    check_expected(CompletionContext);

    static_cd_dfwk_CompletionRoutine = CompletionRoutine;

    function_called();
}

int32_t __wrap_DfwkClientInterfaceOpen(PDFWK_INTERFACE_HEADER Interface)
{
    assert_non_null(Interface);

    function_called();

    // Return success for the mock
    return mock_type(int32_t);
}

void __wrap_DfwkInterfaceSendAsync(PDFWK_INTERFACE_HEADER Interface, PDFWK_ASYNC_REQUEST_HEADER Request)
{
    assert_non_null(Interface);
    assert_non_null(Request);

    function_called();
}

bool __wrap_FPFwCDInitMemoryPool(FPFwCDMemPoolCtx* ctx, uint64_t baseAddr, uint32_t poolSize)
{
    assert_non_null(ctx); // Ensure the context is not NULL

    check_expected(baseAddr);
    check_expected(poolSize);

    function_called();

    return true;
}

bool __wrap_FPFwCDMemPoolOverrideCacheFlush(FPFwCDMemPoolCtx* memPoolCtx, void (*cacheFlush)(uint64_t addr, uint32_t size))
{
    assert_non_null(memPoolCtx);
    check_expected_ptr(cacheFlush);

    function_called();

    return true;
}

bool __wrap_FPFwCDMemPoolOverrideCacheInvalidate(FPFwCDMemPoolCtx* memPoolCtx,
                                                 void (*cacheInvalidate)(uint64_t addr, uint32_t size))
{
    assert_non_null(memPoolCtx);
    check_expected_ptr(cacheInvalidate);

    function_called();

    return true;
}

bool __wrap_FPFwCDInitDumpDescriptor(FPFwCDDumpDescriptorCtx* ctx, FPFwCDDumpDescriptor* dumpDescriptorArray, uint32_t arraySize)
{
    assert_non_null(ctx); // Ensure the context is not NULL
    assert_non_null(dumpDescriptorArray);
    check_expected(arraySize);

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpDescriptorSetMutexCtx(FPFwCDDumpDescriptorCtx* dumpDescCtx, void* mutexCtx)
{
    assert_non_null(dumpDescCtx);
    assert_non_null(mutexCtx);

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpDescriptorOverrideMutexLock(FPFwCDDumpDescriptorCtx* dumpDescCtx, uint32_t (*mutexLock)(void*))
{
    assert_non_null(dumpDescCtx);
    check_expected_ptr(mutexLock);

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpDescriptorOverrideMutexUnlock(FPFwCDDumpDescriptorCtx* dumpDescCtx, uint32_t (*mutexUnlock)(void*))
{
    assert_non_null(dumpDescCtx);
    check_expected_ptr(mutexUnlock);

    function_called();

    return true;
}

bool __wrap_FPFwCDInitDumpFile(FPFwCDDumpFileCtx* ctx)
{
    assert_non_null(ctx); // Ensure the context is not NULL

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpFileOverrideInValidMemory(FPFwCDDumpFileCtx* dumpFileCtx,
                                                bool (*inValidMemory)(void* addr, uint32_t size))
{
    assert_non_null(dumpFileCtx);
    check_expected_ptr(inValidMemory);

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpFileOverrideInValidCsrMemory(FPFwCDDumpFileCtx* dumpFileCtx,
                                                   bool (*inValidCsrMemory)(void* addr, uint32_t size))
{
    assert_non_null(dumpFileCtx);
    check_expected_ptr(inValidCsrMemory);

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpFileOverrideInValidGlobalMemory(FPFwCDDumpFileCtx* dumpFileCtx,
                                                      bool (*inValidGlobalMemory)(uint64_t address, uint32_t size))
{
    assert_non_null(dumpFileCtx);
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
    assert_non_null(ctx);
    assert_non_null(memPoolCtx);
    assert_non_null(descCtx);
    assert_non_null(fileCtx);
    assert_null(stateCtx); // Ensure the state context is NULL
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
    assert_non_null(ctx);

    check_expected_ptr(preDumpCallback);
    check_expected_ptr(preDumpCtx);

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpManagerSetPostDumpCallback(FPFwCrashDumpCtx* ctx, bool (*postDumpCallback)(void*), void* postDumpCtx)
{
    assert_non_null(ctx);

    check_expected_ptr(postDumpCallback);
    check_expected_ptr(postDumpCtx);

    function_called();

    return true;
}

bool __wrap_FPFwCDDumpManagerOverrideGetCurTime(FPFwCrashDumpCtx* ctx, uint64_t (*getCurTime)(void))
{
    assert_non_null(ctx);

    check_expected_ptr(getCurTime);

    function_called();

    return true;
}

bool __wrap_CdRegisterMMIORegisterSet(FPFwCrashDumpCtx* ctx, uint32_t regAddress, uint32_t regCount, uint32_t priority)
{
    assert_non_null(ctx);

    check_expected(regAddress);
    check_expected(regCount);
    check_expected(priority);

    function_called();

    return true;
}

bool __wrap_CdRegisterAddress32(FPFwCrashDumpCtx* ctx, void* address, uint32_t size, FPFwCdDumpPriority priority)
{
    assert_non_null(ctx);

    check_expected_ptr(address);
    check_expected(size);
    check_expected(priority);

    function_called();

    return true;
}

bool __wrap_CdRegisterAddress64(FPFwCrashDumpCtx* ctx, uint64_t address, uint32_t size, FPFwCdDumpPriority priority)
{
    assert_non_null(ctx);

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
    assert_non_null(ctx);

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
    assert_non_null(ctx);

    check_expected_ptr(callback);
    check_expected_ptr(context);
    check_expected(priority);

    function_called();

    return true;
}

bool __wrap_CdRegisterRegisterSet(FPFwCrashDumpCtx* ctx, void* address, uint32_t regIndex, uint32_t regCount, uint32_t priority)
{
    assert_non_null(ctx);

    check_expected_ptr(address);
    check_expected(regIndex);
    check_expected(regCount);
    check_expected(priority);

    function_called();

    return true;
}

FPFW_CD_CUSTOM_CHUNK_UPDATE_CALLBACK __wrap_cd_ecid_update_callback = NULL;

bool __wrap_CdRegisterCustomChunk(FPFwCrashDumpCtx* ctx,
                                  GUID signature,
                                  void* clientContext,
                                  uint32_t payloadSize,
                                  FPFW_CD_CUSTOM_CHUNK_GET_SIZE_CALLBACK getSizeCallback,
                                  FPFW_CD_CUSTOM_CHUNK_UPDATE_CALLBACK updateCallback,
                                  FPFwCdDumpPriority priority)
{
    assert_non_null(ctx);

    check_expected_ptr(&signature);
    check_expected_ptr(clientContext);
    check_expected(payloadSize);
    check_expected_ptr(getSizeCallback);
    assert_non_null(updateCallback);
    check_expected(priority);

    __wrap_cd_ecid_update_callback = updateCallback;

    function_called();

    return true;
}

UINT __wrap__txe_mutex_create(TX_MUTEX* mutex_ptr, CHAR* name_ptr, UINT inherit, UINT mutex_control_block_size)
{
    assert_non_null(mutex_ptr); // Ensure the mutex pointer is not NULL

    check_expected(name_ptr);
    FPFW_UNUSED(inherit);
    FPFW_UNUSED(mutex_control_block_size);

    function_called();

    return 0;
}

UINT __wrap__tx_mutex_get(TX_MUTEX* mutex_ptr, ULONG wait_option)
{
    assert_non_null(mutex_ptr);

    FPFW_UNUSED(wait_option);

    return 0;
}

UINT __wrap__tx_mutex_put(TX_MUTEX* mutex_ptr)
{
    assert_non_null(mutex_ptr);

    return 0;
}

UINT __wrap__txe_timer_create(TX_TIMER* timer_ptr,
                              CHAR* name_ptr,
                              VOID (*expiration_function)(ULONG id),
                              ULONG expiration_input,
                              ULONG initial_ticks,
                              ULONG reschedule_ticks,
                              UINT auto_activate,
                              UINT timer_control_block_size)
{
    assert_non_null(timer_ptr);
    check_expected(name_ptr);
    assert_non_null(expiration_function);
    FPFW_UNUSED(expiration_input);
    assert_true(initial_ticks > 0);
    assert_true(reschedule_ticks > 0);
    FPFW_UNUSED(auto_activate);
    FPFW_UNUSED(timer_control_block_size);

    static_timer_cb = expiration_function;

    function_called();

    return 0;
}

UINT __wrap__txe_timer_deactivate(TX_TIMER* timer_ptr)
{
    assert_non_null(timer_ptr);

    function_called();

    return 0;
}

UINT __wrap__tx_thread_sleep(ULONG timer_ticks)
{
    FPFW_UNUSED(timer_ticks);

    return 0;
}

int __wrap_gpio_set_output(uint32_t gpio_pin_id, uint32_t level)
{
    check_expected(gpio_pin_id);
    check_expected(level);

    function_called();

    return 0;
}

int __wrap_gpio_get_input(uint32_t gpio_ctrl_pin_id, uint32_t* level)
{
    check_expected(gpio_ctrl_pin_id);
    assert_non_null(level);
    *level = mock_type(uint32_t);

    function_called();

    return 0;
}

void __wrap_FPFwCDCrashDumpHandler(FPFwCrashDumpCtx* ctx, const void* coreInfo, FPFwCdBugCheckInfo* CdBugCheckInfo)
{
    check_expected_ptr(ctx);
    check_expected_ptr(coreInfo);

    check_expected(CdBugCheckInfo->data.Code);
    check_expected(CdBugCheckInfo->data.Parameter[0]);
    check_expected(CdBugCheckInfo->data.Parameter[1]);
    check_expected(CdBugCheckInfo->data.Parameter[2]);
    check_expected(CdBugCheckInfo->data.Parameter[3]);

    function_called();
}

fpfw_status_t __wrap_fpfw_icc_base_recv(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_recv_req_t* params)
{
    check_expected(icc_ctx);
    assert_non_null(params);
    check_expected(params->buffer_size);
    check_expected(params->recv_cmd_code);
    assert_non_null(params->cb);
    s_icc_recv_cb[params->recv_cmd_code] = params->cb;
    s_cb_ctx[params->recv_cmd_code] = params->cb_ctx;

    function_called();

    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_base_send(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_send_req_t* params)
{
    check_expected(icc_ctx);
    assert_non_null(params);
    assert_non_null(params->payload_buffer);
    assert_true(params->buffer_size > 0);
    assert_non_null(params->cb);
    assert_non_null(params->cb_ctx);

    function_called();

    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_base_send_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size)
{
    check_expected(icc_ctx);
    assert_non_null(payload_buffer);
    assert_true(buffer_size > 0);

    function_called();

    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_base_recv_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size, size_t* output_recv_bytes)
{
    check_expected(icc_ctx);
    assert_non_null(payload_buffer);
    assert_true(buffer_size > 0);
    assert_non_null(output_recv_bytes);

    function_called();

    return mock_type(fpfw_status_t);
}

void* __wrap_fpfw_init_get_handle(const char* id)
{
    assert_string_equal(id, "cd_drv");
    function_called();

    return mock_type(pcrash_dump_interface_t);
}

void __wrap_initialize_semaphore(SEMAPHORE_ID id)
{
    check_expected(id);

    function_called();
}

void __wrap_wait_for_semaphore(SEMAPHORE_ID id, uint32_t key)
{
    check_expected(id);
    check_expected(key);

    function_called();
}

void __wrap_release_semaphore(SEMAPHORE_ID id)
{
    check_expected(id);

    function_called();
}

bool __wrap_idhw_is_single_die_boot_en(void)
{
    function_called();

    return mock_type(bool);
}

idsw_plat_id_t __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(idsw_plat_id_t);
}

uint32_t __wrap_atu_svc_accel_atu_addr(ACCEL_ID accel_id)
{
    FPFW_UNUSED(accel_id);

    return mock_type(uint32_t);
}

void* __wrap_memcpy(void* __a, const void* __b, size_t __c)
{
    if (memcpy_mock)
    {
        return NULL;
    }
    return __real_memcpy(__a, __b, __c);
}

nvic_status_t __wrap_nvic_get_current_irq(uint32_t* irq_num)
{
    FPFW_UNUSED(irq_num);

    return mock_type(nvic_status_t);
}

bool __wrap_system_info_is_warm_start()
{
    return mock_type(bool);
}

uint64_t __wrap_get_timestamp_from_cd(DUMP_CHUNK_CRASH_INFORMATION* p_dump_crash_info)
{
    FPFW_UNUSED(p_dump_crash_info);
    return 0x123456789ABCDEF;
}

void __wrap_update_timestamp_in_cd(DUMP_CHUNK_CRASH_INFORMATION* p_dump_crash_info, uint64_t utc_crash_time_ms)
{
    FPFW_UNUSED(p_dump_crash_info);
    FPFW_UNUSED(utc_crash_time_ms);
}

uint64_t __wrap_utc_sync_client_get_current_time_epoch_ms()
{
    return 0xFEDCBA987654321;
}

uint64_t __wrap_gtimer_get_timestamp_ms()
{
    return 0x123456789ABCDEF;
}

} // extern "C"
