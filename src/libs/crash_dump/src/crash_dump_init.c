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
#include "crash_dump_status.h"    // for crash_dump_update_state, crash_dump_update_core_state

#include <FpFwAssert.h>               // for FPFW_RUNTIME_ASSERT
#include <crash_dump.h>               // for crash_dump_init
#include <crash_dump_memory.h>        // for CRASH_DUMP_MINI_HEADER_ADDR, CRASH_DUMP_MINI_HEADER_SIZE...
#include <idhw.h>                     // for idhw_is_single_die_boot_en
#include <idsw_kng.h>                 // for DIE_0, DIE_1
#include <modules/CdDumpDescriptor.h> // for FPFwCDInitDumpDescriptor
#include <modules/CdMemoryPool.h>     // for FPFwCDInintMemoryPool
#include <stdbool.h>                  // for false
#include <stdint.h>                   // for uint8_t, uint32_t
#include <tx_api.h> // for TX_MUTEX, TX_SUCCESS, TX_NO_INHERIT, TX_WAIT_FOREVER, tx_mutex_create

/*-- Symbolic Constant Macros (defines) --*/
#define CRASH_DUMP_NUM_DESCRIPTORS 128 // ToDo: Re-evaluate this number

/*-------------- Typedefs ----------------*/

/*-------- Function Prototypes -----------*/
static void init_mem_pool(uint64_t cd_mem_pool, uint32_t block_size);
static void init_dump_desc();
static void init_dump_file();
static void init_dump_manager(uint64_t totalDumpSize);

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
    crash_dump_ctx.prid = CRASH_DUMP_PROCESSOR_ID(config->die_index, config->core_index);
    crash_dump_ctx.isPrimaryCore = config->is_primary;

    // De-assert CD_IN_PROGRESS
    cd_gpio_assert_cd_in_progress(false);

    // Initialize crash dump framework
    init_dump_desc();
    crash_dump_enable_full_dump(config->core_index == CRASH_DUMP_CORE_SCP ? false : true);

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
 * @brief Enable or disable full crash dump
 *
 * @param enable true to enable full crash dump, false to disable (mini dump only)
 *
 * @return true if successful, false otherwise
 */
bool crash_dump_enable_full_dump(bool enable)
{
    uint64_t mem_pool_addr = 0;
    uint32_t mem_pool_size = 0;
    crash_dump_status_t* status = NULL;
    crash_dump_config_t* config = GetCrashDumpConfig();

    if (config == NULL)
    {
        FPFwCDPrintf("Crash dump configuration is not available\n");
        return false;
    }

    if ((config->dump_type == FPFW_CD_DUMP_TYPE_FULL && enable) || (config->dump_type == FPFW_CD_DUMP_TYPE_MINI && !enable))
    {
        // Already in the desired state
        return true;
    }

    if (enable)
    {
        if (IS_PLATFORM_SVP())
        {
            // If initializing a mini crash dump OR on SVP use a local semaphore within the MSCP EXP Block.
            //   - See SVP Bug SVP bug https://azurecsi.visualstudio.com/1P-SoC-Modeling/_workitems/edit/2327121
            // If initializing a full crash dump use a semaphore within the IOSS block.
            config->cd_semaphore.semaphore_id = SEM_ID_MSCP_EXP_0;
        }
        else
        {
            config->cd_semaphore.semaphore_id = SEM_ID_DIE0_IOSS_0;
        }

        // Configure full dump memory pool.
        switch (config->core_index)
        {
        case CRASH_DUMP_CORE_MCP:
            status = (crash_dump_status_t*)CRASH_DUMP_FULL_HEADER_ADDR;
            mem_pool_addr = CRASH_DUMP_FULL_MCP_ADDR;
            mem_pool_size = CRASH_DUMP_FULL_MCP_SIZE;
            break;
        case CRASH_DUMP_CORE_SCP:
            status = (crash_dump_status_t*)CRASH_DUMP_FULL_HEADER_ADDR;
            mem_pool_addr = CRASH_DUMP_FULL_SCP_ADDR;
            mem_pool_size = CRASH_DUMP_FULL_SCP_SIZE;
            break;
        default:
            // MSCP only supports MCP and SCP cores.
            return false;
        }
    }
    else
    {
        // Configure hw semaphore to use MSCP_EXP_0 for mini dump.
        config->cd_semaphore.semaphore_id = SEM_ID_MSCP_EXP_0;

        // Configure mini dump memory pool.
        switch (config->core_index)
        {
        case CRASH_DUMP_CORE_SCP:
            status = (crash_dump_status_t*)CRASH_DUMP_MINI_HEADER_ADDR;
            mem_pool_addr = CRASH_DUMP_MINI_SCP_ADDR;
            mem_pool_size = CRASH_DUMP_MINI_SCP_SIZE;
            break;
        default:
            // Only SCP and HSP support mini dump.
            return false;
        }
    }

    // If the status is already set, clear it.
    crash_dump_update_state(CRASH_DUMP_NOT_IN_USE);

    // Update new crash dump status buffer.
    config->dump_type = enable ? FPFW_CD_DUMP_TYPE_FULL : FPFW_CD_DUMP_TYPE_MINI;
    config->cd_status = status;

    // Initialize crash dump header lock (hw semaphore) for Die0 SCP.
    // ToDo: This must be moved to HSP.
    initialize_crash_dump_header_lock(config);

    crash_dump_update_state(CRASH_DUMP_IN_USE);

    // Initialize FPFW crash dump memory pool, file and manager
    init_mem_pool(mem_pool_addr, mem_pool_size);
    init_dump_file();
    init_dump_manager(mem_pool_size);

    return true;
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
static void init_dump_manager(uint64_t totalDumpSize)
{
    FPFW_RUNTIME_ASSERT(FPFwCDInitDumpManager(&crash_dump_ctx, &mem_ctx, &desc_ctx, &file_ctx, NULL, totalDumpSize)); // No state manager.
    FPFwCDOverridePrintf(&crash_dump_printf);
    (void)FPFwCDDumpManagerSetPreDumpCallback(&crash_dump_ctx, &preDumpCallbackOverride, NULL);
    (void)FPFwCDDumpManagerSetPostDumpCallback(&crash_dump_ctx, &postDumpCallbackOverride, NULL);
    (void)FPFwCDDumpManagerOverrideGetCurTime(&crash_dump_ctx, &getCurTimeDefault);
}
