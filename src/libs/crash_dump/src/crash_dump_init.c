//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file crash_dump_init.c
 *  Crash dump initialization.
 */

/*--------------- Includes ---------------*/
#include "crash_dump_context.h"   // for set_crash_dump_context
#include "crash_dump_gpio.h"      // for cd_gpio_assert_cd_in_progress
#include "crash_dump_overrides.h" // for cacheFlushOverride, cacheInvalidateOverride
#include "crash_dump_payload.h"   // for crash_dump_register_core_registers
#include "crash_dump_status.h"    // for crash_dump_update_state, crash_dump_update_core_state

#include <FpFwAssert.h>               // for FPFW_RUNTIME_ASSERT
#include <crash_dump.h>               // for crash_dump_init
#include <crash_dump_events.h>        // for CRASH_DUMP_ET
#include <idsw_kng.h>                 // for DIE_0, DIE_1
#include <modules/CdDumpDescriptor.h> // for FPFwCDInitDumpDescriptor
#include <modules/CdMemoryPool.h>     // for FPFwCDInintMemoryPool
#include <stdbool.h>                  // for false

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-------- Function Prototypes -----------*/
static void init_mem_pool(crash_dump_type_context_t* type_context);
static void init_dump_desc(crash_dump_type_context_t* type_context);
static void init_dump_file(crash_dump_type_context_t* type_context);
static void init_dump_manager(crash_dump_type_context_t* type_context);

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
/**
 * @brief Start initialization of crash dump.
 *
 * @param config Configuration for crash dump.
 *
 */
void crash_dump_init(crash_dump_context_t* context)
{
    // Cache the main cd context.
    set_crash_dump_context(context);

    // De-assert CD_IN_PROGRESS
    cd_gpio_assert_cd_in_progress(false);
}

/**
 * @brief Register mini dump or full dump
 *
 * @param type_context
 * @return KNG_SUCCESS if succeeded, otherwise error code.
 */
KNG_STATUS crash_dump_register_dump(crash_dump_type_context_t* type_context)
{
    crash_dump_context_t* ctx = crash_dump_context();

    if (ctx == NULL)
    {
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_INIT_INVALID_ADDRESS, KNG_E_NOT_READY);
        return KNG_E_NOT_READY;
    }

    if (type_context == NULL || type_context->header == NULL || type_context->type >= CRASH_DUMP_TYPE_NUM)
    {
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_INIT_INVALID_ADDRESS, KNG_E_INVALIDARG);
        return KNG_E_INVALIDARG;
    }

    if (ctx->type_ctx[type_context->type] == NULL)
    {
        ctx->type_ctx[type_context->type] = type_context;
    }
    else
    {
        return KNG_E_ALREADY_INITIALIZED;
    }

    // Initialize crash dump header.
    initialize_crash_dump_header(type_context);

    // Configure FPFW crash dump context
    // Set Processor ID with DIE index (Upper 16 bits) and core index (Lower 16 bits).
    type_context->crash_dump_ctx.prid = CRASH_DUMP_PROCESSOR_ID(ctx->die_index, ctx->core_index);
    type_context->crash_dump_ctx.isPrimaryCore = ctx->is_primary;

    // Initialize crash dump framework
    init_dump_desc(type_context);
    init_mem_pool(type_context);
    init_dump_file(type_context);
    init_dump_manager(type_context);

    // Register core built-in registers into crash dump
    crash_dump_register_core_registers(type_context);

    // Register error status registers and other diagnostic registers
    crash_dump_register_default_registers(type_context, ctx->mmio_registers, ctx->mmio_register_count);

    // Register core stack
    crash_dump_register_core_stack(type_context);

    // Add capture information about the core and the firmware
    crash_dump_register_standard_info(type_context);

    if (type_context->type == CRASH_DUMP_TYPE_FULL)
    {
        // Register ThreadX data registerer callback
        crash_dump_register_threadx(type_context);
    }

    return KNG_SUCCESS;
}

/**
 * @brief Initialize crash dump memory pool.
 *
 * @param cd_mem_pool Crash dump memory pool address.
 * @param block_size Size of the memory pool.
 *
 */
static void init_mem_pool(crash_dump_type_context_t* type_context)
{
    // Initialize crash dump memory pool
    FPFW_RUNTIME_ASSERT(FPFwCDInitMemoryPool(&type_context->mem_ctx, type_context->mem_pool_addr, type_context->mem_pool_size));
    (void)FPFwCDMemPoolOverrideCacheFlush(&type_context->mem_ctx, &cacheFlushOverride);
    (void)FPFwCDMemPoolOverrideCacheInvalidate(&type_context->mem_ctx, &cacheInvalidateOverride);
}

/**
 * @brief Initialize crash dump description.
 *
 */
static void init_dump_desc(crash_dump_type_context_t* type_context)
{
    // Create Tx mutex for descriptor set
    FPFW_RUNTIME_ASSERT(tx_mutex_create(&type_context->desc_mutex,
                                        type_context->type == CRASH_DUMP_TYPE_MINI ? "cd mini mutex" : "cd full mutex",
                                        TX_NO_INHERIT) == TX_SUCCESS);

    // Create crash dump descriptor
    FPFW_RUNTIME_ASSERT(FPFwCDInitDumpDescriptor(&type_context->desc_ctx, type_context->desc_list, CRASH_DUMP_NUM_DESCRIPTORS));

    (void)FPFwCDDumpDescriptorSetMutexCtx(&type_context->desc_ctx, (void*)&type_context->desc_mutex);
    (void)FPFwCDDumpDescriptorOverrideMutexLock(&type_context->desc_ctx, &mutexLockOverride);
    (void)FPFwCDDumpDescriptorOverrideMutexUnlock(&type_context->desc_ctx, &mutexUnlockOverride);
}

/**
 * @brief Initialize crash dump file.
 *
 */
static void init_dump_file(crash_dump_type_context_t* type_context)
{
    FPFW_RUNTIME_ASSERT(FPFwCDInitDumpFile(&type_context->file_ctx));
    (void)FPFwCDDumpFileOverrideInValidMemory(&type_context->file_ctx, &inMemoryOverride);
    (void)FPFwCDDumpFileOverrideInValidCsrMemory(&type_context->file_ctx, &inMemoryOverride);
    (void)FPFwCDDumpFileOverrideInValidGlobalMemory(&type_context->file_ctx, &inGlobalMemoryOverride);
    type_context->file_ctx.product = CD_PRODUCT_ID_KINGSGATE;
}

/**
 * @brief Initialize crash dump manager.
 *
 */
static void init_dump_manager(crash_dump_type_context_t* type_context)
{
    FPFW_RUNTIME_ASSERT(FPFwCDInitDumpManager(&type_context->crash_dump_ctx,
                                              &type_context->mem_ctx,
                                              &type_context->desc_ctx,
                                              &type_context->file_ctx,
                                              NULL, // No state manager
                                              type_context->mem_pool_size));
    FPFwCDOverridePrintf(&crash_dump_printf);
    (void)FPFwCDDumpManagerSetPreDumpCallback(&type_context->crash_dump_ctx, &preDumpCallbackOverride, type_context);
    (void)FPFwCDDumpManagerSetPostDumpCallback(&type_context->crash_dump_ctx, &postDumpCallbackOverride, type_context);
    (void)FPFwCDDumpManagerOverrideGetCurTime(&type_context->crash_dump_ctx, &getCurTimeDefault);
}
