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
#include "die_2_die_exchange_i.h"
#include "package_interface_i.h"

#include <FpFwUtils.h>
#include <in_band_tlm_cmpnt.h>
#include <telemetry_events_i.h>
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void data_proc_tlm_cmpnt_init(uint8_t die_id)
{
    die_2_die_exch_init(die_id);

    data_smpl_init();

    package_inf_init();
}

void data_proc_tlm_cmpnt_enable_disable_transition(bool enable)
{
    FPFW_UNUSED(enable);
}

void data_proc_tlm_cmpnt_prepare_data_for_inst_sample(void)
{
}

void data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg(void)
{
    if (die_2_die_exch_get_this_die_id() == PRIMARY_DIE_ID)
    {
        in_band_tlm_cmpnt_notify_sec_mcps_prepare_pwr_pkg();
    }
}

void data_proc_tlm_cmpnt_prepare_data_for_24hr_pkg(void)
{
}
