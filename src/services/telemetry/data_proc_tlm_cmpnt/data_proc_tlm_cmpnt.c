//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file data_proc_tlm_cmpnt.c
 * Primary data processing
 */

/*------------- Includes -----------------*/
#include "data_proc_tlm_cmpnt.h"

#include "data_sampling_i.h" // internal APIs
#include "package_interface_i.h"

#include <FpFwUtils.h>
#include <telemetry_events_i.h>
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void data_proc_tlm_cmpnt_init(void)
{
    data_smpl_init();

    package_inf_init();
}

void data_proc_tlm_cmpnt_enable_disable_transition(bool enable)
{
    FPFW_UNUSED(enable);
}