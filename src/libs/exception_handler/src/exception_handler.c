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
#include <atu_api.h>
#include <atu_lib.h>
#include <bug_check.h> // for BUG_CHECK
#include <cmsdk_wd.h>  // for wdog_cmsdk_apb_disable
#include <cper.h>
#include <crash_dump.h> // for crash_dump_handler, crash_dump_bug_check_initiated_dump
#include <health_monitor.h>
#include <idsw_kng.h>
#include <interrupts.h>
#include <kng_atu_mappings.h> // for ATU_MAPPING_SDMSS_BASE, ATU_MAPPING_SDM_RCIEP_ECAM_BASE ...
#include <kng_error.h>        // for KNG_CD_DEFAULT_EXCEPTION, KNG_CD_HARDFAULT_EXCEPTION ...
#include <mscp_error_domain.h>
#include <nvic.h> // for nvic_set_isr_fault, nvic_irq_set_isr ...
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <scp_top_regs.h>
#include <shared_sram_ecc_ras_registers_regs.h>
#include <stddef.h> // for NULL
#include <stdint.h> // for uint32_t
#include <tx_api.h> // for TX_THREAD, tx_thread_stack_error_notify

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
    FPFwCDPrintf("xPSR: [%08lX]\n", crash_context->xpsr);
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
                     : // No outputs
                     : // Inputs
                     [r4_offset] "i"(offsetof(core_crash_context_t, r4)),
                     [r5_offset] "i"(offsetof(core_crash_context_t, r5)),
                     [r6_offset] "i"(offsetof(core_crash_context_t, r6)),
                     [r7_offset] "i"(offsetof(core_crash_context_t, r7)),
                     [r8_offset] "i"(offsetof(core_crash_context_t, r8)),
                     [r9_offset] "i"(offsetof(core_crash_context_t, r9)),
                     [r10_offset] "i"(offsetof(core_crash_context_t, r10)),
                     [r11_offset] "i"(offsetof(core_crash_context_t, r11))
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
    g_core_crash_context.xpsr = stack_frame->PSR;
    g_core_crash_context.sp = (uint32_t)(stack_frame) + sizeof(exception_stack_frame_t);
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

#ifdef SCP_RUNTIME_INIT
// #define DUMP_ARSM_ECC_STATUS_VERBOSE 1
static bool check_shared_sram_ecc_ras_fault(void)
{
    for (mscp_arsm_ram_type_t i = MSCP_S_ARSM_RAM; i < MSCP_ARSM_RAM_COUNT; i++)
    {
        atu_map_entry_t atu_entry;
        get_arsm_ecc_atu_entry(i, &atu_entry);
        atu_map(ATU_ID_MSCP, &atu_entry);
        uint32_t err_status =
            MMIO_READ32(atu_entry.mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ADDRESS);
        uint32_t err_addr =
            MMIO_READ32(atu_entry.mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRADDR_ADDRESS);

        if (err_status != 0)
        {
            bool is_pending = false;
            nvic_is_irq_pending(get_irq_num_for_scp_ecc_isr(i), &is_pending);

            FPFwCDPrintf("type_%d : err_status = 0x%08lx at addr = 0x%08lx, irq = %s\n",
                         i,
                         err_status,
                         err_addr,
                         is_pending ? "PENDING" : "CLEAR");
    #ifdef DUMP_ARSM_ECC_STATUS_VERBOSE
            FPFwCDPrintf("type_%d : SERR = 0x%08lx\n", i, err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_SERR_MASK);
            FPFwCDPrintf("type_%d : UET = 0x%08lx\n",
                         i,
                         (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UET_MASK) >>
                             SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UET_LSB);
            FPFwCDPrintf("type_%d : PN = 0x%08lx\n",
                         i,
                         (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_PN_MASK) >>
                             SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_PN_LSB);
            FPFwCDPrintf("type_%d : DE = 0x%08lx\n",
                         i,
                         (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_DE_MASK) >>
                             SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_DE_LSB);
            FPFwCDPrintf("type_%d : CE = 0x%08lx\n",
                         i,
                         (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK) >>
                             SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_LSB);
            FPFwCDPrintf("type_%d : MV = 0x%08lx\n",
                         i,
                         (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_MV_MASK) >>
                             SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_MV_LSB);
            FPFwCDPrintf("type_%d : OF = 0x%08lx\n",
                         i,
                         (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK) >>
                             SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_LSB);
            FPFwCDPrintf("type_%d : ER = 0x%08lx\n",
                         i,
                         (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ER_MASK) >>
                             SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ER_LSB);
            FPFwCDPrintf("type_%d : UE = 0x%08lx\n",
                         i,
                         (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK) >>
                             SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_LSB);
            FPFwCDPrintf("type_%d : V = 0x%08lx\n",
                         i,
                         (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK) >>
                             SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_LSB);
            FPFwCDPrintf("type_%d : AV = 0x%08lx\n",
                         i,
                         (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_MASK) >>
                             SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_LSB);
    #endif
        }
        atu_unmap(ATU_ID_MSCP, &atu_entry);

        if (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK) // SRAMECC_ERRSTATUS is valid
        {
            return true; // ECC error detected
        }
    }

    return false; // No ECC error detected
}
#endif

