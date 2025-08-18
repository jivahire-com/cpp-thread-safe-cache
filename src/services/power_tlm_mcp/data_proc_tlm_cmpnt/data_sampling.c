//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file data_sampling.c
 * Consume sensor fifo data and log telemetry data
 */

/*------------- Includes -----------------*/
#include "compute_metrics_i.h"
#include "data_sampling_i.h" // internal APIs
#include "data_utilities_i.h"
#include "die_2_die_exchange_i.h"
#include "telemetry_events_i.h"

#include <FpFwAssert.h>
#include <assert.h> // IWYU pragma: keep for static_assert
#include <dvfs.h>
#include <exec_tlm_cmpnt.h>
#include <fpfw_status.h> // for FPFW_STATUS_SUCCEEDED, fpf...
#include <in_band_tlm_cmpnt.h>
#include <sensor_fifo_service.h> // for QUADWORD_SIZE, sensor_ram_...
#include <stdbool.h>             // for false, true
#include <stddef.h>              // for size_t
#include <stdint.h>              // for uint8_t, uint16_t
#include <string.h>              // for memset
#include <tlm_fuses.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

typedef struct
{
    pstate_throttle_status_t throttle_status;
    throttle_source_t throttle_source;
} throttling_lookup_tbl_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
// Allocate enough buffer for 8 strides from sensor fifo
static uint64_t fifo_entry[MAX_BUFFER_ENTRIES];

core_runtime_info_t core_rt[NUMBER_OF_CORES_PER_DIE] = {0};
tile_runtime_info_t tile_rt[NUMBER_OF_TILES_PER_DIE] = {0};
soc_runtime_info_t soc_rt = {0};
dimm_runtime_info_t dimm_rt = {0};
dts_tlm_coeff_t tileDtsCoefficients[NUMBER_OF_TILES_PER_DIE] = {0};

/*------------- Functions ----------------*/
void data_smpl_init(void)
{
    /* Initialize dts coeff data at startup */
    data_smpl_init_dts_coefficients();
}

void data_smpl_init_dts_coefficients(void)
{
    fpfw_status_t status =
        tlm_fuses_get_dts_coeff_tile(tileDtsCoefficients, sizeof(tileDtsCoefficients) / sizeof(tileDtsCoefficients[0]));
    if (FPFW_STATUS_FAILED(status))
    {
        FPFW_ET_LOG(DTSCoefficientReadFailedInit, status);
    }
}

void data_proc_tlm_cmpnt_clear_pwr_tlm_data(void)
{
    memset(core_rt, 0, sizeof(core_rt));
    memset(tile_rt, 0, sizeof(tile_rt));
    memset(&soc_rt, 0, sizeof(soc_rt));
    memset(&dimm_rt, 0, sizeof(dimm_rt));

    FPFW_ET_LOG(TelemetryDataCleared);
}

void data_proc_tlm_cmpnt_process_input_data(void)
{
    bool update_max_die_temp = false;
    bool update_dimm_metrics = false;

    bool is_empty[SENSOR_FIFO_MAX_ID] = {0};
    sensor_fifo_svc_is_empty(&is_empty);

    if (!is_empty[SENSOR_FIFO_TILE_TEMPERATURE_TELEMETRY_HW])
    {
        bool valid_entry = data_smpl_process_tile_temperature_sensor_fifo();
        if (valid_entry)
        {
            // If we have processed at least one tile temperature entry, we need to update the max die temp
            update_max_die_temp = true;
        }
    }

    if (!is_empty[SENSOR_FIFO_TILE_VOLTAGE_TELEMETRY_HW])
    {
        data_smpl_process_tile_voltage_sensor_fifo();
    }

    // Note : pstate packet need to be processed first before the current telemetry packet
    // to handle throttling state changes so the following current packets utilize the correct throttling state
    if (!is_empty[SENSOR_FIFO_PSTATE_TELEMETRY_HW])
    {
        data_smpl_process_pstate_sensor_fifo();
    }

    if (!is_empty[SENSOR_FIFO_CORE_CURRENT_TELEMETRY_HW])
    {
        data_smpl_process_core_current_sensor_fifo();
    }

    if (!is_empty[SENSOR_FIFO_VR_TEMP_FW])
    {
        data_smpl_process_vr_temp_sensor_fifo();
    }

    if (!is_empty[SENSOR_FIFO_VR_CURRENT_FW])
    {
        data_smpl_process_vr_current_sensor_fifo();
    }

    if (!is_empty[SENSOR_FIFO_PVT_TEMP_FW])
    {
        bool valid_entry = data_smpl_process_pvt_temperature_sensor_fifo();
        if (valid_entry)
        {
            update_max_die_temp = true;
        }
    }

    if (!is_empty[SENSOR_FIFO_DIMM_TEMP_FW])
    {
        update_dimm_metrics = data_smpl_process_dimm_sensor_fifo();
    }

    data_smpl_update_soc_avg_pstate();

    if (update_max_die_temp)
    {
        // update the max die temp
        data_smpl_update_max_die_temp();
    }

    if (update_dimm_metrics)
    {
        // inputs to the metrics are accumulated in data_smpl_parse_dimm_entry()
        comp_metrics_for_max_dimm_temp(dimm_rt.latest_max_dimm_temp_dC);
        dimm_rt.latest_max_dimm_temp_dC = 0; // reset for next dimm entry parsing

        comp_metrics_for_total_dimm_pwr(dimm_rt.latest_dimm_total_pwr_mW);
        dimm_rt.latest_dimm_total_pwr_mW = 0; // reset for next dimm entry parsing
    }
}

bool data_smpl_process_tile_temperature_sensor_fifo(void)
{
    sensor_ram_poll_status_t status;
    bool valid_entry = false;
    bool max_tile_temp_cleared = false;
    tile_temp_t* tile_temp_entry = (tile_temp_t*)fifo_entry;
    uint16_t tile_index = 0;
    do
    {
        status = sensor_fifo_svc_poll_tile_temperature(tile_temp_entry, &tile_index);
        if (status.curr_data_is_valid == true)
        {
            if (!max_tile_temp_cleared)
            {
                // Clear the max tile temperature at the start of the first tile temperature entry
                // updated in data_smpl_parse_tile_temperature_entry
                soc_rt.latest_max_tile_temp_dC = 0;
                max_tile_temp_cleared = true;
            }
            // process the tile temperature, this function updates the core_rt[] and sof_info[] values used below
            valid_entry = data_smpl_parse_tile_temperature_entry(tile_temp_entry, tile_index);

            if (valid_entry)
            {
                uint16_t core_id_1 = tile_index * 2;
                uint16_t core_id_2 = core_id_1 + 1;
                comp_metrics_for_single_core_temperature(core_id_1, core_rt[core_id_1].latest_max_value_dC);
                comp_metrics_for_single_core_temperature(core_id_2, core_rt[core_id_2].latest_max_value_dC);

                // hnf channel is mapped the same as core id
                comp_metrics_for_single_hnf_channel(core_id_1, soc_rt.latest_hnf_max_temp_dC[core_id_1]);
                comp_metrics_for_single_hnf_channel(core_id_2, soc_rt.latest_hnf_max_temp_dC[core_id_2]);
            }
        }

    } while (status.more_entries == true);

    return max_tile_temp_cleared; // means we have processed at least one tile temperature entry
}

