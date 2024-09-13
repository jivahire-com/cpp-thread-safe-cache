//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file crash_dump_init.c
 *  Crash dump initialization.
 */

/*--------------- Includes ---------------*/
#include "crash_dump_gpio.h"      // for cd_gpio_assert_cd_in_progress
#include "crash_dump_overrides.h" // for cacheFlushOverride, cacheInvalidateOverride
#include "crash_dump_payload.h"   // for crash_dump_register_core_registers

#include <FpFwAssert.h>               // for FPFW_RUNTIME_ASSERT
#include <crash_dump.h>               // for crash_dump_init
#include <crash_dump_memory.h>        // for CRASH_DUMP_FULL_SIZE
#include <modules/CdDumpDescriptor.h> // for FPFwCDInitDumpDescriptor
#include <modules/CdMemoryPool.h>     // for FPFwCDInintMemoryPool
#include <stdbool.h>                  // for false
#include <stdint.h>                   // for uint8_t, uint32_t
#include <tx_api.h> // for TX_MUTEX, TX_SUCCESS, TX_NO_INHERIT, TX_WAIT_FOREVER, tx_mutex_create

/*-- Symbolic Constant Macros (defines) --*/
#define CRASH_DUMP_NUM_DESCRIPTORS 128 // ToDo: Re-evaluate this number
#define CD_DEFAULT_MEM_POOL_SIZE   1024

/*-------------- Typedefs ----------------*/

/*-------- Function Prototypes -----------*/
static void get_crash_dump_mem_heap_pool(uint64_t* mem_pool_addr, uint32_t* size);
static void init_mem_pool(uint64_t cd_mem_pool, uint32_t block_size);
static void init_dump_desc();
static void init_dump_file();
static void init_dump_manager();

/*-- Declarations (Statics and globals) --*/
static FPFwCrashDumpCtx crash_dump_ctx = {};
static FPFwCDMemPoolCtx mem_ctx = {};
static FPFwCDDumpDescriptorCtx desc_ctx = {};
static FPFwCDDumpFileCtx file_ctx = {};
static FPFwCDDumpDescriptor desc_list[CRASH_DUMP_NUM_DESCRIPTORS] = {};
static TX_MUTEX desc_mutex = {};
static crash_dump_config_t* crash_dump_config = NULL;

/*------------- Functions ----------------*/
/**
 * @brief Get the Crash Dump Context object
 *
 * @return FPFwCrashDumpCtx* Crash dump context
 */
FPFwCrashDumpCtx* GetCrashDumpContext()
{
    return &crash_dump_ctx;
}

/**
 * @brief Get the Crash Dump Context object
 *
 * @return Pointer to static crash dump context.
 */
crash_dump_config_t* GetCrashDumpConfig()
{
    return crash_dump_config;
}

/**
 * @brief Start initialization of crash dump.
 *
 * @param config Configuration for crash dump.
 *
 */
void crash_dump_init(crash_dump_config_t* config)
{
    // Cache the configuration
    crash_dump_config = config;

    // Set Processor ID with DIE index (Upper 16 bits) and core index (Lower 16 bits).
    crash_dump_ctx.prid = (config->die_index << 16) | (config->core_index & 0xFFFF);
    crash_dump_ctx.isPrimaryCore = config->is_primary;

    // De-assert CD_IN_PROGRESS
    cd_gpio_assert_cd_in_progress(false);

    // Initialize crash dump framework
    uint64_t cd_mem_pool = config->mem_pool_addr;
    uint32_t block_size = config->mem_pool_size;
    if (cd_mem_pool == 0)
    {
        // Crash dump memory pool is not available. Use heap pool.
        get_crash_dump_mem_heap_pool(&cd_mem_pool, &block_size);
    }
    init_mem_pool(cd_mem_pool, block_size);
    init_dump_desc();
    init_dump_file();
    init_dump_manager();

    // Register core built-in registers into crash dump
    crash_dump_register_core_registers();

    // Register error status registers and other diagnostic registers
    crash_dump_register_default_registers(config->mmio_registers, config->mmio_register_count);

    // Register core stack
    crash_dump_register_core_stack();

    // Add capture information about the core and the firmware
    crash_dump_register_standard_info();

    // Register ThreadX data registerer callback
    crash_dump_register_threadx();
}

