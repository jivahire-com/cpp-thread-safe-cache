//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_overrides.h
 * File containing all crash dump override functions for the framework
 */
#pragma once

/*----------- Nested includes ------------*/
#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/*------- Memory Pool Overrides -------*/
void cacheFlushOverride(uint64_t addr, uint32_t size);
void cacheInvalidateOverride(uint64_t addr, uint32_t size);

/*------- Dump Descriptor Overrides -------*/
uint32_t mutexLockOverride(void *mutex_ctx);
uint32_t mutexUnlockOverride(void *mutex_ctx);

/*------- Dump File Overrides -------*/
bool inMemoryOverride(void *addr, uint32_t size);
bool inGlobalMemoryOverride(uint64_t addr, uint32_t size);

/*------- Dump Manager Overrides -------*/
bool preDumpCallbackOverride(void *preDumpCtx);
bool postDumpCallbackOverride(void *postDumpCtx);
uint64_t getCurTimeDefault(void);

/*------- Utility Overrides -------*/
int crash_dump_printf(const char *format, ...);

/*------- Memory APIs Overrides --------*/
void crash_dump_aligned_write16(uint64_t addr, uint16_t value);
void crash_dump_aligned_write32(uint64_t addr, uint32_t value);
void crash_dump_aligned_write64(uint64_t addr, uint64_t value);
uint16_t crash_dump_aligned_read16(uint64_t addr);
uint32_t crash_dump_aligned_read32(uint64_t addr);
uint64_t crash_dump_aligned_read64(uint64_t addr);