void data_smpl_process_tile_voltage_sensor_fifo(void)
{
    sensor_ram_poll_status_t status;
    bool valid_entry = false;
    tile_voltage_t* tile_voltage_entry = (tile_voltage_t*)fifo_entry;
    uint16_t tile_index;
    do
    {
        status = sensor_fifo_svc_poll_tile_voltage(tile_voltage_entry, &tile_index);
        if (status.curr_data_is_valid == true)
        {
            // process the tile voltage
            valid_entry = data_smpl_parse_tile_voltage_entry(tile_voltage_entry, tile_index);
            if (valid_entry)
            {
                uint8_t core_id = tile_index * 2;
                // first core in the tile
                comp_metrics_for_single_core_voltage(core_id,
                                                     core_rt[core_id].latest_voltage_mV,
                                                     core_rt[core_id].latest_vcpu_voltage_mV);
                core_id++;
                // Second core in the tile
                comp_metrics_for_single_core_voltage(core_id,
                                                     core_rt[core_id].latest_voltage_mV,
                                                     core_rt[core_id].latest_vcpu_voltage_mV);
            }
        }

    } while (status.more_entries == true);
}

void data_smpl_process_pstate_sensor_fifo(void)
{
    sensor_ram_poll_status_t status;
    pstate_telem_t* pstate_entry = (pstate_telem_t*)fifo_entry;
    core_state_entry_data_t entry_data;
    do
    {
        status = sensor_fifo_svc_poll_core_pstate(pstate_entry);
        if (status.curr_data_is_valid == true)
        {
            memset(&entry_data, 0, sizeof(entry_data));

            // the pstate/cstate residency is always applied to the current pstate
            // then residency from the last pstate packet timestamp until this poll period
            // is accumulated via data_smpl_finalize_pwr_pkg_metrics(). if the pstate changes
            // here, then the accumulation to this poll period occurs on the new pstate
            uint8_t core_id = pstate_entry->data.core;
            uint8_t current_pstate = core_rt[core_id].latest_pstate;
            uint8_t current_cstate = core_rt[core_id].latest_cstate;
            uint8_t current_rack_throttle_priority = core_rt[core_id].latest_rack_throttle_priority;

            // process the core states(pstate/cstate)
            data_smpl_parse_core_states_entry(pstate_entry, &entry_data);

            if (entry_data.valid_entry_pstate)
            {
                comp_metrics_for_single_core_single_pstate(core_id,
                                                           current_pstate,
                                                           entry_data.pstate_time_diff_uS,
                                                           core_rt[core_id].latest_pstate,
                                                           entry_data.pstate_change);

                if (core_rt[core_id].status_flags.pkt_pstate_is_pending_invalid)
                {
                    // data_smpl_parse_core_states_entry() determined that pstate is transitioning from valid
                    // to invalid on that transition, need to complete the pstate residency metrics, then mark
                    // pstate invalid so it is not processed until a transition to valid pstate occurs.
                    core_rt[core_id].status_flags.pkt_pstate_is_pending_invalid = false;
                    core_rt[core_id].status_flags.pkt_pstate_is_valid = false;
                }
            }

            if (entry_data.valid_entry_cstate)
            {
                comp_metrics_for_single_core_single_cstate(core_id,
                                                           current_cstate,
                                                           entry_data.cstate_time_diff_uS,
                                                           core_rt[core_id].latest_cstate,
                                                           entry_data.cstate_change);
            }

            // TODO:Update when implementing MPAM https://azurecsi.visualstudio.com/Dev/_workitems/edit/2319779
            comp_metrics_for_mpam(core_id, core_rt[core_id].latest_mpam_id, core_rt[core_id].latest_pstate);
            // update throttling compute .
            if (entry_data.throttling_state_change)
            {
                // since multiple throttling sources can be active at the same time, no need for current
                // throttling source as it won't be terminated by the start of a new one
                comp_metrics_for_single_core_single_throttle_source(core_id,
                                                                    entry_data.throttle_source,
                                                                    entry_data.throttle_time_diff_uS,

                                                                    entry_data.throttle_start);
            }
            // update rack throttling compute.
            if (entry_data.rack_throttling_state_change)
            {
                // current_rack_throttle_priority
                comp_metrics_for_single_core_single_rack_priority(core_id,
                                                                  current_rack_throttle_priority,
                                                                  entry_data.rack_throttle_time_diff_uS,
                                                                  core_rt[core_id].latest_rack_throttle_priority,
                                                                  entry_data.rack_priority_start);
            }
            if (entry_data.overrun_count_change)
            {
                comp_metrics_for_single_core_throttle_overrun(core_id, entry_data.throttle_source);
            }
        }
    } while (status.more_entries == true);
}

void data_smpl_process_core_current_sensor_fifo(void)
{
    sensor_ram_poll_status_t status;
    bool valid_entry = false;
    core_current_t* core_current_entry = (core_current_t*)fifo_entry;
    uint16_t core_id;

    do
    {
        status = sensor_fifo_svc_poll_core_current(core_current_entry, &core_id);
        if (status.curr_data_is_valid == true)
        {
            // process the core current packet
            valid_entry = data_smpl_parse_core_current_entry(core_current_entry, core_id);
            // update any metrics from data parsed from the entry
            if (valid_entry)
            {
                comp_metrics_for_single_core_current(core_id, core_rt[core_id].latest_current_mA);
                comp_metrics_for_single_core_power(core_id, core_rt[core_id].latest_power_mW);

                if (core_rt[core_id].status_flags.pkt_pstate_is_valid)
                {
                    comp_metrics_for_single_core_power_per_pstate(core_id,
                                                                  core_rt[core_id].latest_pstate,
                                                                  core_rt[core_id].latest_power_mW);
                }

                if (core_rt[core_id].status_flags.throttle_is_active)
                {
                    // when throttling, pstate is coming from this current packet, so update the active
                    // throttle sources pstate metrics
                    for (uint8_t throttle_source = 0; throttle_source <= THROTTLE_SOURCE_ADAPTIVE_CLK; throttle_source++)
                    {
                        if (core_rt[core_id].throttle_source_tracker[throttle_source])
                        {
                            // If the core is throttling, update the metrics for the current throttle source
                            comp_metrics_for_single_core_throttling_pstate(core_id,
                                                                           throttle_source,
                                                                           core_rt[core_id].latest_pstate);
                        }
                    }
                }
            }
        }
    } while (status.more_entries == true);
}

