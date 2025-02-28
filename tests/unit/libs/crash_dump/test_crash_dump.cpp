//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_crash_dump.cpp
 * Crash dump unit tests
 */

/*------------- Includes -----------------*/
#include "test_crash_dump_expect.h" // for __wrap_CdRegisterAddress32, __wrap_CdRegisterCallback...

#include <CMockaWrapper.h> // for check_expected_ptr, expect_fun...
#include <cstdint>         // for uint32_t, uint64_t
#include <icc_platform_defines.h>
#include <modules/CdDumpDescriptor.h> // for _FPFwCdDumpPriority
#include <silibs_common.h>
#include <stddef.h> // for NULL
#include <tx_api.h> // for TX_THREAD, ULONG

extern "C" {
#include <../src/crash_dump_accel.h>     // for crash_dump_copy_accel_cd_file
#include <../src/crash_dump_gpio.h>      // for cd_gpio_assert_cd_available, cd_gpio_as...
#include <../src/crash_dump_icc.h>       // for crash_dump_transfer_full_dump_to_bmc
#include <../src/crash_dump_overrides.h> // for inMemoryOverride
#include <../src/crash_dump_payload.h>   // for CD_THREADX_DATA, crash_dump_capture_threadx
#include <cmsis_m7.h>                    // for SCB
#include <crash_dump.h>                  // for crash_dump_init
#include <crash_dump_memory.h>           // for CRASH_DUMP_MINI_SCP_ADDR, CRASH_DUMP_MINI_SCP_SIZE...
#include <icc_platform_defines.h>        // for accel_cd_addr_msg
#include <kng_icc_shared.h>              // for ICC_SIGNAL_CRASH_DUMP_COLLECT
#include <nvic.h>                        // for NVIC_STATUS_SUCCESS

/*-- Symbolic Constant Macros (defines) --*/
#define CD_DEFAULT_MEM_POOL_SIZE 1024

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

extern icc_base_recv_complete_notify fw_load_cb;
extern void* cb_ctx;
extern bool memcpy_mock;
extern jmp_buf cd_test_setjmp_context;

/*------------- Functions ----------------*/
//
// Mocks
//
bool in_memory(uintptr_t start_addr, uintptr_t end_addr)
{
    assert_true(start_addr <= end_addr);

    return mock_type(bool);
}

//
// Expectations
//
void set_expectations_initialize_crash_dump_header(bool wait, SEMAPHORE_ID sem_id, uint32_t sem_key)
{
    if (!wait)
    {
        // ToDo: Remove this when HSP initialize HW semaphore.
        expect_value(__wrap_initialize_semaphore, id, sem_id);
        expect_function_call(__wrap_initialize_semaphore);

        expect_value(__wrap_wait_for_semaphore, id, sem_id);
        expect_value(__wrap_wait_for_semaphore, key, sem_key);
        expect_function_call(__wrap_wait_for_semaphore);

        expect_value(__wrap_release_semaphore, id, sem_id);
        expect_function_call(__wrap_release_semaphore);

        // crash_dump_update_state
        expect_value(__wrap_wait_for_semaphore, id, sem_id);
        expect_value(__wrap_wait_for_semaphore, key, sem_key);
        expect_function_call(__wrap_wait_for_semaphore);

        expect_value(__wrap_release_semaphore, id, sem_id);
        expect_function_call(__wrap_release_semaphore);
    }
    else
    {
        expect_value(__wrap_wait_for_semaphore, id, sem_id);
        expect_value(__wrap_wait_for_semaphore, key, sem_key);
        expect_function_call(__wrap_wait_for_semaphore);

        expect_value(__wrap_release_semaphore, id, sem_id);
        expect_function_call(__wrap_release_semaphore);
    }

    // crash_dump_update_core_state
    expect_value(__wrap_wait_for_semaphore, id, sem_id);
    expect_value(__wrap_wait_for_semaphore, key, sem_key);
    expect_function_call(__wrap_wait_for_semaphore);

    expect_value(__wrap_release_semaphore, id, sem_id);
    expect_function_call(__wrap_release_semaphore);
}

void set_expectations_init_mem_pool(uint64_t addr, uint32_t size)
{
    expect_function_call(__wrap_FPFwCDInitMemoryPool);
    expect_value(__wrap_FPFwCDInitMemoryPool, baseAddr, addr);
    expect_value(__wrap_FPFwCDInitMemoryPool, poolSize, size);

    expect_function_call(__wrap_FPFwCDMemPoolOverrideCacheFlush);
    expect_value(__wrap_FPFwCDMemPoolOverrideCacheFlush, cacheFlush, &cacheFlushOverride);

    expect_function_call(__wrap_FPFwCDMemPoolOverrideCacheInvalidate);
    expect_value(__wrap_FPFwCDMemPoolOverrideCacheInvalidate, cacheInvalidate, &cacheInvalidateOverride);
}

void set_expectations_init_dump_desc(bool is_full_dump)
{
    expect_function_call(__wrap__txe_mutex_create);
    expect_string(__wrap__txe_mutex_create, name_ptr, is_full_dump ? "cd full mutex" : "cd mini mutex");

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

void set_expectations_init_dump_manager(uint32_t expectDumpSize)
{
    expect_function_call(__wrap_FPFwCDInitDumpManager);
    expect_value(__wrap_FPFwCDInitDumpManager, totalDumpSize, expectDumpSize);

    expect_function_call(__wrap_FPFwCDOverridePrintf);
    expect_value(__wrap_FPFwCDOverridePrintf, printOverride, &crash_dump_printf);

    expect_function_call(__wrap_FPFwCDDumpManagerSetPreDumpCallback);
    expect_value(__wrap_FPFwCDDumpManagerSetPreDumpCallback, preDumpCallback, &preDumpCallbackOverride);
    expect_any(__wrap_FPFwCDDumpManagerSetPreDumpCallback, preDumpCtx);

    expect_function_call(__wrap_FPFwCDDumpManagerSetPostDumpCallback);
    expect_value(__wrap_FPFwCDDumpManagerSetPostDumpCallback, postDumpCallback, &postDumpCallbackOverride);
    expect_any(__wrap_FPFwCDDumpManagerSetPostDumpCallback, postDumpCtx);

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
    expect_not_value(__wrap_CdRegisterCallback, context, NULL);
    expect_value(__wrap_CdRegisterCallback, priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);
}

void set_expectations_crash_dump_register_mmio_register(volatile void* mmio_reg, uint32_t reg_count, FPFwCdDumpPriority priority)
{
    expect_function_call(__wrap_CdRegisterMMIORegisterSet);
    expect_value(__wrap_CdRegisterMMIORegisterSet, regAddress, mmio_reg);
    expect_value(__wrap_CdRegisterMMIORegisterSet, regCount, reg_count);
    expect_value(__wrap_CdRegisterMMIORegisterSet, priority, priority);
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

void set_expectations_crash_dump_register_address64(uint64_t address_exp, uint32_t size_exp, FPFwCdDumpPriority priority_exp)
{
    expect_function_call(__wrap_CdRegisterAddress64);
    expect_value(__wrap_CdRegisterAddress64, address, address_exp);
    expect_value(__wrap_CdRegisterAddress64, size, size_exp);
    expect_value(__wrap_CdRegisterAddress64, priority, priority_exp);
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

void set_expectations_gpio_set_output(crash_dump_context_t* context)
{
    will_return(__wrap_crash_dump_context, context);

    if (context->core_index == CRASH_DUMP_CORE_MCP)
    {
        expect_function_call(__wrap_gpio_set_output);
        expect_value(__wrap_gpio_set_output, gpio_pin_id, 0x0602); // MSCP_EXP_GPIO_6 | SAFE_MODE_REQ (2)
        expect_value(__wrap_gpio_set_output, level, 1);
    }
}

void set_expectations_crash_dump_register_default_registers(const core_register_mmio_t* mmio_registers, uint32_t mmio_register_count)
{
    for (uint32_t i = 0; i < mmio_register_count; i++)
    {
        expect_function_call(__wrap_CdRegisterMMIORegisterSet);
        expect_value(__wrap_CdRegisterMMIORegisterSet, regAddress, mmio_registers[i].address);
        expect_value(__wrap_CdRegisterMMIORegisterSet, regCount, mmio_registers[i].count);
        expect_value(__wrap_CdRegisterMMIORegisterSet, priority, mmio_registers[i].priority);
    }

    // ToDo: Add more register checks for SCP_EXP, Watchdog and Power control registers
    expect_function_call(__wrap_CdRegisterMMIORegisterSet); // SCB->MMFAR
    expect_value(__wrap_CdRegisterMMIORegisterSet, regAddress, &SCB->MMFAR);
    expect_value(__wrap_CdRegisterMMIORegisterSet, regCount, 1);
    expect_value(__wrap_CdRegisterMMIORegisterSet, priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);

    expect_function_call(__wrap_CdRegisterMMIORegisterSet); // SCB->BFAR
    expect_value(__wrap_CdRegisterMMIORegisterSet, regAddress, &SCB->BFAR);
    expect_value(__wrap_CdRegisterMMIORegisterSet, regCount, 1);
    expect_value(__wrap_CdRegisterMMIORegisterSet, priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);

    expect_function_call(__wrap_CdRegisterMMIORegisterSet); // SCB->HFSR
    expect_value(__wrap_CdRegisterMMIORegisterSet, regAddress, &SCB->HFSR);
    expect_value(__wrap_CdRegisterMMIORegisterSet, regCount, 1);
    expect_value(__wrap_CdRegisterMMIORegisterSet, priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);

    expect_function_call(__wrap_CdRegisterMMIORegisterSet); // SCB->CFSR
    expect_value(__wrap_CdRegisterMMIORegisterSet, regAddress, &SCB->CFSR);
    expect_value(__wrap_CdRegisterMMIORegisterSet, regCount, 1);
    expect_value(__wrap_CdRegisterMMIORegisterSet, priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);
}

//
// Tests
//
/**
 * @brief crash dump root context initialization test
 *
 */
TEST_FUNCTION(test_crash_dump_init, nullptr, nullptr)
{
    crash_dump_context_t context = {.die_index = 0, .core_index = CRASH_DUMP_CORE_SCP, .in_memory = in_memory};

    // Set up expectations
    set_expectations_gpio_set_output(&context);

    crash_dump_init(&context);
}

/**
 * @brief MCP full crash dump type context registration test
 *
 */
TEST_FUNCTION(test_crash_dump_register_full_dump_mcp, nullptr, nullptr)
{
    const core_register_mmio_t core_register_mmio[] = {{(volatile void*)(0x12123434), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
                                                       {(volatile void*)(0xababcdcd), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL}};

    crash_dump_header_t header = {.status = CRASH_DUMP_IN_USE};

    crash_dump_type_context_t type_context = {.type = CRASH_DUMP_TYPE_FULL,
                                              .mem_pool_addr = CRASH_DUMP_FULL_MCP_ADDR,
                                              .mem_pool_size = CRASH_DUMP_FULL_MCP_SIZE,
                                              .semaphore = {.id = SEM_ID_DIE0_IOSS_0, .key = 1},
                                              .header = &header};

    crash_dump_context_t context = {.die_index = 0,
                                    .core_index = CRASH_DUMP_CORE_MCP,
                                    .mmio_register_count = 2,
                                    .mmio_registers = core_register_mmio,
                                    .in_memory = in_memory};

    will_return_always(__wrap_crash_dump_context, &context);

    set_expectations_initialize_crash_dump_header(true, SEM_ID_DIE0_IOSS_0, 1);

    // init_dump_desc()
    set_expectations_init_dump_desc(true);

    // init_mem_pool()
    set_expectations_init_mem_pool(CRASH_DUMP_FULL_MCP_ADDR, CRASH_DUMP_FULL_MCP_SIZE);

    // init_dump_file()
    set_expectations_init_dump_file();

    // init_dump_manager()
    set_expectations_init_dump_manager(CRASH_DUMP_FULL_MCP_SIZE);

    // crash_dump_register_core_registers()
    set_expectations_crash_dump_register_core_registers();

    // crash_dump_register_default_registers()
    set_expectations_crash_dump_register_default_registers(core_register_mmio, 2);

    // crash_dump_register_core_stack()
    set_expectations_crash_dump_register_core_stack();

    // crash_dump_register_standard_info()
    set_expectations_crash_dump_register_standard_info();

    // crash_dump_register_threadx()
    set_expectations_crash_dump_register_threadx();

    // Call API under test
    KNG_STATUS result = crash_dump_register_dump(&type_context);
    assert_true(KNG_SUCCESS == result);
}

/**
 * @brief SCP full crash dump type context registration test
 *
 */
TEST_FUNCTION(test_crash_dump_register_full_dump_scp, nullptr, nullptr)
{
    const core_register_mmio_t core_register_mmio[] = {{(volatile void*)(0x12123434), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
                                                       {(volatile void*)(0xababcdcd), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL}};

    crash_dump_header_t header = {};

    crash_dump_type_context_t type_context = {.type = CRASH_DUMP_TYPE_FULL,
                                              .mem_pool_addr = CRASH_DUMP_FULL_SCP_ADDR,
                                              .mem_pool_size = CRASH_DUMP_FULL_SCP_SIZE,
                                              .semaphore = {.id = SEM_ID_DIE0_IOSS_0, .key = 2},
                                              .header = &header};

    crash_dump_context_t context = {.die_index = 0,
                                    .core_index = CRASH_DUMP_CORE_SCP,
                                    .mmio_register_count = 2,
                                    .mmio_registers = core_register_mmio,
                                    .in_memory = in_memory};

    will_return_always(__wrap_crash_dump_context, &context);

    set_expectations_initialize_crash_dump_header(false, SEM_ID_DIE0_IOSS_0, 2);

    // init_dump_desc()
    set_expectations_init_dump_desc(true);

    // init_mem_pool()
    set_expectations_init_mem_pool(CRASH_DUMP_FULL_SCP_ADDR, CRASH_DUMP_FULL_SCP_SIZE);

    // init_dump_file()
    set_expectations_init_dump_file();

    // init_dump_manager()
    set_expectations_init_dump_manager(CRASH_DUMP_FULL_SCP_SIZE);

    // crash_dump_register_core_registers()
    set_expectations_crash_dump_register_core_registers();

    // crash_dump_register_default_registers()
    set_expectations_crash_dump_register_default_registers(core_register_mmio, 2);

    // crash_dump_register_core_stack()
    set_expectations_crash_dump_register_core_stack();

    // crash_dump_register_standard_info()
    set_expectations_crash_dump_register_standard_info();

    // crash_dump_register_threadx()
    set_expectations_crash_dump_register_threadx();

    // Call API under test
    KNG_STATUS result = crash_dump_register_dump(&type_context);
    assert_true(KNG_SUCCESS == result);
}

/**
 * @brief SCP full crash dump type context registration test
 *
 */
TEST_FUNCTION(test_crash_dump_register_mini_dump, nullptr, nullptr)
{
    const core_register_mmio_t core_register_mmio[] = {{(volatile void*)(0x12123434), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
                                                       {(volatile void*)(0xababcdcd), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL}};

    crash_dump_header_t header = {};

    crash_dump_type_context_t type_context = {.type = CRASH_DUMP_TYPE_MINI,
                                              .mem_pool_addr = CRASH_DUMP_MINI_SCP_ADDR,
                                              .mem_pool_size = CRASH_DUMP_MINI_SCP_SIZE,
                                              .semaphore = {.id = SEM_ID_MSCP_EXP_0, .key = 2},
                                              .header = &header};

    crash_dump_context_t context = {.die_index = 0,
                                    .core_index = CRASH_DUMP_CORE_SCP,
                                    .mmio_register_count = 2,
                                    .mmio_registers = core_register_mmio,
                                    .in_memory = in_memory};

    will_return_always(__wrap_crash_dump_context, &context);

    set_expectations_initialize_crash_dump_header(false, SEM_ID_MSCP_EXP_0, 2);

    // init_dump_desc()
    set_expectations_init_dump_desc(false);

    // init_mem_pool()
    set_expectations_init_mem_pool(CRASH_DUMP_MINI_SCP_ADDR, CRASH_DUMP_MINI_SCP_SIZE);

    // init_dump_file()
    set_expectations_init_dump_file();

    // init_dump_manager()
    set_expectations_init_dump_manager(CRASH_DUMP_MINI_SCP_SIZE);

    // crash_dump_register_core_registers()
    set_expectations_crash_dump_register_core_registers();

    // crash_dump_register_default_registers()
    set_expectations_crash_dump_register_default_registers(core_register_mmio, 2);

    // crash_dump_register_core_stack()
    set_expectations_crash_dump_register_core_stack();

    // crash_dump_register_standard_info()
    set_expectations_crash_dump_register_standard_info();

    // Call API under test
    KNG_STATUS result = crash_dump_register_dump(&type_context);
    assert_true(KNG_SUCCESS == result);
}

TEST_FUNCTION(test_crash_dump_config_icc_mhu_local, NULL, NULL)
{
    crash_dump_context_t context = {.die_index = 0, .core_index = CRASH_DUMP_CORE_SCP, .in_memory = in_memory};
    will_return(__wrap_crash_dump_context, &context);

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0x12345678);
    expect_value(__wrap_fpfw_icc_base_recv, params->buffer_size, 512);
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, ICC_SIGNAL_CRASH_DUMP_COLLECT);

    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_recv);

    will_return_always(__wrap_nvic_get_current_irq, NVIC_STATUS_SUCCESS);
    expect_function_call_any(NVIC_SetPriority);

    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_MHU_LOCAL, (fpfw_icc_base_ctx_t*)0x12345678);
    assert_true(context.icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_LOCAL] == (fpfw_icc_base_ctx_t*)0x12345678);

    if (!setjmp(cd_test_setjmp_context))
    {
        fw_load_cb(cb_ctx, 0, FPFW_STATUS_SUCCESS);
    }
}

TEST_FUNCTION(test_crash_dump_config_icc_mhu_remote, NULL, NULL)
{
    crash_dump_context_t context = {.die_index = 0, .core_index = CRASH_DUMP_CORE_SCP, .in_memory = in_memory};
    will_return(__wrap_crash_dump_context, &context);

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0x12345678);
    expect_value(__wrap_fpfw_icc_base_recv, params->buffer_size, 512);
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, ICC_SIGNAL_CRASH_DUMP_COLLECT);

    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_recv);

    will_return_always(__wrap_nvic_get_current_irq, NVIC_STATUS_ERROR);

    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_MHU_REMOTE, (fpfw_icc_base_ctx_t*)0x12345678);

    assert_true(context.icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_REMOTE] == (fpfw_icc_base_ctx_t*)0x12345678);

    if (!setjmp(cd_test_setjmp_context))
    {
        fw_load_cb(cb_ctx, 0, FPFW_STATUS_SUCCESS);
    }
}

