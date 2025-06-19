//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file out_of_band_interface.c
 * This file provides the implementation of the out-of-band telemetry interface.
 */

/*------------- Includes -----------------*/
#include "compute_metrics_i.h"
#include "data_proc_tlm_cmpnt.h"

#include <die_2_die_exchange_i.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void oob_inf_init(void)
{
    // need a call into this file, otherwise the linker will remove this file from the build
}

uint16_t data_proc_tlm_cmpnt_get_oob_crit_max_soc_temp_dC(void)
{
    sliding_window_data_t die1_sliding_window_data = {0};
    die_2_die_exch_oob_read_window_max_die_temp(1, &die1_sliding_window_data);

    uint16_t mean = data_util_mean_of_summations(computed_metrics_oob.max_soc_temp_mov_avg_dC.total_sum,
                                                 computed_metrics_oob.max_soc_temp_mov_avg_dC.sample_count,
                                                 die1_sliding_window_data.sum,
                                                 die1_sliding_window_data.num_samples);
    return (uint16_t)mean;
}