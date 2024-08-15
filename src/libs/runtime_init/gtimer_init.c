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

#define  __NO_CSR_TYPEDEFS__
#include <scp_top_regs.h>
#undef   __NO_CSR_TYPEDEFS__

/*-- Symbolic Constant Macros (defines) --*/
#define REFCLK_FREQUENCY_HZ  250000000ULL
#define REFCLK_SCALING_FACTOR  4ULL
static_assert(REFCLK_FREQUENCY_HZ * REFCLK_SCALING_FACTOR == 1000000000, "REFCLK_FREQUENCY_HZ * REFCLK_SCALING_FACTOR must equal 1GHz");

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(gtimer, FPFW_INIT_NULL_NODE)
{
    gtimer_init(SCP_TOP_GEN_CNTR_CTRL_ADDRESS, REFCLK_FREQUENCY_HZ, REFCLK_SCALING_FACTOR);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
