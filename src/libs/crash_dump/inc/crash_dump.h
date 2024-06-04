//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file crash_dump.h
 *  Public API for crash dump
 */

#pragma once

/*--------------- Includes ---------------*/
#include <modules/CdDumpManager.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
FPFwCrashDumpCtx *GetCrashDumpContext();

void crash_dump_init();

void crash_dump_handler(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4);
