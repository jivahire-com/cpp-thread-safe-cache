//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file data_proc_tlm_cmpnt.c
 * Primary data processing
 */

/*------------- Includes -----------------*/
#include "data_proc_tlm_cmpnt.h"

#include "compute_metrics_i.h"
#include "data_sampling_i.h" // internal APIs
#include "die_2_die_exchange_i.h"
#include "in_band_package_interface_i.h"

#include <FpFwUtils.h>
#include <in_band_tlm_cmpnt.h>
#include <telemetry_events_i.h>
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
extern void oob_inf_init(void);

void data_proc_tlm_cmpnt_init(uint8_t die_id)
{
    die_2_die_exch_init(die_id);

    data_smpl_init();

    comp_metrics_init();

    package_inf_init();

    oob_inf_init();
}

void data_proc_tlm_cmpnt_tlm_mode_enter_actions(tlm_operating_mode_t entering_mode)
{
    if (entering_mode == TLM_OP_MODE_PUBLISHING)
    {
        data_smpl_reset_residency_timestamps(); // reset residency timestamps for all cores
        comp_metrics_reset_local_2_min_metrics();
        comp_metrics_reset_d2d_2_min_metrics();
        comp_metrics_reset_24_hrs_metrics();

        in_band_publishing_active = true;
    }
    else
    {
        in_band_publishing_active = false;
    }
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

    in_band_tlm_cmpnt_notify_scp_tlm_svc_prepare_pwr_pkg();
}

void data_proc_tlm_cmpnt_finalize_data_for_pwr_pkg(void)
{
    data_proc_tlm_cmpnt_process_input_data(); // process any sensor fifo entries since last poll period
    data_smpl_finalize_pwr_pkg_metrics();     // update residencies
}

void data_proc_tlm_cmpnt_prepare_data_for_24hr_pkg(void)
{
}
