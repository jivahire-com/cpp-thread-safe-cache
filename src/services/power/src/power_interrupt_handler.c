//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_interrupt_handling.c
 * This file contains handling of PLL interrupts in power service.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <cper.h>
#include <fgpll_regs.h>
#include <interrupts.h>
#include <nvic.h>
#include <pex_regs.h>
#include <power_runconfig.h>
#include <power_runconfig_i.h>
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <scp_top_regs.h>
#include <stdio.h>

// clang-format off
#include <cmsis_m7.h>
#ifdef PLATFORM_CACHING_ENABLED
#include <cmsis_compiler.h>
#include <m-profile/armv7m_cachel1.h>
#endif
 
#include "power_i.h"
#include "power_interrupt_handler.h"
#include "power_log.h"

// clang-format on

/*-- Symbolic Constant Macros (defines) --*/

#define MMIO_SET_MASK32(addr, mask)   MMIO_WRITE32(addr, (MMIO_READ32(addr) | (mask)))
#define MMIO_CLEAR_MASK32(addr, mask) MMIO_WRITE32(addr, (MMIO_READ32(addr) & ~(mask)))

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Functions ---------------*/
// Function to print the PLL error status for a specific core
void core_pll_error_status(uint32_t core_idx, bool is_unlock)
{
    FPFW_UNUSED(is_unlock);
    power_runconfig_t* p_runconfig = power_runconfig_get();
    const power_service_config_t* p_config = p_runconfig->p_sconfig;

    const uintptr_t cluster_pex_base_addr = (p_config->cluster_pex_base + (p_config->cluster_stride * core_idx));
    uint32_t core_pll_base_addr = cluster_pex_base_addr + PEX_CORE_PLL_ADDRESS;

    ptr_fgpll_reg core_pll;
    fgpll_pll_error_sr pll_error_sr;
    fgpll_pll_error_mask_cr pll_error_mask_cr;

    core_pll = (ptr_fgpll_reg)core_pll_base_addr;

    pll_error_sr.as_uint32 = MMIO_READ32((uint32_t*)&core_pll->pll_error_sr);
    pll_error_mask_cr.as_uint32 = MMIO_READ32((uint32_t*)&core_pll->pll_error_mask_cr);

    POWER_LOG_INFO("core_idx = %u, core_pll_addr = 0x%lx, pll_error_sr = 0x%lx\n",
                   (unsigned int)core_idx,
                   (unsigned long)core_pll_base_addr,
                   (unsigned long)&core_pll->pll_error_sr);
    POWER_LOG_INFO("wait_pll_lock_timer_exp = 0x%x\n", pll_error_sr.wait_pll_lock_timer_exp);
    POWER_LOG_INFO("wait_fll_lock_timer_exp = 0x%x\n", pll_error_sr.wait_fll_lock_timer_exp);
    POWER_LOG_INFO("wait_fll_cal_timer_exp = 0x%x\n", pll_error_sr.wait_fll_cal_timer_exp);
    POWER_LOG_INFO("wait_freq_change_timer_exp = 0x%x\n", pll_error_sr.wait_freq_change_timer_exp);
    POWER_LOG_INFO("pllrawlockerror = 0x%x\n", pll_error_sr.pllrawlockerror);
    POWER_LOG_INFO("fllrawlockerror = 0x%x\n", pll_error_sr.fllrawlockerror);
    POWER_LOG_INFO("pll_error_mask_cr = 0x%x\n", (unsigned int)pll_error_mask_cr.as_uint32);
}