void data_smpl_process_vr_temp_sensor_fifo(void)
{
    sensor_ram_poll_status_t status;
    vr_temp_t* vr_temp_entry = (vr_temp_t*)fifo_entry;
    do
    {
        status = sensor_fifo_svc_poll_vr_temperature(vr_temp_entry);
        if (status.curr_data_is_valid == true)
        {
            // process the VR Temperatures, entries are array bound, so always valid
            // updates soc_rt.latest_rail_temperature_dC for call below
            data_smpl_parse_vr_temperature_entry(vr_temp_entry);
            comp_metrics_for_soc_rail_temperature(&soc_rt.latest_rail_temperature_dC);
        }
    } while (status.more_entries == true);
}

void data_smpl_process_vr_current_sensor_fifo(void)
{
    sensor_ram_poll_status_t status;
    vr_current_t* vr_current_entry = (vr_current_t*)fifo_entry;
    do
    {
        status = sensor_fifo_svc_poll_vr_current(vr_current_entry);
        if (status.curr_data_is_valid == true)
        {
            // process the VR Current and Voltage, entries are array bound, so always valid
            // updates soc_rt.latest_rail_voltage_mV for call below
            data_smpl_parse_vr_current_entry(vr_current_entry);
            comp_metrics_for_soc_rails(&soc_rt.latest_rail_voltage_mV, &soc_rt.latest_rail_current_mA);
        }

    } while (status.more_entries == true);
}

bool data_smpl_process_pvt_temperature_sensor_fifo(void)
{
    bool valid_entry = false;
    sensor_ram_poll_status_t status;
    soc_pvt_temp_t* pvt_temp_entry = (soc_pvt_temp_t*)fifo_entry;
    do
    {
        status = sensor_fifo_svc_poll_soc_pvt_temperature(pvt_temp_entry);
        if (status.curr_data_is_valid == true)
        {
            valid_entry = true;

            // process the soc PVT temperature, updates soc_rt.latest_soc_top_temp_dC for call below
            data_smpl_parse_pvt_temperature_entry(pvt_temp_entry);

            comp_metrics_for_soc_top_temp_sensor(&soc_rt.latest_soc_top_temp_dC);
        }
    } while (status.more_entries == true);

    return valid_entry; // means we have processed at least one PVT temperature entry
}

bool data_smpl_process_dimm_sensor_fifo(void)
{
    bool valid_current_entry = false;
    bool valid_entry = false; // latch if any valid entry was processed
    sensor_ram_poll_status_t status;
    sensor_ram_dimm_info_t* dimm_info_entry = (sensor_ram_dimm_info_t*)fifo_entry;
    do
    {
        status = sensor_fifo_svc_poll_dimm_info(dimm_info_entry);
        if (status.curr_data_is_valid == true)
        {
            valid_current_entry = data_smpl_parse_dimm_entry(dimm_info_entry);
            if (valid_current_entry)
            {
                valid_entry = true;
                comp_metrics_for_single_soc_dimm(dimm_info_entry->dimm_id,
                                                 dimm_info_entry->dimm_temp_s0_dC,
                                                 dimm_info_entry->dimm_temp_s1_dC,
                                                 dimm_info_entry->dimm_power_mW);
            }
        }
    } while (status.more_entries == true);
    return valid_entry; // means we have processed at least one dimm entry
}

void data_smpl_finalize_pwr_pkg_throttling_metrics(uint8_t core_id, uint64_t* this_pwr_pkg_timestamp_uS)
{
    core_runtime_info_t* core_ptr = &core_rt[core_id];
    uint64_t time_diff_since_last_entry_uS = 0;

    for (uint8_t throttle_source = 0; throttle_source <= THROTTLE_SOURCE_ADAPTIVE_CLK; throttle_source++)
    {
        if (core_ptr->throttle_source_tracker[throttle_source])
        {
            data_util_calc_time_diff_and_update(&core_ptr->throttle_res_timestamp_uS[throttle_source],
                                                this_pwr_pkg_timestamp_uS,
                                                &time_diff_since_last_entry_uS);

            comp_metrics_for_single_core_single_throttle_source(core_id, throttle_source, time_diff_since_last_entry_uS, false);
        }
    }

    if (core_ptr->status_flags.rack_throttle_is_active)
    {
        // update the rack throttling metrics
        data_util_calc_time_diff_and_update(&core_ptr->rack_pri_res_timestamp_uS[core_ptr->latest_rack_throttle_priority],
                                            this_pwr_pkg_timestamp_uS,
                                            &time_diff_since_last_entry_uS);

        comp_metrics_for_single_core_single_rack_priority(core_id,
                                                          core_ptr->latest_rack_throttle_priority,
                                                          time_diff_since_last_entry_uS,
                                                          core_ptr->latest_rack_throttle_priority,
                                                          false);
    }
}

void data_smpl_finalize_pwr_pkg_metrics(void)
{
    uint64_t this_pwr_pkg_timestamp_uS = exec_tlm_cmpnt_get_timestamp_microseconds();
    uint64_t time_diff_since_last_entry_uS = 0;

    core_runtime_info_t* core_ptr = core_rt;

    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        if (core_ptr->status_flags.pkt_cstate_is_valid)
        {
            data_util_calc_time_diff_and_update(&core_ptr->cstate_res_timestamp_uS,
                                                &this_pwr_pkg_timestamp_uS,
                                                &time_diff_since_last_entry_uS);
            comp_metrics_for_single_core_single_cstate(core_id,
                                                       core_ptr->latest_cstate,
                                                       time_diff_since_last_entry_uS,
                                                       core_ptr->latest_cstate,
                                                       false);
        }

        if (core_ptr->status_flags.pkt_pstate_is_valid)
        {
            data_util_calc_time_diff_and_update(&core_ptr->pstate_res_timestamp_uS,
                                                &this_pwr_pkg_timestamp_uS,
                                                &time_diff_since_last_entry_uS);
            comp_metrics_for_single_core_single_pstate(core_id,
                                                       core_ptr->latest_pstate,
                                                       time_diff_since_last_entry_uS,
                                                       core_ptr->latest_pstate,
                                                       false);
        }

        if (core_ptr->status_flags.throttle_is_active)
        {
            data_smpl_finalize_pwr_pkg_throttling_metrics(core_id, &this_pwr_pkg_timestamp_uS);
        }

        core_ptr++;
    }
    // read and update cores droop counts data
    comp_metrics_for_cores_droop_counts();
}

