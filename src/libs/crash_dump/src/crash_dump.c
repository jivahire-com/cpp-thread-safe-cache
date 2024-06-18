//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file crash_dump.c
 *  Crash dump public API implementation.
 */

/*--------------- Includes ---------------*/
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <crash_dump.h> // for crash_dump_handler
#include <stdint.h>     // for uint32_t

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
core_crash_context_t g_core_crash_context;

__attribute__((__weak__)) NORETURN void crash_dump_wait_forever();

/*------------- Functions ----------------*/
NORETURN void crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    FPFW_UNUSED(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);

    // The exception handler will hang the core, but this is needed to respect the noreturn attribute
    crash_dump_wait_forever();
}

/**
 * Hangs the core by WFI indefinitely
 */
__attribute__((__weak__)) NORETURN void crash_dump_wait_forever()
{
#ifndef UNIT_TEST
    while (1)
    {
        __asm__ volatile("wfi"); // Wait for interrupt
    }
#endif
}

void crash_dump_handler(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    // ToDo: Implement crash dump handler
    //       https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484988
    FPFW_UNUSED(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);
}

void crash_dump_register_mmio_register(volatile void* mmio_reg, uint32_t reg_count, FPFwCdDumpPriority priority)
{
    CdRegisterMMIORegisterSet(GetCrashDumpContext(), (uintptr_t)mmio_reg, reg_count, priority);
}

void crash_dump_register_address32(void* address, uint32_t size, FPFwCdDumpPriority priority)
{
    CdRegisterAddress32(GetCrashDumpContext(), address, size, priority);
}

void crash_dump_register_address64(uint64_t address, uint32_t size, FPFwCdDumpPriority priority)
{
    CdRegisterAddress64(GetCrashDumpContext(), address, size, priority);
}

void crash_dump_register_address32_pointer_array(FPFwCdDumpPriority priority,
                                                 uint32_t minChunkSize,
                                                 uint32_t maxRegistrationCount,
                                                 void** pointerArray,
                                                 uint32_t pointerArrayCount)
{
    CdRegisterAddress32PointerArray(GetCrashDumpContext(), priority, minChunkSize, maxRegistrationCount, pointerArray, pointerArrayCount);
}