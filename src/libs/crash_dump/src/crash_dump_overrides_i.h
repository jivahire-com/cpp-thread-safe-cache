//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_overrides_i.h
 * File containing all crash dump override functions for the framework
 */
#pragma once

/*----------- Nested includes ------------*/
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/*------- Memory Pool Overrides -------*/
void get_crash_dump_mem_pool(uint64_t *mem_pool_addr, uint32_t *size);
void cacheFlushOverride(uint64_t addr, uint32_t size);
void cacheInvalidateOverride(uint64_t addr, uint32_t size);

/*------- Dump Descriptor Overrides -------*/
uint32_t mutexLockOverride(void *mutex_ctx);
uint32_t mutexUnlockOverride(void *mutex_ctx);