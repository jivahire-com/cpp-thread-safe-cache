//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pwr_tlm_core_ex_init.c
 * This file contains the initialization of the PWR TLM Core Exchange.
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <pwr_tlm_core_exchange.h>
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
// initialize from SCP only
FPFW_INIT_COMPONENT(pwr_tlm_core_ex, FPFW_INIT_NULL_NODE)
{
    pwr_tlm_core_exch_init();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}