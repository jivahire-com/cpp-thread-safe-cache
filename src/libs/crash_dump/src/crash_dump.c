//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file crash_dump.c
 *  Crash dump public API implementation.
 */

/*--------------- Includes ---------------*/
#include "crash_dump_gpio.h" // for cd_gpio_assert_cd_in_progress
#include "crash_dump_icc.h"
#include "crash_dump_status.h" // for crash_dump_update_core_state

#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <cmsis_m7.h>   // for __WFI
#include <crash_dump.h> // for crash_dump_handler
#include <nvic.h>       // for nvic_get_current_irq
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

FPFW_NORETURN void crash_dump_bug_check_external()
{
    uint32_t nvic_irq_num = 0;
    nvic_status_t nvic_status = nvic_get_current_irq(&nvic_irq_num);

    if (nvic_status == NVIC_STATUS_SUCCESS)
    {
        // Lower the interrupt priority to 1 if it is called by the other ISR.
        // This is to ensure DebugMonitor_IRQn interrupt, otherwise HardFault will occur.
        NVIC_SetPriority(nvic_irq_num, 1);
    }
#ifndef _WIN32
    __asm__ volatile("bkpt\n");
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
    // Update the crash dump state
    crash_dump_update_core_state(CRASH_DUMP_STATE_IN_PROGRESS);

    // Assert GPIO_CD_IN_PROGRESS
    cd_gpio_assert_cd_in_progress(true);

    // Trigger remote entities to indicate this core has crashed if needed
    crash_dump_remote_trigger();

    FPFwCrashDumpCtx* crash_dump_context = GetCrashDumpContext();
    FPFwCdBugCheckInfo bug_check_info = {};
    bug_check_info.coreIndex = crash_dump_context->coreIndex;
    bug_check_info.data.Code = errorCode;
    bug_check_info.data.Parameter[0] = p1;
    bug_check_info.data.Parameter[1] = p2;
    bug_check_info.data.Parameter[2] = p3;
    bug_check_info.data.Parameter[3] = p4;

    FPFwCDCrashDumpHandler(crash_dump_context, &g_core_crash_context, &bug_check_info);

    // Update the crash dump state
    crash_dump_update_core_state(CRASH_DUMP_STATE_COMPLETED);
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