void data_smpl_reset_residency_timestamps(void)
{
    uint64_t enable_pwr_pkg_timestamp_uS = exec_tlm_cmpnt_get_timestamp_microseconds();
    core_runtime_info_t* core_ptr = core_rt;

    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        if (core_ptr->status_flags.pkt_cstate_is_valid)
        {
            core_ptr->cstate_res_timestamp_uS = enable_pwr_pkg_timestamp_uS;
        }

        if (core_ptr->status_flags.pkt_pstate_is_valid)
        {
            core_ptr->pstate_res_timestamp_uS = enable_pwr_pkg_timestamp_uS;
        }

        if (core_ptr->status_flags.throttle_is_active)
        {
            for (uint8_t throttle_source = 0; throttle_source < NUMBER_OF_THROTTLE_SOURCES; throttle_source++)
            {
                if (core_ptr->throttle_source_tracker[throttle_source])
                {
                    // reset the throttle source timestamp
                    core_ptr->throttle_res_timestamp_uS[throttle_source] = enable_pwr_pkg_timestamp_uS;
                }
            }
        }

        if (core_ptr->status_flags.rack_throttle_is_active)
        {
            core_ptr->rack_pri_res_timestamp_uS[core_ptr->latest_rack_throttle_priority] = enable_pwr_pkg_timestamp_uS;
        }
        core_ptr++;
    }
}

void data_smpl_update_soc_avg_pstate(void)
{
    uint8_t latest_pstate[NUMBER_OF_CORES_PER_DIE];

    core_runtime_info_t* core_ptr = core_rt;
    uint8_t* core_pstate_ptr = latest_pstate;

    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        if (core_ptr->latest_cstate < CSTATE_C2)
        {
            // latest pstate comes from pstate packet when not throttling, from current packet when throttling
            *core_pstate_ptr = core_ptr->latest_pstate;
        }
        else
        {
            // core is in C2 or lower
            *core_pstate_ptr = INVALID_PSTATE;
        }

        core_pstate_ptr++;
        core_ptr++;
    }
    comp_metrics_for_soc_avg_pstate(&latest_pstate);
}

void data_smpl_update_max_die_temp(void)
{
    soc_rt.latest_max_die_temp_dC = MAX(soc_rt.latest_max_tile_temp_dC, soc_rt.latest_max_soc_top_temp_dC);

    comp_metrics_for_soc_max_temp(soc_rt.latest_max_die_temp_dC);

    if (in_band_tlm_cmpnt_is_inst_record_enabled(INST_TELEMETRY_ELEMENT_SOC_MAX_TEMP) &&
        (die_2_die_exch_get_this_die_id() != PRIMARY_DIE_ID))
    {
        // only write on secondary dies
        die_2_die_exch_ib_write_inst_max_die_temp(soc_rt.latest_max_die_temp_dC);
    }
}

// sensor fifo parsers

bool data_smpl_parse_tile_temperature_entry(tile_temp_t* tile_temp_entry, uint8_t tile_index)
{
    // For all details on the reference how this code was implemented, please
    // refer to the Power Management, Power Telemetry and Sensor Hardware Architecture Specifications (HAS)

    // Check first if our tile number is correct
    if (tile_index >= NUMBER_OF_TILES_PER_DIE)
    {
        FPFW_ET_LOG(LogInvalidTileId, tile_index);
        return false;
    }

    // Since this is a tile temperature, log the temperature where the tile temp belongs for the core
    uint8_t core_id = tile_index * 2;

    // First check for Temp Valid bit to indicate whether we are using the Beat 3 clock
    // off the Tile Temp Telemetry. If the temp valid indicator says 0 or not valid
    // we should not be reading the other strides, just read the first data stride that
    // contains the max temperature id and max temp of the tile. Temp valid or not
    // we must read the max id and max tile temp
    if (tile_temp_entry->temp0.temp_valid == 1)
    {
        // Check between which is bigger to log for tile core0
        uint16_t inst_temp_0_dC = TLM_FUSE_DOUT_TO_TEMP_DC(tile_temp_entry->temp1.temp0,
                                                           tileDtsCoefficients[tile_index].k_val,
                                                           tileDtsCoefficients[tile_index].y_val);
        uint16_t inst_temp_1_dC = TLM_FUSE_DOUT_TO_TEMP_DC(tile_temp_entry->temp1.temp1,
                                                           tileDtsCoefficients[tile_index].k_val,
                                                           tileDtsCoefficients[tile_index].y_val);
        uint16_t inst_temp_2_dC = TLM_FUSE_DOUT_TO_TEMP_DC(tile_temp_entry->temp1.temp2,
                                                           tileDtsCoefficients[tile_index].k_val,
                                                           tileDtsCoefficients[tile_index].y_val);

        core_rt[core_id].latest_max_value_dC = data_util_get_max_val(inst_temp_0_dC, inst_temp_1_dC, inst_temp_2_dC);

        // Check between which is bigger to log for tile core1
        inst_temp_0_dC = TLM_FUSE_DOUT_TO_TEMP_DC(tile_temp_entry->temp1.temp3,
                                                  tileDtsCoefficients[tile_index].k_val,
                                                  tileDtsCoefficients[tile_index].y_val);
        inst_temp_1_dC = TLM_FUSE_DOUT_TO_TEMP_DC(tile_temp_entry->temp2.temp4,
                                                  tileDtsCoefficients[tile_index].k_val,
                                                  tileDtsCoefficients[tile_index].y_val);
        inst_temp_2_dC = TLM_FUSE_DOUT_TO_TEMP_DC(tile_temp_entry->temp2.temp5,
                                                  tileDtsCoefficients[tile_index].k_val,
                                                  tileDtsCoefficients[tile_index].y_val);

        core_rt[core_id + 1].latest_max_value_dC = data_util_get_max_val(inst_temp_0_dC, inst_temp_1_dC, inst_temp_2_dC);

        // HNF channel is calculated the same as core_id so can re-use the same index
        soc_rt.latest_hnf_max_temp_dC[core_id] = TLM_FUSE_DOUT_TO_TEMP_DC(tile_temp_entry->temp2.temp6,
                                                                          tileDtsCoefficients[tile_index].k_val,
                                                                          tileDtsCoefficients[tile_index].y_val);

        soc_rt.latest_hnf_max_temp_dC[core_id + 1] = TLM_FUSE_DOUT_TO_TEMP_DC(tile_temp_entry->temp2.temp7,
                                                                              tileDtsCoefficients[tile_index].k_val,
                                                                              tileDtsCoefficients[tile_index].y_val);
    }

    // Also store the Max tile temperatures and its ID
    tile_rt[tile_index].latest_max_temp_dC = TLM_FUSE_DOUT_TO_TEMP_DC(tile_temp_entry->temp0.max_temp,
                                                                      tileDtsCoefficients[tile_index].k_val,
                                                                      tileDtsCoefficients[tile_index].y_val);

    if (tile_rt[tile_index].latest_max_temp_dC > soc_rt.latest_max_tile_temp_dC)
    {
        soc_rt.latest_max_tile_temp_dC = tile_rt[tile_index].latest_max_temp_dC;
    }

    tile_rt[tile_index].latest_max_temp_sensor_index = tile_temp_entry->temp0.max_id;

    return true;
}