TEST_FUNCTION(test_crash_dump_config_icc_spi_remote, NULL, NULL)
{
    crash_dump_context_t context = {.die_index = 0, .core_index = CRASH_DUMP_CORE_SCP, .in_memory = in_memory};
    will_return(__wrap_crash_dump_context, &context);

    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0x12345678);
    expect_value(__wrap_fpfw_icc_base_recv, params->buffer_size, sizeof(rmss_d2d_mailbox_msg));
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, RMSS_D2D_MAILBOX_MSG_CRASHDUMP_SIGNAL_REQ);

    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_recv);

    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_SPI_REMOTE, (fpfw_icc_base_ctx_t*)0x12345678);

    assert_true(context.icc_ctx[CRASH_DUMP_ICC_CONFIG_SPI_REMOTE] == (fpfw_icc_base_ctx_t*)0x12345678);
}

TEST_FUNCTION(test_crash_dump_config_icc_hsp, NULL, NULL)
{
    crash_dump_context_t context = {.die_index = 0, .core_index = CRASH_DUMP_CORE_SCP, .in_memory = in_memory};
    will_return(__wrap_crash_dump_context, &context);

    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_HSP, (fpfw_icc_base_ctx_t*)0x12345678);

    assert_true(context.icc_ctx[CRASH_DUMP_ICC_CONFIG_HSP] == (fpfw_icc_base_ctx_t*)0x12345678);
}