void pll_isr(void)
{
    uint32_t core_idx;
    uint32_t status[3];
    uint32_t temp_status;
    ptr_scp_pwr_ctrl_reg scp_pwr_ctrl = (ptr_scp_pwr_ctrl_reg)(SCP_TOP_SCP_PWR_CTRL_ADDRESS);

    uint32_t nvic_irq_num = 0;
    int nvic_status = nvic_get_current_irq(&nvic_irq_num);

    // Determine if this is an unlock IRQ
    bool is_unlock =
        (nvic_irq_num == HW_INT_CPU_67_64_31_0_PLL_UNLOCK_INT || nvic_irq_num == HW_INT_CPU_63_32_PLL_UNLOCK_INT);

    // Select status registers based on IRQ
    volatile uint32_t* status_regs[3];

    if (nvic_status != NVIC_STATUS_SUCCESS)
    {
        POWER_LOG_ERR("Failed to get current IRQ number: %d\n", nvic_status);
        return;
    }
    if (nvic_irq_num == HW_INT_CPU_67_64_31_0_PLL_UNLOCK_INT || nvic_irq_num == HW_INT_CPU_63_32_PLL_UNLOCK_INT)
    {
        status_regs[0] = (volatile uint32_t*)&scp_pwr_ctrl->cpu_pll_unlock_status0;
        status_regs[1] = (volatile uint32_t*)&scp_pwr_ctrl->cpu_pll_unlock_status1;
        status_regs[2] = (volatile uint32_t*)&scp_pwr_ctrl->cpu_pll_unlock_status2;
    }
    else if (nvic_irq_num == HW_INT_CPU_67_64_31_0_PLL_LOCK_INT || nvic_irq_num == HW_INT_CPU_63_32_PLL_LOCK_INT)
    {
        status_regs[0] = (volatile uint32_t*)&scp_pwr_ctrl->cpu_pll_lock_status0;
        status_regs[1] = (volatile uint32_t*)&scp_pwr_ctrl->cpu_pll_lock_status1;
        status_regs[2] = (volatile uint32_t*)&scp_pwr_ctrl->cpu_pll_lock_status2;
    }
    else
    {
        POWER_LOG_ERR("pll_isr: Unexpected IRQ number: 0x%x\n", (unsigned int)nvic_irq_num);
        return;
    }

    for (int i = 0; i < 3; i++)
    {
        status[i] = MMIO_READ32((uint32_t*)status_regs[i]);
        if (status[i] != 0)
        {
            MMIO_SET_MASK32((uint32_t*)status_regs[i], status[i]);
        }
        temp_status = status[i];
        core_idx = i * 32;
        while (temp_status != 0)
        {
            if (temp_status & 1)
            {
                core_pll_error_status(core_idx, is_unlock);
            }
            temp_status >>= 1;
            core_idx++;
        }
    }

    POWER_LOG_INFO("Status0 = 0x%x, Status1 = 0x%x, Status2 = 0x%x\n",
                   (unsigned int)status[0],
                   (unsigned int)status[1],
                   (unsigned int)status[2]);
}

static void register_pll_isr(uint32_t irq_num, isr_callback_fn_sans_params_t isr)
{
    nvic_status_t nvic_status = NVIC_STATUS_SUCCESS;

    nvic_status = nvic_irq_set_isr(irq_num, isr);
    BUG_ASSERT_PARAM(nvic_status == NVIC_STATUS_SUCCESS, nvic_status, irq_num);
    nvic_status = nvic_irq_enable(irq_num);
    BUG_ASSERT_PARAM(nvic_status == NVIC_STATUS_SUCCESS, nvic_status, irq_num);
}

void enable_pll_interrupts()
{
    register_pll_isr(HW_INT_CPU_67_64_31_0_PLL_LOCK_INT, pll_isr); // Consolidated CPUs 0-31 and 64-67 PLL Error Assertion Interrupt
    register_pll_isr(HW_INT_CPU_63_32_PLL_LOCK_INT, pll_isr); // Consolidated CPUs 32-63 PLL Error Assertion Interrupt

    register_pll_isr(HW_INT_CPU_67_64_31_0_PLL_UNLOCK_INT,
                     pll_isr); // Consolidated CPUs 0-31 and 64-67 PLL Error Deaassertion Interrupt
    register_pll_isr(HW_INT_CPU_63_32_PLL_UNLOCK_INT, pll_isr); // Consolidated CPUs 32-63 PLL Error Deassertion Interrupt
}