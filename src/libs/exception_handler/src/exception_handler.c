//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file exception_handler_init.c
 *  Sets up the exception handler for the Cortex-M7
 */

/*------------- Includes -----------------*/
#include "exception_handler_i.h"

#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <bug_check.h>  // for BUG_CHECK
#include <crash_dump.h> // for crash_dump_handler, crash_dump_bug_check_initiated_dump
#include <kng_error.h>  // for KNG_CD_DEFAULT_EXCEPTION, KNG_CD_HARDFAULT_EXCEPTION ...
#include <nvic.h>       // for nvic_set_isr_fault, nvic_irq_set_isr ...
#include <stddef.h>     // for NULL
#include <stdint.h>     // for uint32_t
#include <tx_api.h>     // for TX_THREAD, tx_thread_stack_error_notify

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static void print_context_info(core_crash_context_t* crash_context)
{
    FPFwCDPrintf("Crash Context:\n");
    FPFwCDPrintf("==============\n");

    FPFwCDPrintf("R0 : [%08lX]\n", crash_context->r0);
    FPFwCDPrintf("R1 : [%08lX]\n", crash_context->r1);
    FPFwCDPrintf("R2 : [%08lX]\n", crash_context->r2);
    FPFwCDPrintf("R3 : [%08lX]\n", crash_context->r3);
    FPFwCDPrintf("R4 : [%08lX]\n", crash_context->r4);
    FPFwCDPrintf("R5 : [%08lX]\n", crash_context->r5);
    FPFwCDPrintf("R6 : [%08lX]\n", crash_context->r6);
    FPFwCDPrintf("R7 : [%08lX]\n", crash_context->r7);
    FPFwCDPrintf("R8 : [%08lX]\n", crash_context->r8);
    FPFwCDPrintf("R9 : [%08lX]\n", crash_context->r9);
    FPFwCDPrintf("R10: [%08lX]\n", crash_context->r10);
    FPFwCDPrintf("R11: [%08lX]\n", crash_context->r11);
    FPFwCDPrintf("R12: [%08lX]\n", crash_context->r12);
    FPFwCDPrintf("SP : [%08lX]\n", crash_context->sp);
    FPFwCDPrintf("LR : [%08lX]\n", crash_context->lr);
    FPFwCDPrintf("PC : [%08lX]\n", crash_context->pc);
    FPFwCDPrintf("==============\n");

    FPFwCDPrintf("HFSR  : [%08lX]\n", SCB->HFSR);
    FPFwCDPrintf("CFSR  : [%08lX]\n", SCB->CFSR);
    FPFwCDPrintf("DFSR  : [%08lX]\n", SCB->DFSR);
    FPFwCDPrintf("AFSR  : [%08lX]\n", SCB->AFSR);
    FPFwCDPrintf("MMFAR : [%08lX]\n", SCB->MMFAR);
    FPFwCDPrintf("BFAR  : [%08lX]\n", SCB->BFAR);
    FPFwCDPrintf("==============\n");
}

/**
 * @brief Save the crash context into the global crash context object.
 *
 * @param stack_frame Pointer to the exception stack frame
 */
#ifndef _WIN32
static inline __attribute__((always_inline)) void save_crash_context(exception_stack_frame_t* stack_frame)
#else
__attribute__((__weak__)) void save_crash_context(exception_stack_frame_t* stack_frame)
#endif
{
#ifndef _WIN32
    // Pick up static crash context object
    __asm__ volatile(".global g_core_crash_context   \n"
                     "ldr r12, =g_core_crash_context \n"
                     // Store registers not in stack_frame into crash context
                     "str r4,  [r12, %[r4_offset]]   \n"
                     "str r5,  [r12, %[r5_offset]]   \n"
                     "str r6,  [r12, %[r6_offset]]   \n"
                     "str r7,  [r12, %[r7_offset]]   \n"
                     "str r8,  [r12, %[r8_offset]]   \n"
                     "str r9,  [r12, %[r9_offset]]   \n"
                     "str r10, [r12, %[r10_offset]]  \n"
                     "str r11, [r12, %[r11_offset]]  \n"
                     "str sp,  [r12, %[sp_offset]]   \n"
                     : // No outputs
                     : // Inputs
                     [r4_offset] "i"(offsetof(core_crash_context_t, r4)),
                     [r5_offset] "i"(offsetof(core_crash_context_t, r5)),
                     [r6_offset] "i"(offsetof(core_crash_context_t, r6)),
                     [r7_offset] "i"(offsetof(core_crash_context_t, r7)),
                     [r8_offset] "i"(offsetof(core_crash_context_t, r8)),
                     [r9_offset] "i"(offsetof(core_crash_context_t, r9)),
                     [r10_offset] "i"(offsetof(core_crash_context_t, r10)),
                     [r11_offset] "i"(offsetof(core_crash_context_t, r11)),
                     [sp_offset] "i"(offsetof(core_crash_context_t, sp))
                     : // Clobbers
                     "r12");
#endif
    // Store registers from exception stack frame into crash context
    g_core_crash_context.r0 = stack_frame->R0;
    g_core_crash_context.r1 = stack_frame->R1;
    g_core_crash_context.r2 = stack_frame->R2;
    g_core_crash_context.r3 = stack_frame->R3;
    g_core_crash_context.r12 = stack_frame->R12;
    g_core_crash_context.lr = stack_frame->LR;
    g_core_crash_context.pc = stack_frame->PC;
}