TEST_FUNCTION(test_crash_dump_config_icc_sdm, NULL, NULL)
{
    crash_dump_context_t context = {.die_index = 0, .core_index = CRASH_DUMP_CORE_SCP, .in_memory = in_memory};
    will_return_always(__wrap_crash_dump_context, &context);

    // for icc_register_accel_addr_callback()
    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0x12345678);
    expect_value(__wrap_fpfw_icc_base_recv, params->buffer_size, sizeof(accel_cd_addr_msg));
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, LARGE_FIFO_MAILBOX_MSG_CRASHDUMP_ADDR_REQ);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_recv);

    // Recv ICC CB
    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_SDM, (fpfw_icc_base_ctx_t*)0x12345678);

    assert_true(context.icc_ctx[CRASH_DUMP_ICC_CONFIG_SDM] == (fpfw_icc_base_ctx_t*)0x12345678);
    fw_load_cb(cb_ctx, 0, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_crash_dump_config_icc_sdm_recv_fail, NULL, NULL)
{
    crash_dump_context_t context = {.die_index = 0, .core_index = CRASH_DUMP_CORE_SCP, .in_memory = in_memory};
    will_return_always(__wrap_crash_dump_context, &context);

    // for icc_register_accel_addr_callback()
    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0x12345678);
    expect_value(__wrap_fpfw_icc_base_recv, params->buffer_size, sizeof(accel_cd_addr_msg));
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, LARGE_FIFO_MAILBOX_MSG_CRASHDUMP_ADDR_REQ);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_INVALID_ARG_ERR);
    expect_function_call(__wrap_fpfw_icc_base_recv);

    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_SDM, (fpfw_icc_base_ctx_t*)0x12345678);

    assert_true(context.icc_ctx[CRASH_DUMP_ICC_CONFIG_SDM] == (fpfw_icc_base_ctx_t*)0x0);
}

