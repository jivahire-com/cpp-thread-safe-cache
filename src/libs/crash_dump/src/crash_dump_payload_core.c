//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_payload_core.c
 * Registering common core data to put in crash dump
 */

/*--------------- Includes ---------------*/
#include <cmsis_m7.h>                 // for SCB
#include <crash_dump.h>               // for crash_dump_type_context_t
#include <modules/CdDumpDescriptor.h> // for _FPFwCdDumpPriority
#include <modules/CdDumpManager.h>    // for CdRegisterAddress32, CdRegiste...

/*-- Symbolic Constant Macros (defines) --*/
#define CORE_BUILTIN_REG_INDEX 0

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
extern uint8_t __stack_start__;
extern uint8_t __stack_end__;

/*------------- Functions ----------------*/

/**
 * @brief Captures built-in Cortex M7 registers
 *
 * #param type_context crash dump type based context
 */
void crash_dump_register_core_registers(crash_dump_type_context_t* type_context)
{
    if (type_context)
    {
        CdRegisterRegisterSet(&type_context->crash_dump_ctx,
                              &g_core_crash_context,
                              CORE_BUILTIN_REG_INDEX,
                              sizeof(g_core_crash_context) / sizeof(uint32_t),
                              FPFW_CD_DUMP_PRIORITY_CRITICAL);
    }
}

/**
 * @brief Captures system stack
 *
 * #param type_context crash dump type based context
 */
void crash_dump_register_core_stack(crash_dump_type_context_t* type_context)
{
    if (type_context)
    {
        CdRegisterAddress32(&type_context->crash_dump_ctx, &__stack_start__, &__stack_end__ - &__stack_start__, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    }
}

/**
 * @brief Captures default crash registers
 *
 * #param type_context crash dump type based context
 * @param mmio_registers Array of core_register_mmio_t
 * @param mmio_register_count Number of elements in mmio_registers
 */
void crash_dump_register_default_registers(crash_dump_type_context_t* type_context,
                                           const core_register_mmio_t* mmio_registers,
                                           uint32_t mmio_register_count)
{
    if (mmio_registers != NULL && type_context != NULL)
    {
        // Register platform specific registers configured by init configuration.
        for (uint32_t i = 0; i < mmio_register_count; i++)
        {
            CdRegisterMMIORegisterSet(&type_context->crash_dump_ctx,
                                      (uint32_t)mmio_registers[i].address,
                                      mmio_registers[i].count,
                                      mmio_registers[i].priority);
        }
    }

    // Register Exception status registers
    CdRegisterMMIORegisterSet(&type_context->crash_dump_ctx, (uint32_t)&SCB->MMFAR, 1, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    CdRegisterMMIORegisterSet(&type_context->crash_dump_ctx, (uint32_t)&SCB->BFAR, 1, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    CdRegisterMMIORegisterSet(&type_context->crash_dump_ctx, (uint32_t)&SCB->HFSR, 1, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    CdRegisterMMIORegisterSet(&type_context->crash_dump_ctx, (uint32_t)&SCB->CFSR, 1, FPFW_CD_DUMP_PRIORITY_CRITICAL);
}