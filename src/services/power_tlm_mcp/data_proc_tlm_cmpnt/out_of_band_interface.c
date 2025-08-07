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
#include <dvfs.h>

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

    uint16_t max_dC = data_util_max_avg_of_summations(computed_metrics_oob.max_soc_temp_mov_avg_dC.total_sum,
                                                      computed_metrics_oob.max_soc_temp_mov_avg_dC.sample_count,
                                                      die1_sliding_window_data.sum,
                                                      die1_sliding_window_data.num_samples);
    return max_dC;
}

uint16_t data_proc_tlm_cmpnt_get_oob_crit_max_dimm_temp_dC(void)
{
    sliding_window_data_t die1_sliding_window_data = {0};
    die_2_die_exch_oob_read_window_max_dimm_temp(1, &die1_sliding_window_data);

    uint16_t max_dC = data_util_max_avg_of_summations(computed_metrics_oob.max_dimm_temp_mov_avg_dC.total_sum,
                                                      computed_metrics_oob.max_dimm_temp_mov_avg_dC.sample_count,
                                                      die1_sliding_window_data.sum,
                                                      die1_sliding_window_data.num_samples);
    return max_dC;
}

uint32_t data_proc_tlm_cmpnt_get_oob_soc_pwr_mW(void)
{
    // die1 has two power rails,  could be zero if running on single die
    sliding_window_data_t die1_sliding_window_data = {0};
    die_2_die_exch_oob_read_window_soc_pwr(1, &die1_sliding_window_data);

    uint32_t die1_pwr_mw = 0;
    if (die1_sliding_window_data.num_samples > 0)
    {
        die1_pwr_mw = die1_sliding_window_data.sum / die1_sliding_window_data.num_samples;
    }

    uint32_t total_pwr_mW = data_util_mov_avg_u32_get(&computed_metrics_oob.soc_total_pwr_mov_avg_mW) + die1_pwr_mw;

    return total_pwr_mW;
}

uint32_t data_proc_tlm_cmpnt_get_oob_dimm_total_pwr_mW(void)
{
    // die1 has half the dimms,  could be zero if running on single die
    sliding_window_data_t die1_sliding_window_data = {0};
    die_2_die_exch_oob_read_window_dimm_pwr(1, &die1_sliding_window_data);

    uint32_t die1_pwr_mw = 0;
    if (die1_sliding_window_data.num_samples > 0)
    {
        die1_pwr_mw = die1_sliding_window_data.sum / die1_sliding_window_data.num_samples;
    }

    uint32_t total_pwr_mW = data_util_mov_avg_u32_get(&computed_metrics_oob.dimm_total_pwr_mov_avg_mW) + die1_pwr_mw;

    return total_pwr_mW;
}

uint16_t data_proc_tlm_cmpnt_get_oob_crit_max_vr_temp_dC(void)
{
    sliding_window_data_t die1_sliding_window_data = {0};
    die_2_die_exch_oob_read_window_max_vr_temp(1, &die1_sliding_window_data);

    uint16_t max_dC = data_util_max_avg_of_summations(computed_metrics_oob.max_vr_temp_mov_avg_dC.total_sum,
                                                      computed_metrics_oob.max_vr_temp_mov_avg_dC.sample_count,
                                                      die1_sliding_window_data.sum,
                                                      die1_sliding_window_data.num_samples);
    return max_dC;
}

uint16_t data_proc_tlm_cmpnt_get_oob_soc_avg_freq_MHz(void)
{
    sliding_window_data_t die1_sliding_window_data = {0};
    die_2_die_exch_oob_read_avg_pstate(1, &die1_sliding_window_data);

    uint16_t avg_pstate = data_util_mean_of_summations(computed_metrics_oob.pstate_mov_avg.total_sum,
                                                       computed_metrics_oob.pstate_mov_avg.sample_count,
                                                       die1_sliding_window_data.sum,
                                                       die1_sliding_window_data.num_samples);

    uint16_t freq_Mhz = dvfs_get_freq_from_plimit(avg_pstate); // api range checks
    return freq_Mhz;
}

uint16_t data_proc_tlm_cmpnt_get_oob_dimm_avg_temp_dC(uint8_t dimm_idx)
{
    uint16_t average_temp_dC = 0;
    if (dimm_idx < NUMBER_OF_DIMMS_PER_DIE)
    {
        // on die 0
        average_temp_dC = data_util_mov_avg_u16_get(&computed_metrics_oob.dimm_chan_temp_mov_avg[dimm_idx]);
    }
    else if (dimm_idx < (NUMBER_OF_DIMMS_PER_DIE * 2))
    {
        // on die 1
        average_temp_dC = die_2_die_exch_oob_read_dimm_avg_temp_dC(1, dimm_idx - NUMBER_OF_DIMMS_PER_DIE);
    }

    return average_temp_dC;
}

uint16_t data_proc_tlm_cmpnt_get_oob_dimm_max_temp_dC(uint8_t dimm_idx)
{
    uint16_t max_temp_dC = 0;
    if (dimm_idx < NUMBER_OF_DIMMS_PER_DIE)
    {
        // on die 0
        max_temp_dC = computed_metrics_oob.dimm_chan_max_temp_dC[dimm_idx];
        computed_metrics_oob.dimm_chan_max_temp_dC[dimm_idx] = 0; // reset for next read
    }
    else if (dimm_idx < (NUMBER_OF_DIMMS_PER_DIE * 2))
    {
        // on die 1
        // api resets max temp for next read
        max_temp_dC = die_2_die_exch_oob_read_dimm_max_temp_dC(1, dimm_idx - NUMBER_OF_DIMMS_PER_DIE);
    }

    return max_temp_dC;
}

uint16_t data_proc_tlm_cmpnt_get_oob_dimm_avg_pwr_mW(uint8_t dimm_idx)
{
    uint16_t avg_pwr_mW = 0;
    if (dimm_idx < NUMBER_OF_DIMMS_PER_DIE)
    {
        // on die 0
        avg_pwr_mW = data_util_mov_avg_u16_get(&computed_metrics_oob.dimm_chan_pwr_mov_avg[dimm_idx]);
    }
    else if (dimm_idx < (NUMBER_OF_DIMMS_PER_DIE * 2))
    {
        // on die 1
        avg_pwr_mW = die_2_die_exch_oob_read_dimm_avg_pwr_mW(1, dimm_idx - NUMBER_OF_DIMMS_PER_DIE);
    }

    return avg_pwr_mW;
}