TEST_FUNCTION(test_crash_dump_config_icc_cded, NULL, NULL)
{
    crash_dump_context_t context = {.die_index = 0, .core_index = CRASH_DUMP_CORE_SCP, .in_memory = in_memory};
    will_return_always(__wrap_crash_dump_context, &context);

    // for icc_register_accel_addr_callback()
    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0x12345678);
    expect_value(__wrap_fpfw_icc_base_recv, params->buffer_size, sizeof(accel_cd_addr_msg));
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, LARGE_FIFO_MAILBOX_MSG_CRASHDUMP_ADDR_REQ);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_recv);

    // Recv ICC CB
    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_CDED, (fpfw_icc_base_ctx_t*)0x12345678);

    assert_true(context.icc_ctx[CRASH_DUMP_ICC_CONFIG_CDED] == (fpfw_icc_base_ctx_t*)0x12345678);
    fw_load_cb(cb_ctx, 0, FPFW_STATUS_SUCCESS);
}

TEST_FUNCTION(test_crash_dump_config_icc_cded_recv_fail, NULL, NULL)
{
    crash_dump_context_t context = {.die_index = 0, .core_index = CRASH_DUMP_CORE_SCP, .in_memory = in_memory};
    will_return_always(__wrap_crash_dump_context, &context);

    // for icc_register_accel_addr_callback()
    expect_value(__wrap_fpfw_icc_base_recv, icc_ctx, (fpfw_icc_base_ctx_t*)0x12345678);
    expect_value(__wrap_fpfw_icc_base_recv, params->buffer_size, sizeof(accel_cd_addr_msg));
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, LARGE_FIFO_MAILBOX_MSG_CRASHDUMP_ADDR_REQ);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_INVALID_ARG_ERR);
    expect_function_call(__wrap_fpfw_icc_base_recv);

    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_CDED, (fpfw_icc_base_ctx_t*)0x12345678);

    assert_true(context.icc_ctx[CRASH_DUMP_ICC_CONFIG_CDED] == (fpfw_icc_base_ctx_t*)0x0);
}

TEST_FUNCTION(test_crash_dump_config_icc_null_config, NULL, NULL)
{
    will_return(__wrap_crash_dump_context, NULL);

    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_CDED, (fpfw_icc_base_ctx_t*)0x12345678);
}

TEST_FUNCTION(test_crash_dump_config_icc_invalid_icc_config_enum, NULL, NULL)
{
    crash_dump_context_t context = {.die_index = 0, .core_index = CRASH_DUMP_CORE_SCP, .in_memory = in_memory};
    will_return_always(__wrap_crash_dump_context, &context);

    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_MAX, (fpfw_icc_base_ctx_t*)0x12345678);

    assert_true(context.icc_ctx[CRASH_DUMP_ICC_CONFIG_CDED] == (fpfw_icc_base_ctx_t*)0x0);
}

TEST_FUNCTION(test_crash_dump_config_icc_null_icc_ctx, NULL, NULL)
{
    crash_dump_context_t context = {.die_index = 0, .core_index = CRASH_DUMP_CORE_SCP, .in_memory = in_memory};
    will_return_always(__wrap_crash_dump_context, &context);

    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_MAX, (fpfw_icc_base_ctx_t*)0x0);

    assert_true(context.icc_ctx[CRASH_DUMP_ICC_CONFIG_CDED] == (fpfw_icc_base_ctx_t*)0x0);
}

TEST_FUNCTION(test_crash_dump_config_icc_invalid_icc_ctx, NULL, NULL)
{
    crash_dump_context_t context = {.die_index = 0, .core_index = CRASH_DUMP_CORE_SCP, .in_memory = in_memory};
    will_return_always(__wrap_crash_dump_context, &context);

    context.icc_ctx[CRASH_DUMP_ICC_CONFIG_CDED] = (fpfw_icc_base_ctx_t*)0xABCD1234;
    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_MAX, (fpfw_icc_base_ctx_t*)0x12345678);
}

