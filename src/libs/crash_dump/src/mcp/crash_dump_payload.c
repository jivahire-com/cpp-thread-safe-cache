//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_payload.c
 * Registering MCP data to put in crash dump
 */

/*--------------- Includes ---------------*/
#include <cmsis_m7.h>
#include <crash_dump.h>
#include <modules/CdDumpDescriptor.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
/**
 * @brief Captures default crash registers
 */
void crash_dump_register_default_registers()
{
    // https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484991
    // ToDo: Add SCP_EXP registers
    // ToDo: Add Watchdog registers
    // ToDo: Add Power Control registers

    // Exception status registers
    crash_dump_register_mmio_register((volatile void*)(&SCB->MMFAR), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    crash_dump_register_mmio_register((volatile void*)(&SCB->BFAR), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    crash_dump_register_mmio_register((volatile void*)(&SCB->HFSR), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    crash_dump_register_mmio_register((volatile void*)(&SCB->CFSR), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL);
}
