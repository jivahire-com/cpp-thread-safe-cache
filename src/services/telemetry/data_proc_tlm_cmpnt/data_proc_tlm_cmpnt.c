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
    /* initialize dts coeff data at startup */
    fpfw_status_t status = tlm_logger_init_fuse_dts_coeff_data();
    if (FPFW_STATUS_FAILED(status))
    {
        FPFW_ET_LOG(DTSCoefficientReadFailedInit, status);
    }
}

void data_proc_tlm_cmpnt_enable_disable_transition(bool enable)
{
    FPFW_UNUSED(enable);
}