TEST_FUNCTION(test_crash_dump_config_icc_invalid_icc_ctx2, NULL, NULL)
{
    crash_dump_context_t context = {.die_index = 0, .core_index = CRASH_DUMP_CORE_SCP, .in_memory = in_memory};
    will_return_always(__wrap_crash_dump_context, &context);

    context.icc_ctx[CRASH_DUMP_ICC_CONFIG_CDED] = (fpfw_icc_base_ctx_t*)0xABCD1234;
    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_MAX, (fpfw_icc_base_ctx_t*)0x0);
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
    crash_dump_context_t context = {.die_index = 0, .core_index = CRASH_DUMP_CORE_SCP, .in_memory = in_memory};
    will_return(__wrap_crash_dump_context, &context);
    will_return(in_memory, true);

    // Test valid address and size
    void* addr = (void*)0x40;
    uint32_t size = 8;

    assert_true(inMemoryOverride(addr, size));
}

TEST_FUNCTION(test_crash_dump_inMemoryOverride_inValidAddr, nullptr, nullptr)
{
    // Test invalid address (overflow)
    void* addr = (void*)0xFFFFFFFD;
    uint32_t size = 6;

    assert_false(inMemoryOverride(addr, size));
}

TEST_FUNCTION(test_crash_dump_handler, nullptr, nullptr)
{
    uint32_t errorCode = 0x12345678;
    uint32_t p1 = 0x87654321;
    uint32_t p2 = 0x12348765;
    uint32_t p3 = 0x87651234;
    uint32_t p4 = 0x12348765;

    // Set up expectations
    crash_dump_header_t mini_header = {.status = CRASH_DUMP_IN_USE, .cores = {3, 3, 0, 0, 0, 0}};
    crash_dump_header_t full_header = {.status = CRASH_DUMP_IN_USE, .cores = {3, 3, 0, 0, 0, 0}};
    crash_dump_type_context_t mini_context = {.type = CRASH_DUMP_TYPE_MINI, .header = &mini_header};
    crash_dump_type_context_t full_context = {.type = CRASH_DUMP_TYPE_FULL, .header = &full_header};
    crash_dump_context_t context = {.type_ctx = {&mini_context, &full_context},
                                    .die_index = 0,
                                    .core_index = CRASH_DUMP_CORE_SCP,
                                    .icc_ctx =
                                        {
                                            (fpfw_icc_base_ctx_t*)0x00000001, // Non-null ICC MHU MSCP local Context,
                                            (fpfw_icc_base_ctx_t*)0x00000002, // Non-null ICC MHU SCP remote Context
                                            (fpfw_icc_base_ctx_t*)0x00000003, // Non-null ICC SPI SCP remote Context
                                            (fpfw_icc_base_ctx_t*)0x00000004, // Non-null ICC HSP Context
                                            (fpfw_icc_base_ctx_t*)0x00000005, // Non-null ICC SDM Context
                                            (fpfw_icc_base_ctx_t*)0x00000006, // Non-null ICC CDED Context
                                        },
                                    .in_memory = in_memory};

    will_return_always(__wrap_crash_dump_context, &context);

    // Set core state to in-progress for mini and full
    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    // for cd_gpio_assert_cd_in_progress()

    // Expect ICC local notification
    expect_value(__wrap_fpfw_icc_base_send_sync, icc_ctx, context.icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_LOCAL]);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);

    // Expect ICC remote notification
    expect_value(__wrap_fpfw_icc_base_send_sync, icc_ctx, context.icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_REMOTE]);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);

    // Send ICC Accel notification
    expect_value(__wrap_fpfw_icc_base_send_sync, icc_ctx, context.icc_ctx[CRASH_DUMP_ICC_CONFIG_SDM]);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);
    expect_value(__wrap_fpfw_icc_base_send_sync, icc_ctx, context.icc_ctx[CRASH_DUMP_ICC_CONFIG_CDED]);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);

    // Expect ICC HSP notification
    expect_value(__wrap_fpfw_icc_base_send_sync, icc_ctx, context.icc_ctx[CRASH_DUMP_ICC_CONFIG_HSP]);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);

    // Main handler for mini dump
    expect_any(__wrap_FPFwCDCrashDumpHandler, ctx);
    expect_any(__wrap_FPFwCDCrashDumpHandler, coreInfo);

    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Code, errorCode);
    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Parameter[0], p1);
    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Parameter[1], p2);
    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Parameter[2], p3);
    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Parameter[3], p4);

    expect_function_call(__wrap_FPFwCDCrashDumpHandler);

    // Set core state to completed for mini
    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    // Main handler for full dump
    expect_any(__wrap_FPFwCDCrashDumpHandler, ctx);
    expect_any(__wrap_FPFwCDCrashDumpHandler, coreInfo);

    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Code, errorCode);
    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Parameter[0], p1);
    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Parameter[1], p2);
    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Parameter[2], p3);
    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Parameter[3], p4);

    expect_function_call(__wrap_FPFwCDCrashDumpHandler);

    // Set core state to completed for full
    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    // Expect ICC HSP notification
    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    expect_value(__wrap_fpfw_icc_base_send_sync, icc_ctx, context.icc_ctx[CRASH_DUMP_ICC_CONFIG_HSP]);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);

    crash_dump_handler(errorCode, p1, p2, p3, p4);
}

TEST_FUNCTION(test_crash_dump_handler_icc_ctx_null, nullptr, nullptr)
{
    uint32_t errorCode = 0x12345678;
    uint32_t p1 = 0x87654321;
    uint32_t p2 = 0x12348765;
    uint32_t p3 = 0x87651234;
    uint32_t p4 = 0x12348765;

    // Set up expectations
    crash_dump_header_t full_header = {};
    crash_dump_type_context_t full_context = {.type = CRASH_DUMP_TYPE_FULL, .header = &full_header};
    crash_dump_context_t context = {.type_ctx = {NULL, &full_context},
                                    .die_index = 0,
                                    .core_index = CRASH_DUMP_CORE_SCP,
                                    .icc_ctx =
                                        {
                                            (fpfw_icc_base_ctx_t*)0x00000000, // NULL ICC MHU MSCP local Context,
                                            (fpfw_icc_base_ctx_t*)0x00000000, // NULL ICC MHU SCP remote Context
                                            (fpfw_icc_base_ctx_t*)0x00000000, // NULL ICC SPI SCP remote Context
                                            (fpfw_icc_base_ctx_t*)0x00000000, // NULL ICC HSP Context
                                            (fpfw_icc_base_ctx_t*)0x00000000, // NULL ICC SDM Context
                                            (fpfw_icc_base_ctx_t*)0x00000000, // NULL ICC CDED Context
                                        },
                                    .in_memory = in_memory};

    will_return_always(__wrap_crash_dump_context, &context);

    // Set core state to in-progress
    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    // for cd_gpio_assert_cd_in_progress() : no_op for SCP

    // Expect ICC core notification : no_op

    // Send ICC Accel notification : no_op

    // Expect ICC HSP notification : no_op

    expect_any(__wrap_FPFwCDCrashDumpHandler, ctx);
    expect_any(__wrap_FPFwCDCrashDumpHandler, coreInfo);

    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Code, errorCode);
    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Parameter[0], p1);
    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Parameter[1], p2);
    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Parameter[2], p3);
    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Parameter[3], p4);

    expect_function_call(__wrap_FPFwCDCrashDumpHandler);

    // Set core state to completed
    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    // Checking dump done.
    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    crash_dump_handler(errorCode, p1, p2, p3, p4);
}

