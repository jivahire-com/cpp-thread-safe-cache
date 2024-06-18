//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_payload_core.c
 * Registering common core data to put in crash dump
 */

/*--------------- Includes ---------------*/
#include <crash_dump.h>               // for GetCrashDumpContext, g_core_cr...
#include <modules/CdDumpDescriptor.h> // for _FPFwCdDumpPriority
#include <modules/CdDumpManager.h>    // for CdRegisterAddress32, CdRegiste...
#include <stdint.h>                   // for uint8_t

/*-- Symbolic Constant Macros (defines) --*/
#define CORE_BUILTIN_REG_COUNT 16
#define CORE_BUILTIN_REG_INDEX 0

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
extern uint8_t __stack_start__;
extern uint8_t __stack_end__;

/*------------- Functions ----------------*/

/**
 * @brief Captures built-in Cortex M7 registers
 */
void crash_dump_register_core_registers()
{
    CdRegisterRegisterSet(GetCrashDumpContext(), &g_core_crash_context, CORE_BUILTIN_REG_INDEX, CORE_BUILTIN_REG_COUNT, FPFW_CD_DUMP_PRIORITY_CRITICAL);
}

/**
 * @brief Captures system stack
 */
void crash_dump_register_core_stack()
{
    CdRegisterAddress32(GetCrashDumpContext(), &__stack_start__, &__stack_end__ - &__stack_start__, FPFW_CD_DUMP_PRIORITY_CRITICAL);
}