bool data_smpl_parse_tile_voltage_entry(tile_voltage_t* tile_voltage_entry, uint8_t tile_index)
{
    // For all details on the reference how this code was implemented, please
    // refer to the Power Management, Power Telemetry and Sensor Hardware Architecture Specifications (HAS)

    // Check first if our tile number is correct
    if (tile_index >= NUMBER_OF_TILES_PER_DIE)
    {
        FPFW_ET_LOG(LogInvalidTileId, tile_index);
        return false;
    }

    // Since this is a tile voltage, log the core where the tile voltage belongs
    uint8_t core_id = tile_index * 2;
    core_rt[core_id].latest_voltage_mV = DOUT2MILLIVOLTS(tile_voltage_entry->data.vcore0);
    core_rt[core_id + 1].latest_voltage_mV = DOUT2MILLIVOLTS(tile_voltage_entry->data.vcore1);

    // Log  vcpu for droop count record
    core_rt[core_id].latest_vcpu_voltage_mV = DOUT2MILLIVOLTS(tile_voltage_entry->data.vcpu);
    core_rt[core_id + 1].latest_vcpu_voltage_mV = DOUT2MILLIVOLTS(tile_voltage_entry->data.vcpu);

    return true;
}

bool data_smpl_parse_core_current_entry(core_current_t* core_current_entry, uint8_t core_id)
{
    // Each index here refers to the core, check if correct
    if (core_id >= NUMBER_OF_CORES_PER_DIE)
    {
        FPFW_ET_LOG(LogInvalidCoreId, core_id);
        return false;
    }
    if (core_current_entry->timestamp == 0)
    {
        return false;
    }

    core_rt[core_id].latest_current_mA = (uint16_t)(core_current_entry->data.avg * CORE_CURRENT_CONVERSION_FACTOR);
    core_rt[core_id].latest_power_mW = (uint16_t)(core_current_entry->data.pwr * CORE_POWER_MW_PER_BIT);
    core_rt[core_id].latest_mpam_id = core_current_entry->data.mpam_id_low;

    if (!core_rt[core_id].status_flags.throttle_is_active)
    {
        // Note: We handle pstate during no throttling under pstate/cstate processing
        return true;
    }

    // throttling is active, which means in C0,C1 so can update latest_pstate from this current packet
    core_rt[core_id].pstate_from_current_pkt = core_current_entry->data.pstate;
    core_rt[core_id].latest_pstate = core_current_entry->data.pstate;

    return true;
}

void data_smpl_parse_vr_temperature_entry(vr_temp_t* vr_temp_entry)
{
    // Extract VR Temperature entries for all VR Rails
    for (uint8_t vr_index = 0; vr_index < MAX_NUM_OF_VR_RAILS; vr_index++)
    {
        soc_rt.latest_rail_temperature_dC[vr_index] = vr_temp_entry->vr_temp_dC[vr_index];
    }
}

void data_smpl_parse_vr_current_entry(vr_current_t* vr_current_entry)
{
    // Extract VR Current and voltage entries for all VR Rails
    for (uint8_t vr_index = 0; vr_index < MAX_NUM_OF_VR_RAILS; vr_index++)
    {
        soc_rt.latest_rail_current_mA[vr_index] = vr_current_entry->vr_current_mA[vr_index];
        soc_rt.latest_rail_voltage_mV[vr_index] = vr_current_entry->vr_voltage_mV[vr_index];
    }
}

void data_smpl_parse_pvt_temperature_entry(soc_pvt_temp_t* pvt_temp_entry)
{
    soc_rt.latest_max_soc_top_temp_dC = 0;

    for (uint8_t pvt_index = 0; pvt_index < NUMBER_OF_SOC_TEMP_SENSORS; pvt_index++)
    {
        soc_rt.latest_soc_top_temp_dC[pvt_index] = pvt_temp_entry->sensor_temp_dC[pvt_index];
        if (soc_rt.latest_soc_top_temp_dC[pvt_index] > soc_rt.latest_max_soc_top_temp_dC)
        {
            soc_rt.latest_max_soc_top_temp_dC = soc_rt.latest_soc_top_temp_dC[pvt_index];
        }
    }
}