TEST_FUNCTION(test_crash_dump_handler_send_sync_fail, nullptr, nullptr)
{
    uint32_t errorCode = 0x12345678;
    uint32_t p1 = 0x87654321;
    uint32_t p2 = 0x12348765;
    uint32_t p3 = 0x87651234;
    uint32_t p4 = 0x12348765;

    // Set up expectations
    crash_dump_header_t full_header = {};
    crash_dump_type_context_t full_context = {.type = CRASH_DUMP_TYPE_FULL, .header = &full_header};
    crash_dump_context_t context = {.type_ctx = {NULL, &full_context},
                                    .die_index = 0,
                                    .core_index = CRASH_DUMP_CORE_SCP,
                                    .icc_ctx =
                                        {
                                            (fpfw_icc_base_ctx_t*)0x00000001, // Non-null ICC MHU MSCP local Context,
                                            (fpfw_icc_base_ctx_t*)0x00000002, // Non-null ICC MHU SCP remote Context
                                            (fpfw_icc_base_ctx_t*)0x00000003, // Non-null ICC SPI SCP remote Context
                                            (fpfw_icc_base_ctx_t*)0x00000004, // Non-null ICC HSP Context
                                            (fpfw_icc_base_ctx_t*)0x00000005, // Non-null ICC SDM Context
                                            (fpfw_icc_base_ctx_t*)0x00000006, // Non-null ICC CDED Context
                                        },
                                    .in_memory = in_memory};

    will_return_always(__wrap_crash_dump_context, &context);

    // Set core state to in-progress
    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    // for cd_gpio_assert_cd_in_progress()

    // Expect ICC local notification
    expect_value(__wrap_fpfw_icc_base_send_sync, icc_ctx, context.icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_LOCAL]);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_NULL_ARG_ERR);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);

    // Expect ICC remote notification
    expect_value(__wrap_fpfw_icc_base_send_sync, icc_ctx, context.icc_ctx[CRASH_DUMP_ICC_CONFIG_MHU_REMOTE]);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_NULL_ARG_ERR);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);

    // Expect ICC SPI notification
    expect_value(__wrap_fpfw_icc_base_send_sync, icc_ctx, context.icc_ctx[CRASH_DUMP_ICC_CONFIG_SPI_REMOTE]);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_NULL_ARG_ERR);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);

    // Send ICC Accel notification
    expect_value(__wrap_fpfw_icc_base_send_sync, icc_ctx, context.icc_ctx[CRASH_DUMP_ICC_CONFIG_SDM]);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_NULL_ARG_ERR);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);
    expect_value(__wrap_fpfw_icc_base_send_sync, icc_ctx, context.icc_ctx[CRASH_DUMP_ICC_CONFIG_CDED]);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_NULL_ARG_ERR);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);

    // Expect ICC HSP notification
    expect_value(__wrap_fpfw_icc_base_send_sync, icc_ctx, context.icc_ctx[CRASH_DUMP_ICC_CONFIG_HSP]);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_NULL_ARG_ERR);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);

    expect_any(__wrap_FPFwCDCrashDumpHandler, ctx);
    expect_any(__wrap_FPFwCDCrashDumpHandler, coreInfo);

    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Code, errorCode);
    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Parameter[0], p1);
    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Parameter[1], p2);
    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Parameter[2], p3);
    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Parameter[3], p4);

    expect_function_call(__wrap_FPFwCDCrashDumpHandler);

    // Set core state to completed
    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    // Checking dump done.
    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    // Sending warm reset
    expect_value(__wrap_fpfw_icc_base_send_sync, icc_ctx, context.icc_ctx[CRASH_DUMP_ICC_CONFIG_HSP]);
    will_return(__wrap_fpfw_icc_base_send_sync, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send_sync);

    crash_dump_handler(errorCode, p1, p2, p3, p4);
}

TEST_FUNCTION(test_crash_dump_handler_null_context, nullptr, nullptr)
{
    uint32_t errorCode = 0x12345678;
    uint32_t p1 = 0x87654321;
    uint32_t p2 = 0x12348765;
    uint32_t p3 = 0x87651234;
    uint32_t p4 = 0x12348765;

    // Set up expectations
    crash_dump_header_t full_header = {};
    crash_dump_type_context_t full_context = {.type = CRASH_DUMP_TYPE_FULL, .header = &full_header};
    crash_dump_context_t context = {.type_ctx = {NULL, &full_context},
                                    .die_index = 0,
                                    .core_index = CRASH_DUMP_CORE_SCP,
                                    .icc_ctx =
                                        {
                                            (fpfw_icc_base_ctx_t*)0x00000001, // Non-null ICC MHU MSCP local Context,
                                            (fpfw_icc_base_ctx_t*)0x00000002, // Non-null ICC MHU SCP remote Context
                                            (fpfw_icc_base_ctx_t*)0x00000003, // Non-null ICC SPI SCP remote Context
                                            (fpfw_icc_base_ctx_t*)0x00000004, // Non-null ICC HSP Context
                                            (fpfw_icc_base_ctx_t*)0x00000005, // Non-null ICC SDM Context
                                            (fpfw_icc_base_ctx_t*)0x00000006, // Non-null ICC CDED Context
                                        },
                                    .in_memory = in_memory};

    // Set core state to in-progress
    will_return_count(__wrap_crash_dump_context, &context, 2);
    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    // for cd_gpio_assert_cd_in_progress()
    will_return(__wrap_crash_dump_context, &context);

    // for checking single mode
    will_return(__wrap_crash_dump_context, &context);

    // Expect ICC local notification
    will_return(__wrap_crash_dump_context, NULL);

    // Send ICC Accel notification
    will_return(__wrap_crash_dump_context, NULL);

    // Expect ICC HSP notification
    will_return(__wrap_crash_dump_context, NULL);

    expect_any(__wrap_FPFwCDCrashDumpHandler, ctx);
    expect_any(__wrap_FPFwCDCrashDumpHandler, coreInfo);

    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Code, errorCode);
    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Parameter[0], p1);
    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Parameter[1], p2);
    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Parameter[2], p3);
    expect_value(__wrap_FPFwCDCrashDumpHandler, CdBugCheckInfo->data.Parameter[3], p4);

    expect_function_call(__wrap_FPFwCDCrashDumpHandler);

    // Set core state to completed
    will_return(__wrap_crash_dump_context, &context);
    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    // Expect warm reset request
    will_return(__wrap_crash_dump_context, NULL);

    crash_dump_handler(errorCode, p1, p2, p3, p4);
}

