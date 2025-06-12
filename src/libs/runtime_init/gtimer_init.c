//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file gtimer_init.c
 * Initialization of the gtimer library
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <gtimer_prodfw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <interrupts.h>
#include <silibs_mcp_top_regs.h>
#include <stdint.h>
#include <stdio.h>
#include <systick_update.h> // for systick_get_emcpu_clock

#define __NO_CSR_TYPEDEFS__
#include <scp_top_regs.h>
#undef __NO_CSR_TYPEDEFS__

/*-- Symbolic Constant Macros (defines) --*/
/*
 * Set according to the SoC boot reset hardware programming guide section
 * 14.4.11 to ensure that an OS gets a 1GHz resolution timer as per SBSA
 * requirements.
 * Base frequency = 250 MHz
 */
#define SOC_GTIMER_TARGET_FREQUENCY_HZ 1000000000ULL // 1 GHz

/*
 * Big FPGA does not support a 250 MHz refclk. The counter frequency on the
 * FPGA instead runs at 10 MHz (with a potential to scale it 16x - up to 160 MHz).
 */
#define FPGA_GTIMER_TARGET_FREQUENCY_HZ 10000000ULL // 10 MHz

/*
 * SVP does not support a 250 MHz refclk. The counter frequency on the
 * SVP instead runs at 125 MHz (with a potential to scale it 8x - to make it
 * appear to tick at 1 GHz).
 */
#define SVP_GTIMER_TARGET_FREQUENCY_HZ 125000000ULL // 125 MHz

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(gtimer, FPFW_INIT_DEPENDENCIES("std_io", "hw_ver", "spi_bridge", "systick_upd"))
{
    gtimer_prodfw_init_config_t config;
    config.frequency_hz = systick_get_emcpu_clock();
    config.scaling_factor = SOC_GTIMER_TARGET_FREQUENCY_HZ / config.frequency_hz;

#if defined(SCP_RUNTIME_INIT)
    config.counter_control_base = SCP_TOP_GEN_CNTR_CTRL_ADDRESS;
    config.timer_control_base = SCP_TOP_SCP_TIMER_CTRL_ADDRESS;
    config.timer_base_address = SCP_TOP_SCP_TIMER_BASE_ADDRESS;
    config.timer_irq = HW_INT_SCP_GENERIC_TIMER_INT;
#elif defined(MCP_RUNTIME_INIT)
    config.timer_control_base = MCP_TOP_MCP_TIMER_CTRL_ADDRESS;
    config.timer_base_address = MCP_TOP_MCP_TIMER_BASE_ADDRESS;
    config.timer_irq = HW_INT_MCP_GENERIC_TIMER_INT;
#endif

    /* Override default settings in case dev. platforms have different capabilities */
    if (IS_PLATFORM_FPGA())
    {
        config.scaling_factor = FPGA_GTIMER_TARGET_FREQUENCY_HZ / config.frequency_hz;
    }
    else if (IS_PLATFORM_SVP())
    {
        config.scaling_factor = SVP_GTIMER_TARGET_FREQUENCY_HZ / config.frequency_hz;
    }

    gtimer_prodfw_init(&config);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