bool data_smpl_parse_dimm_entry(sensor_ram_dimm_info_t* dimm_info_entry)
{
    if (dimm_info_entry->dimm_id < NUMBER_OF_DIMMS_PER_DIE)
    {
        dimm_rt.latest_dimm[dimm_info_entry->dimm_id].temperature_dC =
            (dimm_info_entry->dimm_temp_s0_dC > dimm_info_entry->dimm_temp_s1_dC) ? dimm_info_entry->dimm_temp_s0_dC
                                                                                  : dimm_info_entry->dimm_temp_s1_dC;

        if (dimm_rt.latest_dimm[dimm_info_entry->dimm_id].temperature_dC > dimm_rt.latest_max_dimm_temp_dC)
        {
            dimm_rt.latest_max_dimm_temp_dC = dimm_rt.latest_dimm[dimm_info_entry->dimm_id].temperature_dC;
        }

        dimm_rt.latest_dimm[dimm_info_entry->dimm_id].power_mW = dimm_info_entry->dimm_power_mW;
        dimm_rt.latest_dimm_total_pwr_mW += dimm_info_entry->dimm_power_mW;

        dimm_rt.latest_dimm[dimm_info_entry->dimm_id].memory_freq_id = dimm_info_entry->dimm_memory_frequency_id;
        dimm_rt.latest_dimm[dimm_info_entry->dimm_id].throttling_flags = dimm_info_entry->dimm_throttling;

        // TODO:https://azurecsi.visualstudio.com/Dev/_workitems/edit/2592610
        dimm_rt.latest_dimm[dimm_info_entry->dimm_id].threshold_dC = 0;
    }
    else
    {
        FPFW_ET_LOG(DIMMInfoInvalidDimmId, FPFW_STATUS_INVALID_ARGS);
        return false;
    }
    return true;
}

void data_smpl_parse_core_states_entry(pstate_telem_t* pstate_entry, core_state_entry_data_t* entry_data)
{
    // Convert Sensor Data into Pstate Entry
    uint8_t core_id = pstate_entry->data.core;
    uint8_t cstate = pstate_entry->data.cstate;
    pstate_throttle_status_t entry_throttle_status = pstate_entry->data.throttle_status;
    uint8_t rack_priority = pstate_entry->data.vm_throttle_pri;

    if (core_id >= NUMBER_OF_CORES_PER_DIE)
    {
        FPFW_ET_LOG(LogInvalidCoreId, core_id);
        return;
    }

    if (cstate >= NUMBER_OF_CSTATES)
    {
        FPFW_ET_LOG(LogInValidCstateId, cstate);
        return;
    }

    entry_data->packet_timestamp_uS = data_util_convert_systick_to_microseconds(pstate_entry->timestamp);

    // no need to range check pstate_entry->data.pstate against NUMBER_OF_PSTATES because it is a bitfield of
    // width 5 so it can't be invalid

    data_smpl_parse_cstate(pstate_entry, entry_data);

    if (core_rt[core_id].latest_cstate > CSTATE_C1)
    {
        // this handles the transition from an active core to an inactive core
        // C0 and C1 are active states, where normal pstate/throttling processing occurs
        // A core may be in C0 or C1, throttling or not throttling and then transition to C2 or deeper
        // pstate and throttling metrics are invalid in C2 or deeper, so we need to wrap up the
        // pstate residency and throttling state change if applicable
        if (core_rt[core_id].status_flags.throttle_is_active)
        {
            // corner case where core was in C0,C1 and throttling, then went to C2 or deeper
            // terminate throttling residencies
            core_rt[core_id].status_flags.throttle_is_active = false;

            // have to terminate rack throttling explicitly since the following api does not handle
            data_smpl_handle_rack_throttle_end(core_id, entry_data);

            // this is a special case api and when call comp metrics directly to terminate active throttling residencies
            data_smpl_terminate_non_rack_throttle_sources(core_id, &entry_data->packet_timestamp_uS);
        }
        else if (core_rt[core_id].status_flags.pkt_pstate_is_valid)
        {
            // not throttling, transition from active pstate to C2 or deeper, wrap up residency for active pstate
            data_smpl_parse_pstate(pstate_entry, entry_data);

            // Override these flags from data_smpl_parse_pstate() to ensure caller will update pstate
            // residency and then transition packet pstate to invalid
            entry_data->pstate_change = false; // ignore current pstate on transition to C2 or deeper
            core_rt[core_id].status_flags.pkt_pstate_is_pending_invalid = true;
        }

        return;
    }

    // latch here, to detect inactive to active or active to inactive transition
    bool previous_throttle_is_active = core_rt[core_id].status_flags.throttle_is_active;

    switch (entry_throttle_status)
    {
    case NO_THROTTLING:
        data_smpl_parse_pstate(pstate_entry, entry_data);

        if (core_rt[core_id].status_flags.throttle_is_active)
        {
            // transitioned from throttling to no throttling state
            // note: rack throttling is independent and may or may not still be active
            // this is a special case api and when call comp metrics directly to terminate active throttling residencies
            data_smpl_terminate_non_rack_throttle_sources(core_id, &entry_data->packet_timestamp_uS);
        }
        break;

    case CURRENT_THROTTLING_OVERRUN:
        // no residency for overrun, just update the count
        entry_data->throttle_source = THROTTLE_SOURCE_CURRENT_OVERRUN;
        entry_data->overrun_count_change = true;
        break;

    case ADPT_CLK_THROTTLING_OVERRUN:
        // no residency for overrun, just update the count
        entry_data->throttle_source = THROTTLE_SOURCE_ADAPTIVE_CLK_OVERRUN;
        entry_data->overrun_count_change = true;
        break;

    case RACK_THROTTLING_START:
        data_smpl_handle_rack_throttle_start(core_id, rack_priority, entry_data);
        break;

    case RACK_THROTTLING_END:
        data_smpl_handle_rack_throttle_end(core_id, entry_data);
        break;

    case TEMP_THROTTLING_START:
        data_smpl_handle_throttle_source_start(core_id, THROTTLE_SOURCE_TEMPERATURE, entry_data);
        break;

    case TEMP_THROTTLING_END:
        data_smpl_handle_throttle_source_end(core_id, THROTTLE_SOURCE_TEMPERATURE, entry_data);
        break;

    case ADPT_CLK_THROTTLING_START:
        data_smpl_handle_throttle_source_start(core_id, THROTTLE_SOURCE_ADAPTIVE_CLK, entry_data);
        break;

    case ADPT_CLK_THROTTLING_END:
        data_smpl_handle_throttle_source_end(core_id, THROTTLE_SOURCE_ADAPTIVE_CLK, entry_data);
        break;

    case CURRENT_THROTTLING_START:
        data_smpl_handle_throttle_source_start(core_id, THROTTLE_SOURCE_CURRENT, entry_data);
        break;

    case CURRENT_THROTTLING_END:
        data_smpl_handle_throttle_source_end(core_id, THROTTLE_SOURCE_CURRENT, entry_data);
        break;

    default:
        break;
    }

    if (!previous_throttle_is_active && core_rt[core_id].status_flags.throttle_is_active)
    {
        // wrap up pstate residency on transition from not throttling to throttling
        // catches any transition to any throttle source
        uint8_t last_known_valid_pstate = core_rt[core_id].latest_pstate;
        data_smpl_parse_pstate(pstate_entry, entry_data);

        // on transition to throttling, pstate isn't valid anymore.  normal data_smpl_parse_pstate() will
        // set the latest_pstate to the one in the packet. Since this is an end case, just restore the last
        // known valid pstate, which is the one before throttling started
        core_rt[core_id].latest_pstate = last_known_valid_pstate; // restore latest pstate to the last known valid pstate
        entry_data->pstate_change = false;
        core_rt[core_id].status_flags.pkt_pstate_is_pending_invalid = true;
    }

    if (previous_throttle_is_active && !core_rt[core_id].status_flags.throttle_is_active &&
        (entry_throttle_status != NO_THROTTLING))
    {
        // throttling ended with this packet, so pstate is now valid, start tracking,
        // note if status is NO_THROTTLING, then it is already handled above
        // this covers a single source of throttling ending as well as rack throttle ending with no other
        // throttle source's active. rack throttle ending is not handled by NO_THROTTLING
        data_smpl_parse_pstate(pstate_entry, entry_data);
    }
}