typedef struct _gpio_test_data_t
{
    bool input;
    uint32_t expected_level;
} gpio_test_data_t;

TEST_FUNCTION(test_crash_dump_in_progress_assert, nullptr, nullptr)
{
    crash_dump_context_t context = {.die_index = 0, .core_index = CRASH_DUMP_CORE_MCP, .in_memory = in_memory};

    gpio_test_data_t test_data[] = {{
                                        .input = true,
                                        .expected_level = 0,
                                    },
                                    {
                                        .input = false,
                                        .expected_level = 1,
                                    }};

    will_return_always(__wrap_crash_dump_context, &context);

    for (size_t i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i++)
    {
        // Set up expectations
        expect_function_call(__wrap_gpio_set_output);
        expect_value(__wrap_gpio_set_output, gpio_pin_id, 0x0602); // MSCP_EXP_GPIO_6 | SAFE_MODE_REQ (2)
        expect_value(__wrap_gpio_set_output, level, test_data[i].expected_level);

        // Call API under test
        cd_gpio_assert_cd_in_progress(test_data[i].input);
    }
}

TEST_FUNCTION(test_crash_dump_available_assert, nullptr, nullptr)
{
    crash_dump_context_t context = {.die_index = 0, .core_index = CRASH_DUMP_CORE_MCP, .in_memory = in_memory};

    gpio_test_data_t test_data[] = {{
                                        .input = true,
                                        .expected_level = 0,
                                    },
                                    {
                                        .input = false,
                                        .expected_level = 1,
                                    }};

    will_return_always(__wrap_crash_dump_context, &context);

    for (size_t i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i++)
    {
        // Set up expectations
        expect_function_call(__wrap_gpio_set_output);
        expect_value(__wrap_gpio_set_output, gpio_pin_id, 0x0603); // MSCP_EXP_GPIO_6 | GPIO_CD_AVAILABLE (3)
        expect_value(__wrap_gpio_set_output, level, test_data[i].expected_level);

        // Call API under test
        cd_gpio_assert_cd_available(test_data[i].input);
    }
}