/**
 * @brief Get crash dump memory heap pool.
 *
 * @param mem_pool_addr Memory pool address.
 * @param size Size of the memory pool.
 *
 */
static void get_crash_dump_mem_heap_pool(uint64_t* mem_pool_addr, uint32_t* size)
{
    *mem_pool_addr = (uint64_t)(uintptr_t)malloc(CD_DEFAULT_MEM_POOL_SIZE);
    *size = CD_DEFAULT_MEM_POOL_SIZE;
}

/**
 * @brief Initialize crash dump memory pool.
 *
 * @param cd_mem_pool Crash dump memory pool address.
 * @param block_size Size of the memory pool.
 *
 */
static void init_mem_pool(uint64_t cd_mem_pool, uint32_t block_size)
{
    // Initialize crash dump memory pool
    FPFW_RUNTIME_ASSERT(FPFwCDInitMemoryPool(&mem_ctx, cd_mem_pool, block_size));
    (void)FPFwCDMemPoolOverrideCacheFlush(&mem_ctx, &cacheFlushOverride);
    (void)FPFwCDMemPoolOverrideCacheInvalidate(&mem_ctx, &cacheInvalidateOverride);
}

/**
 * @brief Initialize crash dump description.
 *
 */
static void init_dump_desc()
{
    // Create Tx mutex for descriptor set
    FPFW_RUNTIME_ASSERT(tx_mutex_create(&desc_mutex, "cd desc mutex", TX_NO_INHERIT) == TX_SUCCESS);

    // Create crash dump descriptor
    FPFW_RUNTIME_ASSERT(FPFwCDInitDumpDescriptor(&desc_ctx, desc_list, CRASH_DUMP_NUM_DESCRIPTORS));

    (void)FPFwCDDumpDescriptorSetMutexCtx(&desc_ctx, (void*)&desc_mutex);
    (void)FPFwCDDumpDescriptorOverrideMutexLock(&desc_ctx, &mutexLockOverride);
    (void)FPFwCDDumpDescriptorOverrideMutexUnlock(&desc_ctx, &mutexUnlockOverride);
}

/**
 * @brief Initialize crash dump file.
 *
 */
static void init_dump_file()
{
    FPFW_RUNTIME_ASSERT(FPFwCDInitDumpFile(&file_ctx));
    (void)FPFwCDDumpFileOverrideInValidMemory(&file_ctx, &inMemoryOverride);
    (void)FPFwCDDumpFileOverrideInValidCsrMemory(&file_ctx, &inMemoryOverride);
    (void)FPFwCDDumpFileOverrideInValidGlobalMemory(&file_ctx, &inGlobalMemoryOverride);
    file_ctx.product = CD_PRODUCT_ID_KINGSGATE;
}

/**
 * @brief Initialize crash dump manager.
 *
 */
static void init_dump_manager()
{
    FPFW_RUNTIME_ASSERT(FPFwCDInitDumpManager(&crash_dump_ctx, &mem_ctx, &desc_ctx, &file_ctx, NULL, CRASH_DUMP_FULL_SIZE)); // No state manager.
    FPFwCDOverridePrintf(&crash_dump_printf);
    (void)FPFwCDDumpManagerSetPreDumpCallback(&crash_dump_ctx, &preDumpCallbackOverride, NULL);
    (void)FPFwCDDumpManagerSetPostDumpCallback(&crash_dump_ctx, &postDumpCallbackOverride, NULL);
    (void)FPFwCDDumpManagerOverrideGetCurTime(&crash_dump_ctx, &getCurTimeDefault);
}