void data_smpl_parse_cstate(pstate_telem_t* cstate_telemetry, core_state_entry_data_t* entry_data)
{
    uint8_t core_id = cstate_telemetry->data.core; // already bounds checked against NUMBER_OF_CORES_PER_DIE
    uint8_t packet_cstate = cstate_telemetry->data.cstate; // already bounds checked against NUMBER_OF_CSTATES

    if (entry_data->packet_timestamp_uS < core_rt[core_id].cstate_res_timestamp_uS)
    {
        core_rt[core_id].cstate_res_timestamp_uS = entry_data->packet_timestamp_uS;
        // FPFW_ET_LOG(LogCstateValidTimeStamp, timestamp_uS, core_rt[core_id].cstate_res_timestamp_uS, core_id, packet_cstate); //disabled due to occurrence on SVP
        return;
    }

    entry_data->cstate_time_diff_uS = entry_data->packet_timestamp_uS - core_rt[core_id].cstate_res_timestamp_uS;

    if (core_rt[core_id].status_flags.pkt_cstate_is_valid)
    {
        entry_data->cstate_change = packet_cstate != core_rt[core_id].latest_cstate;
    }
    else
    {
        // this path gets taken the very first cstate packet after reset
        entry_data->cstate_time_diff_uS = 0; // no residency update on the transition
        entry_data->cstate_change = true;    // this will increment the entry count for the new cstate
        core_rt[core_id].status_flags.pkt_cstate_is_valid = true;
    }
    entry_data->valid_entry_cstate = true;
    core_rt[core_id].latest_cstate = packet_cstate;
    core_rt[core_id].cstate_res_timestamp_uS = entry_data->packet_timestamp_uS;
}

void data_smpl_parse_pstate(pstate_telem_t* pstate_entry, core_state_entry_data_t* entry_data)
{
    uint8_t core_id = pstate_entry->data.core; // already bounds checked against NUMBER_OF_CORES_PER_DIE
    uint8_t packet_pstate = pstate_entry->data.pstate; // already bounds checked against NUMBER_OF_PSTATES

    if (entry_data->packet_timestamp_uS < core_rt[core_id].pstate_res_timestamp_uS)
    {
        core_rt[core_id].pstate_res_timestamp_uS = entry_data->packet_timestamp_uS; // SVP workaround for timestamp issue
        // TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2721348
        // FPFW_ET_LOG(LogInValidPstateTimeStamp, timestamp_uS, core_rt[core_id].pstate_res_timestamp_uS, core_id, packet_pstate); //disabled due to occurrence on SVP
        return;
    }

    if (core_rt[core_id].status_flags.pkt_pstate_is_valid)
    {
        entry_data->pstate_time_diff_uS = entry_data->packet_timestamp_uS - core_rt[core_id].pstate_res_timestamp_uS;
        entry_data->pstate_change = packet_pstate != core_rt[core_id].latest_pstate;
    }
    else
    {
        // this path gets taken the very first pstate packet after reset, or after exiting a low power cstate
        entry_data->pstate_time_diff_uS = 0; // no residency update on the transition
        entry_data->pstate_change = true;    // this will increment the entry count for the new pstate
        core_rt[core_id].status_flags.pkt_pstate_is_valid = true;
    }

    entry_data->valid_entry_pstate = true;
    core_rt[core_id].pstate_res_timestamp_uS = entry_data->packet_timestamp_uS;
    core_rt[core_id].pstate_from_pstate_pkt = packet_pstate;
    core_rt[core_id].latest_pstate = packet_pstate;
    core_rt[core_id].latest_plimit = pstate_entry->data.plimit;
}

void data_smpl_handle_throttle_source_start(uint8_t core_id, throttle_source_t throttle_source, core_state_entry_data_t* entry_data)
{
    if ((throttle_source == THROTTLE_SOURCE_CURRENT_OVERRUN) || (throttle_source == THROTTLE_SOURCE_ADAPTIVE_CLK_OVERRUN))
    {
        // residencies are not tracked for overrun throttling sources;  not an error
        return;
    }

    if (core_rt[core_id].throttle_source_tracker[throttle_source] == false)
    {
        core_rt[core_id].throttle_source_tracker[throttle_source] = true;
        core_rt[core_id].throttle_res_timestamp_uS[throttle_source] = entry_data->packet_timestamp_uS; // latch start time

        entry_data->throttle_time_diff_uS = 0; // starting so no time diff
        entry_data->throttle_source = throttle_source;
        entry_data->throttling_state_change = true;
        entry_data->throttle_start = true;
    }
    // else unexpected to get another start without an end, but continue residency anyway

    core_rt[core_id].status_flags.throttle_is_active = true;
}

