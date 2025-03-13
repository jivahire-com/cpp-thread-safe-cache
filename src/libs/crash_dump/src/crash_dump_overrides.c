//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_overrides.c
 * File containing all crash dump override functions for the framework
 */

/*------------- Includes -----------------*/
#include "crash_dump_overrides.h"

#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <crash_dump.h> // for GetCrashDumpConfig
#include <stdarg.h>     // for va_list, va_start, va_end
#include <stdbool.h>    // for bool
#include <stdint.h>     // for uint32_t, uint64_t
#include <stdio.h>      // for printf
#include <tx_api.h>     // for tx_mutex_get, tx_mutex_put

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void crash_dump_register_pre_dump_callback(void cb(void*), void* ctx, crash_dump_type_t dump_type)
{
    FPFW_RUNTIME_ASSERT(cb != NULL);
    crash_dump_context_t* cd_ctx = crash_dump_context();

    for (int i = 0; i < CRASH_DUMP_TYPE_NUM; i++)
    {
        if (dump_type == CRASH_DUMP_TYPE_ALL || dump_type == i)
        {
            crash_dump_type_callback_t* callback = &cd_ctx->callbacks[i];
            FPFW_RUNTIME_ASSERT(callback->pre_dump_cb_count < PRE_DUMP_CB_MAX - 1);

            callback->pre_dump_callbacks[callback->pre_dump_cb_count].callback_fn = cb;
            callback->pre_dump_callbacks[callback->pre_dump_cb_count].callback_ctx = ctx;
            callback->pre_dump_cb_count++;
        }
    }
}

void crash_dump_register_post_dump_callback(void cb(void*), void* ctx, crash_dump_type_t dump_type)
{
    FPFW_RUNTIME_ASSERT(cb != NULL);
    crash_dump_context_t* cd_ctx = crash_dump_context();

    for (int i = 0; i < CRASH_DUMP_TYPE_NUM; i++)
    {
        if (dump_type == CRASH_DUMP_TYPE_ALL || dump_type == i)
        {
            crash_dump_type_callback_t* callback = &cd_ctx->callbacks[i];
            FPFW_RUNTIME_ASSERT(callback->post_dump_cb_count < POST_DUMP_CB_MAX - 1);

            callback->post_dump_callbacks[callback->post_dump_cb_count].callback_fn = cb;
            callback->post_dump_callbacks[callback->post_dump_cb_count].callback_ctx = ctx;
            callback->post_dump_cb_count++;
        }
    }
}

/*------- Memory Pool Overrides -------*/
void cacheFlushOverride(uint64_t addr, uint32_t size)
{
    FPFW_UNUSED(addr);
    FPFW_UNUSED(size);
}

void cacheInvalidateOverride(uint64_t addr, uint32_t size)
{
    FPFW_UNUSED(addr);
    FPFW_UNUSED(size);
}

/*------- Dump Descriptor Overrides -------*/
uint32_t mutexLockOverride(void* mutex_ctx)
{
    return tx_mutex_get((TX_MUTEX*)mutex_ctx, TX_WAIT_FOREVER);
}

uint32_t mutexUnlockOverride(void* mutex_ctx)
{
    return tx_mutex_put((TX_MUTEX*)mutex_ctx);
}

/*------- Dump File Overrides -------*/
bool inMemoryOverride(void* addr, uint32_t size)
{
    uintptr_t end_address = (uintptr_t)addr;
    if (size > 0)
    {
        end_address += (size - 1);
    }

    // only return true if start and end addresses in valid memory space
    return (uintptr_t)addr <= end_address ? crash_dump_context()->in_memory((uintptr_t)addr, end_address) : false;
}

bool inGlobalMemoryOverride(uint64_t addr, uint32_t size)
{
    FPFW_UNUSED(addr);
    FPFW_UNUSED(size);

    return true;
}

/*------- Dump Manager Overrides -------*/
/**
 * @brief Crash dump pre-dump callback override to call all registered pre-dump callbacks
 *
 * @param preDumpCtx
 * @return true if succeeded, otherwise false
 */
bool preDumpCallbackOverride(void* preDumpCtx)
{
    crash_dump_context_t* ctx = crash_dump_context();
    crash_dump_type_t type = ((crash_dump_type_context_t*)preDumpCtx)->type;
    crash_dump_type_callback_t* callback = &ctx->callbacks[type];

    for (uint32_t i = 0; i < callback->pre_dump_cb_count; i++)
    {
        callback->pre_dump_callbacks[i].callback_fn(callback->pre_dump_callbacks[i].callback_ctx);
    }

    return true;
}

/**
 * @brief ToDo: Implement this function with the actual post dump implementation
 *
 * @param postDumpCtx
 * @return true if succeeded, otherwise false
 */
bool postDumpCallbackOverride(void* postDumpCtx)
{
    crash_dump_context_t* ctx = crash_dump_context();
    crash_dump_type_t type = ((crash_dump_type_context_t*)postDumpCtx)->type;
    crash_dump_type_callback_t* callback = &ctx->callbacks[type];

    for (uint32_t i = 0; i < callback->post_dump_cb_count; i++)
    {
        callback->post_dump_callbacks[i].callback_fn(callback->post_dump_callbacks[i].callback_ctx);
    }

    return true;
}

/**
 * @brief ToDo: Implement this function with the actual time implementation
 *
 * @return
 */
uint64_t getCurTimeDefault(void)
{
    return 0;
}

/*------- Utility Overrides -------*/
int crash_dump_printf(const char* format, ...)
{
    // ToDo: Implement this function with the actual printf implementation
    size_t length = 0;
    va_list args;

    va_start(args, format);
    length = vprintf(format, args);
    va_end(args);

    return length;
}
