//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file exception_trigger.c
 * This file contains generate fw fault exception
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <atu_api.h>
#include <atu_lib.h>
#include <cmsdk_wd.h>
#include <error_domain_i.h>
#include <nvic.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <mcp_top_regs.h>
#include <scp_top_regs.h>
#include <tx_api.h> // for tx_thread_sleep

/*-- Symbolic Constant Macros (defines) --*/
#if defined(SCP_RUNTIME_INIT)
    #define DTCM_DISABLED_EXECUTION_ADDRESS (SCP_TOP_SCP_DATA_RAM_ADDRESS)
    #define MSCP_EXP_RESERVED_ADDRESS       (SCP_TOP_UNUSED10_ADDRESS)
#elif defined(MCP_RUNTIME_INIT)
    #define DTCM_DISABLED_EXECUTION_ADDRESS (MCP_TOP_MCP_DATA_RAM_ADDRESS)
    #define MSCP_EXP_RESERVED_ADDRESS       (MCP_TOP_UNUSED0_ADDRESS)
#endif
/*-------- Function Prototypes -----------*/

/*-------------- Typedefs ----------------*/
typedef void (*mmu_fault_func_ptr)(void);

/*-- Declarations (Statics and globals) --*/

/*-------------- Functions ---------------*/

void trigger_bus_fault()
{
    // Accessing a reserved address in the MSCP_EXP memory map
    volatile uint32_t* reserved_addr = (volatile uint32_t*)MSCP_EXP_RESERVED_ADDRESS;
    MMIO_WRITE32(reserved_addr, 0xBADC0FFE);
}

void trigger_usage_fault()
{
#ifndef _WIN32
    // Invoke undefined instruction in Thumb-2.
    asm volatile(".word 0xf7f0a000\n");
#endif
}

void trigger_hard_fault()
{
    // Bus Fault handler itself causes another fault for Hard Fault
    NVIC_SetVector(UsageFault_IRQn, (uint32_t)trigger_usage_fault);
    trigger_usage_fault();
}

void trigger_mmu_fault()
{
    // Execute code against DISABLE_EXEC attribute section, DTCM
    mmu_fault_func_ptr mmu_fault = (mmu_fault_func_ptr)DTCM_DISABLED_EXECUTION_ADDRESS;
    mmu_fault();
}

void trigger_mscp_watchdog_fault()
{
    // Disable the watchdog regardless of the current state and reenable
    // The minimum valid value for WDOGLOAD is 1
    wdog_cmsdk_apb_disable();
    wdog_cmsdk_apb_init(1, true);
}

void trigger_lockup(void)
{
    /* The processor enters a lockup state if a fault occurs when executing the NMI or HardFault handlers.*/
    // Set the usage and hardfault handlers to our fake ones
    NVIC_SetVector(UsageFault_IRQn, (uint32_t)trigger_usage_fault);
    NVIC_SetVector(HardFault_IRQn, (uint32_t)trigger_usage_fault);

    trigger_usage_fault();
}

void trigger_atu_error(void)
{
    uint64_t atu_error_addr = 0;
    uint32_t atu_translate_addr = 0;

    atu_error_addr = SCP_TOP_ATU_AP_WINDOW_MEM_ADDRESS + SCP_TOP_ATU_AP_WINDOW_MEM_SIZE - (8 * SL_1KB);

    int status = atu_translate_address(ATU_ID_MSCP, atu_error_addr, &atu_translate_addr);
    if (status == 0)
    {
        FPFW_DBGPRINT_ERROR("Inject ATU error test fail, status: %d\n", status);
    }
    else
    {
        // Accessing unmap regions trigger ATU error
        FPFW_DBGPRINT_INFO("Inject ATU error success, status: %d\n", status);
        MMIO_READ32(atu_error_addr);
    }
}