TEST_FUNCTION(test_crash_dump_register_mmio_register, nullptr, nullptr)
{
    crash_dump_type_context_t mini_context = {.type = CRASH_DUMP_TYPE_MINI};
    crash_dump_type_context_t full_context = {.type = CRASH_DUMP_TYPE_FULL};
    crash_dump_context_t context = {.type_ctx = {&mini_context, &full_context}};

    will_return_always(__wrap_crash_dump_context, &context);

    // mini and full dump
    set_expectations_crash_dump_register_mmio_register((volatile void*)3434, 2, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    set_expectations_crash_dump_register_mmio_register((volatile void*)3434, 2, FPFW_CD_DUMP_PRIORITY_CRITICAL);

    crash_dump_register_mmio_register((volatile void*)3434, 2, FPFW_CD_DUMP_PRIORITY_CRITICAL);

    // mini dump only
    context.type_ctx[CRASH_DUMP_TYPE_FULL] = NULL;
    set_expectations_crash_dump_register_mmio_register((volatile void*)1212, 5, FPFW_CD_DUMP_PRIORITY_CRITICAL);

    crash_dump_register_mmio_register((volatile void*)1212, 5, FPFW_CD_DUMP_PRIORITY_CRITICAL);
}

TEST_FUNCTION(test_crash_dump_register_address32, nullptr, nullptr)
{
    crash_dump_type_context_t mini_context = {.type = CRASH_DUMP_TYPE_MINI};
    crash_dump_type_context_t full_context = {.type = CRASH_DUMP_TYPE_FULL};
    crash_dump_context_t context = {.type_ctx = {&mini_context, &full_context}};

    will_return_always(__wrap_crash_dump_context, &context);

    // mini and full dump
    set_expectations_crash_dump_register_address32((void*)1212, 135, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    set_expectations_crash_dump_register_address32((void*)1212, 135, FPFW_CD_DUMP_PRIORITY_CRITICAL);

    crash_dump_register_address32((void*)1212, 135, FPFW_CD_DUMP_PRIORITY_CRITICAL);

    // mini dump only
    context.type_ctx[CRASH_DUMP_TYPE_FULL] = NULL;

    set_expectations_crash_dump_register_address32((void*)2424, 246, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    crash_dump_register_address32((void*)2424, 246, FPFW_CD_DUMP_PRIORITY_CRITICAL);
}

TEST_FUNCTION(test_crash_dump_register_address64, nullptr, nullptr)
{
    crash_dump_type_context_t mini_context = {.type = CRASH_DUMP_TYPE_MINI};
    crash_dump_type_context_t full_context = {.type = CRASH_DUMP_TYPE_FULL};
    crash_dump_context_t context = {.type_ctx = {&mini_context, &full_context}};

    will_return_always(__wrap_crash_dump_context, &context);

    // mini and full dump
    set_expectations_crash_dump_register_address64(3434, 2, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    set_expectations_crash_dump_register_address64(3434, 2, FPFW_CD_DUMP_PRIORITY_CRITICAL);

    crash_dump_register_address64(3434, 2, FPFW_CD_DUMP_PRIORITY_CRITICAL);

    // mini dump only
    context.type_ctx[CRASH_DUMP_TYPE_FULL] = NULL;
    set_expectations_crash_dump_register_address64(1212, 20, FPFW_CD_DUMP_PRIORITY_CRITICAL);

    crash_dump_register_address64(1212, 20, FPFW_CD_DUMP_PRIORITY_CRITICAL);
}

TEST_FUNCTION(test_crash_dump_register_address32_pointer_array, nullptr, nullptr)
{
    crash_dump_type_context_t mini_context = {.type = CRASH_DUMP_TYPE_MINI};
    crash_dump_type_context_t full_context = {.type = CRASH_DUMP_TYPE_FULL};
    crash_dump_context_t context = {.type_ctx = {&mini_context, &full_context}};

    will_return_always(__wrap_crash_dump_context, &context);

    // mini and full dump
    set_expectations_crash_dump_register_address32_ptr_array(FPFW_CD_DUMP_PRIORITY_CRITICAL, 128, 256, NULL, 0);
    set_expectations_crash_dump_register_address32_ptr_array(FPFW_CD_DUMP_PRIORITY_CRITICAL, 128, 256, NULL, 0);

    crash_dump_register_address32_pointer_array(FPFW_CD_DUMP_PRIORITY_CRITICAL, 128, 256, NULL, 0);

    // full dump only
    context.type_ctx[CRASH_DUMP_TYPE_FULL] = NULL;
    set_expectations_crash_dump_register_address32_ptr_array(FPFW_CD_DUMP_PRIORITY_CRITICAL, 128, 256, NULL, 0);

    crash_dump_register_address32_pointer_array(FPFW_CD_DUMP_PRIORITY_CRITICAL, 128, 256, NULL, 0);
}

TEST_FUNCTION(test_crash_dump_copy_accel_cd_file_sdm, nullptr, nullptr)
{
    crash_dump_type_context_t type_context = {.type = CRASH_DUMP_TYPE_FULL};
    crash_dump_context_t context = {.type_ctx = {NULL, &type_context}};
    ACCEL_ID accel_type = ACCEL_ID_SDM;

    will_return(__wrap_crash_dump_context, &context);
    will_return(__wrap_atu_svc_accel_atu_addr, 0xDEADCAFE);

    context.accel_cd_dtcm_offset[accel_type] = 0x1234DEAD;
    memcpy_mock = true;
    crash_dump_copy_accel_cd_file((void*)accel_type);
    memcpy_mock = false;
}

TEST_FUNCTION(test_crash_dump_copy_accel_cd_file_cded, nullptr, nullptr)
{
    crash_dump_type_context_t type_context = {.type = CRASH_DUMP_TYPE_FULL};
    crash_dump_context_t context = {.type_ctx = {NULL, &type_context}};
    ACCEL_ID accel_type = ACCEL_ID_CDED;

    will_return(__wrap_crash_dump_context, &context);
    will_return(__wrap_atu_svc_accel_atu_addr, 0xDEADCAFE);

    context.accel_cd_dtcm_offset[accel_type] = 0x1234DEAD;
    memcpy_mock = true;
    crash_dump_copy_accel_cd_file((void*)accel_type);
    memcpy_mock = false;
}

TEST_FUNCTION(test_crash_dump_copy_accel_cd_file_null_config, nullptr, nullptr)
{
    ACCEL_ID accel_type = ACCEL_ID_SDM;

    will_return(__wrap_crash_dump_context, NULL);

    crash_dump_copy_accel_cd_file((void*)accel_type);
}

TEST_FUNCTION(test_crash_dump_copy_accel_cd_file_invalid_accel, nullptr, nullptr)
{
    crash_dump_context_t context = {};
    ACCEL_ID accel_type = NUM_VALID_ACCEL_ID;

    will_return(__wrap_crash_dump_context, &context);

    crash_dump_copy_accel_cd_file((void*)accel_type);
}

TEST_FUNCTION(test_crash_dump_copy_accel_cd_file_null_dtcm, nullptr, nullptr)
{
    crash_dump_context_t context = {};
    ACCEL_ID accel_type = ACCEL_ID_SDM;

    will_return(__wrap_crash_dump_context, &context);

    crash_dump_copy_accel_cd_file((void*)accel_type);
}

TEST_FUNCTION(test_crash_dump_check_completion, nullptr, nullptr)
{
    crash_dump_header_t mini_header = {.status = CRASH_DUMP_NOT_IN_USE};
    crash_dump_header_t full_header = {.status = CRASH_DUMP_NOT_IN_USE};
    crash_dump_type_context_t mini_context = {.type = CRASH_DUMP_TYPE_MINI, .header = &mini_header};
    crash_dump_type_context_t full_context = {.type = CRASH_DUMP_TYPE_FULL, .header = &full_header};
    crash_dump_context_t context = {.type_ctx = {&mini_context, &full_context}, .die_index = 0, .core_index = CRASH_DUMP_CORE_SCP};
    will_return_always(__wrap_crash_dump_context, &context);

    // for mini dump
    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    // for full dump
    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    // Test all types
    bool is_completed = crash_dump_get_is_dump_complete(NULL);
    assert_true(is_completed);

    // for mini dump
    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    // Test mini dump only
    is_completed = crash_dump_get_is_dump_complete(&mini_context);
    assert_true(is_completed);

    // for full dump
    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    // Test full dump only
    full_context.header->status = CRASH_DUMP_IN_USE;
    full_context.header->cores[3] = CRASH_DUMP_STATE_IN_PROGRESS;
    is_completed = crash_dump_get_is_dump_complete(&full_context);
    assert_false(is_completed);

    // for mini dump
    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    mini_context.header->status = CRASH_DUMP_IN_USE;
    mini_context.header->cores[0] = CRASH_DUMP_STATE_NOT_AVAILABLE;
    mini_context.header->cores[1] = CRASH_DUMP_STATE_COMPLETED;
    mini_context.header->cores[CRASH_DUMP_CORE_NUM] = CRASH_DUMP_STATE_NOT_AVAILABLE;
    mini_context.header->cores[CRASH_DUMP_CORE_NUM + 1] = CRASH_DUMP_STATE_COMPLETED;

    // Test mini dump only
    is_completed = crash_dump_get_is_dump_complete(&mini_context);
    assert_true(is_completed);

    // for mini dump
    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    // for full dump
    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    // Test all types
    is_completed = crash_dump_get_is_dump_complete(NULL);
    assert_false(is_completed);

    // Single core mode tests
    // for mini dump
    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    // for full dump
    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    context.single_core_mode = true;
    full_context.header->cores[3] = CRASH_DUMP_STATE_IN_PROGRESS;

    // Test all types
    is_completed = crash_dump_get_is_dump_complete(NULL);
    assert_true(is_completed);
}

TEST_FUNCTION(test_crash_dump_tx_full_dump_to_bm, nullptr, nullptr)
{
    crash_dump_header_t mini_header = {.status = CRASH_DUMP_NOT_IN_USE};
    crash_dump_header_t full_header = {.status = CRASH_DUMP_NOT_IN_USE};
    crash_dump_type_context_t mini_context = {.type = CRASH_DUMP_TYPE_MINI, .header = &mini_header};
    crash_dump_type_context_t full_context = {.type = CRASH_DUMP_TYPE_FULL, .header = &full_header};
    crash_dump_context_t context = {.type_ctx = {&mini_context, &full_context}, .die_index = 0, .core_index = CRASH_DUMP_CORE_SCP};
    will_return_always(__wrap_crash_dump_context, &context);

    // ToDo: Add expectations once the function is fully implemented.
    // https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484995

    crash_dump_transfer_full_dump_to_bmc();
}
}