/**
 * @brief Exception handler for Cortex-M7. This function is called when an exception occurs.
 *
 * @param stack_frame Pointer to the exception stack frame
 */
void exception_handler(exception_stack_frame_t* stack_frame)
{
    save_crash_context(stack_frame);

#ifdef SCP_RUNTIME_INIT
    bool is_shared_sram_ecc_fault = check_shared_sram_ecc_ras_fault();
    FPFwCDPrintf("Shared SRAM ECC RAS Fault: %s\n", is_shared_sram_ecc_fault ? "Detected" : "Not Detected");
#endif

    // Disable Watchdog
    wdog_cmsdk_apb_disable();         // Disable watchdog
    wdog_cmsdk_apb_lock_unlock(true); // Lock counter

    int32_t errorCode = KNG_CD_DEFAULT_EXCEPTION;
    uint32_t bugCheckParams[4] = {};
    const int exceptionIdx = get_active_exception();

#if defined(MCP_RUNTIME_INIT)
    // acpi_error_domain_t err_domain = ACPI_ERROR_DOMAIN_MCP_PROC;
    // const guid_t status_guid = exceptionIdx == NonMaskableInt_IRQn ? (guid_t)MCP_WD : (guid_t)MCP_EXCEPTION;
#elif defined(SCP_RUNTIME_INIT)
    acpi_error_domain_t err_domain = ACPI_ERROR_DOMAIN_SCP_PROC;
    const guid_t status_guid = exceptionIdx == NonMaskableInt_IRQn ? (guid_t)SCP_WD : (guid_t)SCP_EXCEPTION;
#endif

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

#ifdef SCP_RUNTIME_INIT
    // Send CPER
    acpi_err_sec_firmware_t sec_fw_cper_section = {
        .severity = ACPI_ERROR_SEVERITY_CORRECTED,
        .record_id = status_guid,
        .param = {errorCode, bugCheckParams[0], bugCheckParams[1], bugCheckParams[2]}};

    acpi_cper_section_t cper_section;
    cper_section.sec_fw = sec_fw_cper_section;

    hm_submit_cper(err_domain, ACPI_ERROR_SEVERITY_CORRECTED, &cper_section, sizeof(cper_section));
#endif

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
KNG_STATUS exception_handler_init(void)
{
    // Register the main exception handler for generic fault.
    if (nvic_set_isr_fault(main_exception_handler) != NVIC_STATUS_SUCCESS)
    {
        return KNG_E_FAIL;
    }

    // Register the main exception handler for DebugMonitor_IRQn
    NVIC_SetVector(DebugMonitor_IRQn, (uint32_t)main_exception_handler);

    // Register the watchdog timeout handler for NonMaskableInt_IRQn
    NVIC_SetVector(NonMaskableInt_IRQn, (uint32_t)main_exception_handler);

    // Register the stack error handler for tx stack error
    if (tx_thread_stack_error_notify(threadx_stack_error_handler) != TX_SUCCESS)
    {
        return KNG_E_FAIL;
    }

    return KNG_SUCCESS;
}
