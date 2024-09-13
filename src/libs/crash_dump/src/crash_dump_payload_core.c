//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_payload_core.c
 * Registering common core data to put in crash dump
 */

/*--------------- Includes ---------------*/
#include <cmsis_m7.h>                 // for SCB
#include <crash_dump.h>               // for GetCrashDumpContext, g_core_cr...
#include <modules/CdDumpDescriptor.h> // for _FPFwCdDumpPriority
#include <modules/CdDumpManager.h>    // for CdRegisterAddress32, CdRegiste...
#include <stdint.h>                   // for uint8_t
#include <stdio.h>                    // for printf

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

/**
 * @brief Captures default crash registers
 *
 * @param mmio_registers Array of core_register_mmio_t
 * @param mmio_register_count Number of elements in mmio_registers
 */
void crash_dump_register_default_registers(const core_register_mmio_t* mmio_registers, uint32_t mmio_register_count)
{
    if (mmio_registers != NULL)
    {
        // Register platform specific registers configured by init configuration.
        for (uint32_t i = 0; i < mmio_register_count; i++)
        {
            crash_dump_register_mmio_register(mmio_registers[i].address,
                                              mmio_registers[i].count,
                                              mmio_registers[i].priority);
        }
    }

    // Register Exception status registers
    crash_dump_register_mmio_register((volatile void*)(&SCB->MMFAR), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    crash_dump_register_mmio_register((volatile void*)(&SCB->BFAR), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    crash_dump_register_mmio_register((volatile void*)(&SCB->HFSR), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    crash_dump_register_mmio_register((volatile void*)(&SCB->CFSR), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL);
}