/**
 * @brief Get the active exception number
 *
 * @return int Active exception number
 */
__attribute__((__weak__)) int get_active_exception(void)
{
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) - NVIC_USER_IRQ_OFFSET;
}

/**
 * @brief Exception handler for Cortex-M7. This function is called when an exception occurs.
 *
 * @param stack_frame Pointer to the exception stack frame
 */
void exception_handler(exception_stack_frame_t* stack_frame)
{
    save_crash_context(stack_frame);

    // ToDo: Disable Watchdog

    int32_t errorCode = KNG_CD_DEFAULT_EXCEPTION;
    uint32_t bugCheckParams[4] = {};
    const int exceptionIdx = get_active_exception();

    switch (exceptionIdx)
    {
    case HardFault_IRQn:
        FPFwCDPrintf("Hard Fault occurred\n");
        errorCode = KNG_CD_HARDFAULT_EXCEPTION;
        bugCheckParams[0] = (uint32_t)__FILE__;
        bugCheckParams[1] = __LINE__;
        break;
    case MemoryManagement_IRQn:
        FPFwCDPrintf("Memory Management Fault occurred\n");
        errorCode = KNG_CD_MEMORYMANAGEMENT_EXCEPTION;
        bugCheckParams[0] = (uint32_t)__FILE__;
        bugCheckParams[1] = __LINE__;
        break;
    case BusFault_IRQn:
        FPFwCDPrintf("Bus Fault occurred\n");
        errorCode = KNG_CD_BUSFAULT_EXCEPTION;
        bugCheckParams[0] = (uint32_t)__FILE__;
        bugCheckParams[1] = __LINE__;
        break;
    case UsageFault_IRQn:
        FPFwCDPrintf("Usage Fault occurred\n");
        errorCode = KNG_CD_USAGEFAULT_EXCEPTION;
        bugCheckParams[0] = (uint32_t)__FILE__;
        bugCheckParams[1] = __LINE__;
        break;
    case DebugMonitor_IRQn:
        FPFwCDPrintf("Debug Monitor Exception occurred\n");
        if (crash_dump_bug_check_initiated_dump())
        {
            errorCode = g_core_crash_context.r0;
            bugCheckParams[0] = g_core_crash_context.r1;
            bugCheckParams[1] = g_core_crash_context.r2;
            bugCheckParams[2] = g_core_crash_context.r3;
            bugCheckParams[3] = g_core_crash_context.r4;
        }
        else
        {
            FPFwCDPrintf("CTI-triggered Debug Monitor Exception\n");
            errorCode = KNG_CD_EXTERNAL_REQUEST;
        }
        break;
    case NonMaskableInt_IRQn:
        FPFwCDPrintf("WDT timeout occurred\n");
        errorCode = KNG_CD_WDT_TIMEOUT;
        break;
    default:
        FPFwCDPrintf("other fault occurred\n");
        errorCode = KNG_CD_DEFAULT_EXCEPTION;
        break;
    }

    // Provide printout for debugging
    print_context_info(&g_core_crash_context);

    // Call the crash dump handler
    crash_dump_handler(errorCode, bugCheckParams[0], bugCheckParams[1], bugCheckParams[2], bugCheckParams[3]);

    // Hang the core
    crash_dump_wait_forever();
}

/**
 * @brief Main exception handler for Cortex-M7. This function is called when an exception occurs.
 *
 */
void main_exception_handler(void)
{
    exception_stack_frame_t* stack_frame = NULL;
#ifndef _WIN32
    __asm__ volatile("CPSID   i     \n" // Disable interrupts
                     "tst lr, #4    \n" // Check LR[2] (LR holds EXC_RETURN)
                     "ite eq        \n"
                     "mrseq %0, msp \n"  // Move msp into output register if EXC_RETURN[2] == 0
                     "mrsne %0, psp \n"  // Move psp into output register if EXC_RETURN[2] == 1
                     :                   // Outputs
                     "=r"(stack_frame)); // Point stack_frame to the location of the exception stack frame
#endif
    // Capture stack frame and non-stacked registers, and jump into main handler
    exception_handler(stack_frame);
}

/**
 * @brief ThreadX thread stack error handler. Generate a bug check with the thread pointer.
 *
 */
void threadx_stack_error_handler(TX_THREAD* tx_thread)
{
    BUG_CHECK(KNG_CD_THREAD_STACK_ERROR, tx_thread, 0);
}

/**
 * @brief Register main exception handler for fault ISR and debug monitor and thread stack error.
 *
 */
int32_t exception_handler_init(void)
{
    // Register the main exception handler for generic fault.
    if (nvic_set_isr_fault(main_exception_handler) != NVIC_STATUS_SUCCESS)
    {
        return KNG_E_FAIL;
    }

    // Register the main exception handler for DebugMonitor_IRQn
    NVIC_SetVector(DebugMonitor_IRQn, (uint32_t)main_exception_handler);

    // Register the stack error handler for tx stack error
    if (tx_thread_stack_error_notify(threadx_stack_error_handler) != TX_SUCCESS)
    {
        return KNG_E_FAIL;
    }

    return KNG_SUCCESS;
}
