//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file gtimer_init.c
 * Initialization of the gtimer library
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <gtimer.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <stdint.h>

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
#define SOC_REFCLK_FREQUENCY_HZ   250000000ULL
#define SOC_REFCLK_SCALING_FACTOR 4ULL
static_assert(SOC_REFCLK_FREQUENCY_HZ * SOC_REFCLK_SCALING_FACTOR == 1000000000,
              "SOC_REFCLK_FREQUENCY_HZ * SOC_REFCLK_SCALING_FACTOR must equal 1GHz");

/*
 * Big FPGA does not support a 250 MHz refclk. The counter frequency on the
 * FPGA instead runs at 10 MHz (with a potential to scale it 16x - up to 160 MHz).
 */
#define FPGA_REFCLK_FREQUENCY 10000000ULL
#define FPGA_SCALING_FACTOR   1ULL
static_assert(FPGA_REFCLK_FREQUENCY * FPGA_SCALING_FACTOR == 10000000,
              "FPGA_REFCLK_FREQUENCY * FPGA_SCALING_FACTOR must equal 10MHz");

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(gtimer, FPFW_INIT_NULL_NODE)
{
    uint32_t frequency_hz = SOC_REFCLK_FREQUENCY_HZ;
    uint8_t scaling_factor = SOC_REFCLK_SCALING_FACTOR;

    if (IS_PLATFORM_FPGA())
    {
        frequency_hz = FPGA_REFCLK_FREQUENCY;
        scaling_factor = FPGA_SCALING_FACTOR;
    }

    gtimer_init(SCP_TOP_GEN_CNTR_CTRL_ADDRESS, frequency_hz, scaling_factor);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