void data_smpl_handle_throttle_source_end(uint8_t core_id, throttle_source_t throttle_source, core_state_entry_data_t* entry_data)
{
    if ((throttle_source == THROTTLE_SOURCE_CURRENT_OVERRUN) || (throttle_source == THROTTLE_SOURCE_ADAPTIVE_CLK_OVERRUN))
    {
        // residencies are not tracked for overrun throttling sources;  not an error
        return;
    }

    uint64_t time_diff_uS = 0;

    if (core_rt[core_id].throttle_source_tracker[throttle_source])
    {
        core_rt[core_id].throttle_source_tracker[throttle_source] = false;

        if (entry_data->packet_timestamp_uS < core_rt[core_id].throttle_res_timestamp_uS[throttle_source])
        {
            FPFW_ET_LOG(ThrottleEndInvalidTimeStamp,
                        entry_data->packet_timestamp_uS,
                        core_rt[core_id].throttle_res_timestamp_uS[throttle_source],
                        core_id,
                        throttle_source);
        }
        else
        {
            time_diff_uS = entry_data->packet_timestamp_uS - core_rt[core_id].throttle_res_timestamp_uS[throttle_source];
        }

        entry_data->throttle_time_diff_uS = time_diff_uS;
        entry_data->throttle_source = throttle_source;
        entry_data->throttling_state_change = true;
        entry_data->throttle_start = false;
    }

    core_rt[core_id].status_flags.throttle_is_active = data_smpl_get_active_throttle_sources(core_id) == 0 ? false : true;
}

void data_smpl_handle_rack_throttle_start(uint8_t core_id, uint8_t new_priority, core_state_entry_data_t* entry_data)
{
    if (new_priority >= NUMBER_OF_RACK_THROTTLE_PRIORITIES)
    {
        FPFW_ET_LOG(InvalidRackThrottlePriority, core_id, new_priority);
        return;
    }

    data_smpl_handle_throttle_source_start(core_id, THROTTLE_SOURCE_RACK_LIMIT, entry_data);

    uint8_t current_priority = core_rt[core_id].latest_rack_throttle_priority;

    if (core_rt[core_id].status_flags.rack_throttle_is_active)
    {
        // rack throttling is in progress, but with a different priority
        if (current_priority != new_priority)
        {
            entry_data->rack_priority_start = true;
            entry_data->rack_throttling_state_change = true;
            entry_data->rack_throttle_time_diff_uS =
                entry_data->packet_timestamp_uS - core_rt[core_id].rack_pri_res_timestamp_uS[current_priority];
            core_rt[core_id].rack_pri_res_timestamp_uS[new_priority] = entry_data->packet_timestamp_uS;
        }
        else
        {
            // rack throttling is in progress with the same priority, so just continue residency
            entry_data->rack_priority_start = false;
            entry_data->rack_throttling_state_change = false;
        }
    }
    else
    {
        // no rack throttling in progress, new one starts
        core_rt[core_id].status_flags.rack_throttle_is_active = true;

        entry_data->rack_priority_start = true;
        entry_data->rack_throttling_state_change = true;
        entry_data->rack_throttle_time_diff_uS = 0; // starting so no time diff to apply to previous priority
        core_rt[core_id].rack_pri_res_timestamp_uS[new_priority] = entry_data->packet_timestamp_uS; // latch start time
    }

    core_rt[core_id].latest_rack_throttle_priority = new_priority;
}

void data_smpl_handle_rack_throttle_end(uint8_t core_id, core_state_entry_data_t* entry_data)
{
    data_smpl_handle_throttle_source_end(core_id, THROTTLE_SOURCE_RACK_LIMIT, entry_data);

    if (core_rt[core_id].status_flags.rack_throttle_is_active)
    {
        uint64_t time_diff_uS = 0;
        uint8_t active_priority = core_rt[core_id].latest_rack_throttle_priority;

        if (entry_data->packet_timestamp_uS < core_rt[core_id].rack_pri_res_timestamp_uS[active_priority])
        {
            FPFW_ET_LOG(RackThrottleEndInvalidTimeStamp,
                        entry_data->packet_timestamp_uS,
                        core_rt[core_id].rack_pri_res_timestamp_uS[active_priority],
                        core_id,
                        active_priority);
        }
        else
        {
            time_diff_uS = entry_data->packet_timestamp_uS - core_rt[core_id].rack_pri_res_timestamp_uS[active_priority];
        }

        entry_data->rack_throttle_time_diff_uS = time_diff_uS;
        entry_data->rack_throttling_state_change = true; // wrap up the residency
        entry_data->rack_priority_start = false;         // rack throttling ended,
    }

    core_rt[core_id].status_flags.rack_throttle_is_active = false;
    core_rt[core_id].latest_rack_throttle_priority = 0;
}

uint8_t data_smpl_get_active_throttle_sources(uint8_t core_id)
{
    uint8_t active_sources = 0;
    for (int throttle_source = 0; throttle_source < NUMBER_OF_THROTTLE_SOURCES; ++throttle_source)
    {
        if (core_rt[core_id].throttle_source_tracker[throttle_source])
        {
            active_sources |= (1 << throttle_source);
        }
    }
    return active_sources;
}

void data_smpl_terminate_non_rack_throttle_sources(uint8_t core_id, uint64_t* timestamp_uS)
{
    // End all active non rack throttling and update residency
    // overrun sources are not tracked for residency, so skip them
    uint64_t time_diff_uS = 0;

    for (uint8_t throttle_source = 0; throttle_source <= THROTTLE_SOURCE_ADAPTIVE_CLK; throttle_source++)
    {
        if (throttle_source == THROTTLE_SOURCE_RACK_LIMIT)
        {
            // all other throttle sources end with a NO_THROTTLE status but RACK does not end, more info in telemetry spec V0.4
            continue;
        }

        time_diff_uS = 0;

        if (core_rt[core_id].throttle_source_tracker[throttle_source])
        {
            core_rt[core_id].throttle_source_tracker[throttle_source] = false;

            if (*timestamp_uS < core_rt[core_id].throttle_res_timestamp_uS[throttle_source])
            {
                FPFW_ET_LOG(ThrottleEndInvalidTimeStamp,
                            *timestamp_uS,
                            core_rt[core_id].throttle_res_timestamp_uS[throttle_source],
                            core_id,
                            throttle_source);
            }
            else
            {
                // calculate the time difference for the residency
                time_diff_uS = *timestamp_uS - core_rt[core_id].throttle_res_timestamp_uS[throttle_source];
            }

            // special case here where metrics need to be update for all of the non rack throttling sources
            comp_metrics_for_single_core_single_throttle_source(core_id, throttle_source, time_diff_uS, false);
        }
    }
    core_rt[core_id].status_flags.throttle_is_active = data_smpl_get_active_throttle_sources(core_id) == 0 ? false : true;
}

void data_proc_tlm_cmpnt_pwr_pkg_completed(void)
{
    // Reset the computed per core metrics
    comp_metrics_reset_2_mins_metrics();
}

void data_proc_tlm_cmpnt_24hr_pkg_completed(void)
{
    comp_metrics_reset_24_hrs_metrics();
}
