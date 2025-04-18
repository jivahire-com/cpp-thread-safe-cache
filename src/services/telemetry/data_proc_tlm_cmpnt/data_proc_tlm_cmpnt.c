//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file data_proc_tlm_cmpnt.c
 * Primary data processing
 */

/*------------- Includes -----------------*/
#include "data_proc_tlm_cmpnt.h"

#include "tlm_logger_i.h" // internal APIs

#include <FpFwUtils.h>
#include <telemetry_events_i.h>
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void data_proc_tlm_cmpnt_init(void)
{
    /* Initialize dts coeff data at startup */
    tlm_logger_init_fuse_dts_coeff_data();

    /* Initialize the data that doesn't update */
    tlm_logger_init_constant_data();
}

void data_proc_tlm_cmpnt_enable_disable_transition(bool enable)
{
    FPFW_UNUSED(enable);
}