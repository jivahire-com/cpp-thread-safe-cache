//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_overrides.c
 * File containing all crash dump override functions for the framework
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <stdint.h>    // for uint32_t, uint64_t
#include <tx_api.h>    // for tx_mutex_get, tx_mutex_put

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

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