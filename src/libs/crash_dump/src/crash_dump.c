//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file crash_dump.c
 *  Crash dump public API implementation.
 */

/*--------------- Includes ---------------*/
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <cmsis_m7.h>   // for __WFI
#include <crash_dump.h> // for crash_dump_handler
#include <stdint.h>     // for uint32_t

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
core_crash_context_t g_core_crash_context;
static volatile bool s_bug_check_initiated_crash = false;

/*------------- Functions ----------------*/
bool crash_dump_bug_check_initiated_dump()
{
    return s_bug_check_initiated_crash;
}

FPFW_NORETURN void crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    __disable_irq();

    s_bug_check_initiated_crash = true;

#ifndef _WIN32
    // Ensure args are put into specific registers, so they can later be recovered by the debug handler
    __asm__ volatile("mov r0, %[code]\n"
                     "mov r1, %[p1]\n"
                     "mov r2, %[p2]\n"
                     "mov r3, %[p3]\n"
                     "mov r4, %[p4]\n"
                     : // No output
                     : // Inputs
                     [code] "r"(errorCode),
                     [p1] "r"(p1),
                     [p2] "r"(p2),
                     [p3] "r"(p3),
                     [p4] "r"(p4)
                     : // Clobbers
                     "r0", "r1", "r2", "r3", "r4");
#else
    FPFW_UNUSED(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);
#endif
    // Initiate a Debug Monitor exception, which will call the FPFwCDCrashDumpHandler
    __enable_irq();
#ifndef _WIN32
    __asm__ volatile("bkpt \n");
#endif

    // The exception handler will hang the core, but this is needed to respect the noreturn attribute
    crash_dump_wait_forever();
}

/**
 * Hangs the core by WFI indefinitely
 */
__attribute__((__weak__)) FPFW_NORETURN void crash_dump_wait_forever()
{
    while (true)
    {
        __WFI();
    }
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