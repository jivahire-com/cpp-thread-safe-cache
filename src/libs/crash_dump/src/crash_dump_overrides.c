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
#define PRE_DUMP_CB_MAX  16
#define POST_DUMP_CB_MAX 8

/*------------- Typedefs -----------------*/
typedef struct
{
    void (*callback_fn)(void*);
    void* callback_ctx;
} dump_callback_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static dump_callback_t pre_dump_callbacks[PRE_DUMP_CB_MAX];
static uint32_t pre_dump_cb_count = 0;

static dump_callback_t post_dump_callbacks[POST_DUMP_CB_MAX];
static uint32_t post_dump_cb_count = 0;

/*------------- Functions ----------------*/
void crash_dump_register_pre_dump_callback(void cb(void*), void* ctx)
{
    FPFW_RUNTIME_ASSERT(pre_dump_cb_count < PRE_DUMP_CB_MAX - 1);
    FPFW_RUNTIME_ASSERT(cb != NULL);

    pre_dump_callbacks[pre_dump_cb_count].callback_fn = cb;
    pre_dump_callbacks[pre_dump_cb_count].callback_ctx = ctx;
    pre_dump_cb_count++;
}

void crash_dump_register_post_dump_callback(void cb(void*), void* ctx)
{
    FPFW_RUNTIME_ASSERT(post_dump_cb_count < POST_DUMP_CB_MAX - 1);
    FPFW_RUNTIME_ASSERT(cb != NULL);

    post_dump_callbacks[post_dump_cb_count].callback_fn = cb;
    post_dump_callbacks[post_dump_cb_count].callback_ctx = ctx;
    post_dump_cb_count++;
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
    FPFW_UNUSED(preDumpCtx);

    for (uint32_t i = 0; i < pre_dump_cb_count; i++)
    {
        pre_dump_callbacks[i].callback_fn(pre_dump_callbacks[i].callback_ctx);
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
    FPFW_UNUSED(postDumpCtx);

    for (uint32_t i = 0; i < post_dump_cb_count; i++)
    {
        post_dump_callbacks[i].callback_fn(post_dump_callbacks[i].callback_ctx);
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
