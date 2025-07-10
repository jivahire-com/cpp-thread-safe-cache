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
#include <power_tlm_fuse.h>      //for power fuse
#include <sensor_fifo_service.h> // for QUADWORD_SIZE, sensor_ram_...
#include <stdbool.h>             // for false, true
#include <stddef.h>              // for size_t
#include <stdint.h>              // for uint8_t, uint16_t
#include <string.h>              // for memset
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

typedef struct
{
    pstate_throttle_status_t throttle_status;
    throttle_source_t throttle_source;
} throttling_lookup_tbl_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

// ** May need to move to ARSM, ADO task
//  https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1978713
core_runtime_info_t core[NUMBER_OF_CORES_PER_DIE];
tile_runtime_info_t tile[NUMBER_OF_TILES_PER_DIE];
soc_runtime_info_t soc_info;
inst_soc_element_dimm_runtime_t latest_dimm[NUMBER_OF_DIMM_MODULES_PER_DIE];
dts_tlm_coeff_t tileDtsCoefficients[NUMBER_OF_TILES_PER_DIE] = {0};

static throttling_lookup_tbl_t throttling_tbl[] = {
    // throttle  status             throttle source                     // Status from KNG RMSSHASv0.p14 Document index value
    {CURRENT_THROTTLING_START, THROTTLE_SOURCE_CURRENT},       //   b0001 - Current Throttling Start
    {TEMP_THROTTLING_START, THROTTLE_SOURCE_TEMPERATURE},      //   b0010 - Temperature Throttling Start
    {RACK_THROTTLING_START, THROTTLE_SOURCE_RACK_LIMIT},       //   b0011 - Rack Limit Throttling Start
    {SYS_FRC_PMIN_THROTTLING_START, THROTTLE_SOURCE_VR_HOT},   //   b0100 - Sys_ForcePmin Throttling Start
    {ADPT_CLK_THROTTLING_START, THROTTLE_SOURCE_ADAPTIVE_CLK}, //   b0101 - Adaptive Clocking Throttling Start
    {CURRENT_THROTTLING_END, THROTTLE_SOURCE_CURRENT},         //   b0110 - Current Throttling End
    {TEMP_THROTTLING_END, THROTTLE_SOURCE_TEMPERATURE},        //   b0111 - Temperature Throttling End
    {RACK_THROTTLING_END, THROTTLE_SOURCE_RACK_LIMIT},         //   b1000 - Rack Limit Throttling End
    {SYS_FRC_PMIN_THROTTLING_END, THROTTLE_SOURCE_VR_HOT},     //   b1001 - Sys_ForcePmin Throttling End
    {ADPT_CLK_THROTTLING_END, THROTTLE_SOURCE_ADAPTIVE_CLK},   //   b1010 - Adaptive Clocking Throttling End
    {CURRENT_THROTTLING_OVERRUN, THROTTLE_SOURCE_CURRENT_OVERRUN}, //   b1011 - current throttling overrun-start
    {ADPT_CLK_THROTTLING_OVERRUN, THROTTLE_SOURCE_ADAPTIVE_CLK_OVERRUN}, //   b1100 - adaptive clocking overrun - start
};

/*------------- Functions ----------------*/
void data_smpl_init(void)
{
    /* Initialize dts coeff data at startup */
    data_smpl_init_dts_coefficients();
}

void data_smpl_init_dts_coefficients(void)
{
    fpfw_status_t status =
        platform_power_fuses_get_dts_coeff_tile(tileDtsCoefficients,
                                                sizeof(tileDtsCoefficients) / sizeof(tileDtsCoefficients[0]));
    if (FPFW_STATUS_FAILED(status))
    {
        FPFW_ET_LOG(DTSCoefficientReadFailedInit, status);
    }
}

void data_proc_tlm_cmpnt_clear_pwr_tlm_data(void)
{
    memset(core, 0, sizeof(core));
    memset(tile, 0, sizeof(tile));
    memset(&soc_info, 0, sizeof(soc_info));

    FPFW_ET_LOG(TelemetryDataCleared);
}

void data_proc_tlm_cmpnt_process_input_data(void)
{
    // Allocate enough buffer for 8 strides from sensor fifo. Please review this if the sensor fifo may have more entries, optimize when less
    uint64_t buffer_data[MAX_BUFFER_ENTRIES];
    sensor_ram_poll_status_t status;
    bool valid_entry = false;

    bool update_max_die_temp = false;
    bool update_max_dimm_temp = false;

    // NOTE: All sensor fifo API to check and poll data availability is guaranteed to return
    //  more_entries as false once all entries that was latched during the initial call has been
    //  consumed. This guarantees that we can exit the loops within this API

    // Check and poll for tile temperatures
    bool max_tile_temp_cleared = false;
    do
    {
        tile_temp_t* temperature_data = (tile_temp_t*)buffer_data;
        uint16_t tile_index;
        status = sensor_fifo_svc_poll_tile_temperature(temperature_data, &tile_index);
        if (status.curr_data_is_valid == true)
        {
            if (!max_tile_temp_cleared)
            {
                // Clear the max tile temperature at the start of the first tile temperature entry
                // updated in data_smpl_parse_tile_temperature_entry
                soc_info.latest_max_tile_temp_dC = 0;
                max_tile_temp_cleared = true;
            }
            // process the tile temperature, this function updates the core[] and sof_info[] values used below
            valid_entry = data_smpl_parse_tile_temperature_entry(temperature_data, tile_index);

            if (valid_entry)
            {
                update_max_die_temp = true;

                uint16_t core_id_1 = tile_index * 2;
                uint16_t core_id_2 = core_id_1 + 1;
                comp_metrics_for_single_core_temperature(core_id_1, core[core_id_1].latest_max_value_dC);
                comp_metrics_for_single_core_temperature(core_id_2, core[core_id_2].latest_max_value_dC);

                // hnf channel is mapped the same as core id
                comp_metrics_for_single_hnf_channel(core_id_1, soc_info.latest_hnf_max_temp_dC[core_id_1]);
                comp_metrics_for_single_hnf_channel(core_id_2, soc_info.latest_hnf_max_temp_dC[core_id_2]);
            }
        }

    } while (status.more_entries == true);

    // Check and poll for tile voltages
    do
    {
        tile_voltage_t* voltage_data = (tile_voltage_t*)buffer_data;
        uint16_t tile_index;
        status = sensor_fifo_svc_poll_tile_voltage(voltage_data, &tile_index);
        if (status.curr_data_is_valid == true)
        {
            // process the tile voltage
            valid_entry = data_smpl_parse_tile_voltage_entry(voltage_data, tile_index);
            if (valid_entry)
            {
                uint8_t core_id = tile_index * 2;
                // first core in the tile
                comp_metrics_for_single_core_voltage(core_id, core[core_id].latest_voltage_mV);
                core_id++;
                // Second core in the tile
                comp_metrics_for_single_core_voltage(core_id, core[core_id].latest_voltage_mV);
            }
        }

    } while (status.more_entries == true);

    // Note : pstate packet need to be processed first before the current telemetry packet
    // to handle throttling state changes so the following current packets utlize the correct throttling state
    uint8_t pstate_scf_entry_count = 0;
    do
    {
        pstate_telem_t* state_data = (pstate_telem_t*)buffer_data;
        status = sensor_fifo_svc_poll_core_pstate(state_data);
        if (status.curr_data_is_valid == true)
        {
            // used to determine if no pstate packets were processed this sampling period
            pstate_scf_entry_count++;

            // if (flags.valid_entry_pstate)
            // {
            //     comp_metrics_for_single_core_single_pstate(flags.core_id,
            //                                                core[flags.core_id].latest_pstate,
            //                                                flags.pstate_time_diff_uS,
            //                                                flags.new_pstate);
            // }

            // Capture previous PSTATE in a variable so that can be used to update residency for the previous PSTATE
            uint8_t core_id = state_data->data.core;
            uint8_t previous_pstate = core[core_id].latest_pstate;

            // process the core states(pstate/cstate)
            core_state_metrics_flags_t flags = data_smpl_parse_core_states_entry(state_data);
            // A true value of "valid_entry_pstate" indicate we have valid pstate packet and need to update residency and entry count.
            if (flags.valid_entry_pstate)
            {
                // if flags.new_pstate == true: residency goes to the previous PSTATE (time spent before switch)
                // if flags.new_pstate == false: residency goes to the current PSTATE (same PSTATE continues)
                uint8_t target_pstate = flags.new_pstate ? previous_pstate : core[core_id].latest_pstate;

                comp_metrics_for_single_core_single_pstate(flags.core_id,
                                                           target_pstate, // Use the right PSTATE based on flags.new_pstate
                                                           flags.pstate_time_diff_uS,
                                                           flags.new_pstate);
            }
            // A valid "valid_entry_cstate" indicate we have valid cstate packet and need to update residency and entry count.
            if (flags.valid_entry_cstate)
            {
                comp_metrics_for_single_core_single_cstate(flags.core_id,
                                                           core[flags.core_id].latest_cstate,
                                                           flags.cstate_time_diff_uS,
                                                           flags.new_ctstate);
            }

            // TODO:Update when implementing MPAM https://azurecsi.visualstudio.com/Dev/_workitems/edit/2319779
            comp_metrics_for_mpam(flags.core_id,
                                  core[flags.core_id].active_sample_mpam_id,
                                  core[flags.core_id].latest_pstate);
            // update throttling compute .
            if (flags.throttling_state_change)
            {
                comp_metrics_for_single_core_single_throttle_update(flags.core_id,
                                                                    core[flags.core_id].latest_throttle_type,
                                                                    flags.throttle_time_diff_uS,
                                                                    flags.throttle_start);
            }
            // update rack throttling compute.
            if (flags.rack_throttling_state_change)
            {
                comp_metrics_for_single_core_single_rack_throttle_update(flags.core_id,
                                                                         core[flags.core_id].latest_rack_throttling_priority_id,
                                                                         flags.rack_throttle_time_diff_uS,
                                                                         flags.rack_priority_start);
            }
            if (flags.overrun_count_change)
            {
                comp_metrics_for_single_core_single_throttle_overrun_count_update(flags.core_id,
                                                                                  core[flags.core_id].latest_throttle_type);
            }
        }
    } while (status.more_entries == true);

    // Check and poll for core currents
    do
    {
        core_current_t* current_data = (core_current_t*)buffer_data;
        uint16_t core_index;
        status = sensor_fifo_svc_poll_core_current(current_data, &core_index);
        if (status.curr_data_is_valid == true)
        {
            // process the core current packet
            bool valid_entry = data_smpl_parse_core_current_entry(current_data, core_index);
            // update any metrics from data parsed from the entry
            if (valid_entry)
            {
                comp_metrics_for_single_core_current(core_index, core[core_index].latest_current_mA);
                comp_metrics_for_single_core_power(core_index, core[core_index].latest_power_mW);
                comp_metrics_for_single_core_power_per_pstate(core_index,
                                                              core[core_index].latest_pstate,
                                                              core[core_index].latest_power_mW);
            }
        }
    } while (status.more_entries == true);

    /* In case we don't have a pstate temetry packet from scf ram,
     we still need to update the general residency.*/

    /*Note: To handle no-pstate telemetry packet scenario correctly, moved processing of general core states metrics compute
    after parsing both pstate and current telemetry packet. we may see below cases:
    A core may be in a no throttling state in which case lastest pstate info is coming from pstate packet.
    On following sample periods, there may be no pstate packet, so the following call udpates metrics appropriately.
    The other scenario is that a core is in a throttling state, and the current state will come from the current
    packet which will update latest pstate. Then the following call will also update metrics appropriately.*/
    if (pstate_scf_entry_count == 0)
    {
        data_smpl_update_comp_metrics_cores_states_for_no_pstate_entry();
    }

    // Check and poll for VR Temperatures
    do
    {
        vr_temp_t* vr_temperature = (vr_temp_t*)buffer_data;
        status = sensor_fifo_svc_poll_vr_temperature(vr_temperature);
        if (status.curr_data_is_valid == true)
        {
            // process the VR Temperatures, entries are array bound, so always valid
            // updates soc_info.latest_rail_temperature_dC for call below
            data_smpl_parse_vr_temperature_entry(vr_temperature);
            comp_metrics_for_soc_rail_temperature(&soc_info.latest_rail_temperature_dC);
        }
    } while (status.more_entries == true);

    // Check and poll for VR Current and Voltage
    do
    {
        vr_current_t* vr_current = (vr_current_t*)buffer_data;
        status = sensor_fifo_svc_poll_vr_current(vr_current);
        if (status.curr_data_is_valid == true)
        {
            // process the VR Current and Voltage, entries are array bound, so always valid
            // updates soc_info.latest_rail_voltage_mV for call below
            data_smpl_parse_vr_current_entry(vr_current);
            comp_metrics_for_soc_rails(&soc_info.latest_rail_voltage_mV, &soc_info.latest_rail_current_mA);
        }

    } while (status.more_entries == true);

    do
    {
        soc_pvt_temp_t* pvt_temperature = (soc_pvt_temp_t*)buffer_data;
        status = sensor_fifo_svc_poll_soc_pvt_temperature(pvt_temperature);
        if (status.curr_data_is_valid == true)
        {
            update_max_die_temp = true;

            // process the soc PVT temperature, updates soc_info.latest_soc_top_temp_dC for call below
            data_smpl_parse_pvt_temperature_entry(pvt_temperature);

            comp_metrics_for_soc_top_temp_sensor(&soc_info.latest_soc_top_temp_dC);
        }
    } while (status.more_entries == true);

    /*For soc PVT voltage, no requirement as per the Telemetry spec */

    do
    {
        sensor_ram_dimm_info_t* dimm_info = (sensor_ram_dimm_info_t*)buffer_data;
        status = sensor_fifo_svc_poll_dimm_info(dimm_info);
        if (status.curr_data_is_valid == true)
        {
            update_max_dimm_temp = true;
            valid_entry = data_smpl_parse_dimm_entry(dimm_info);
            if (valid_entry)
            {
                comp_metrics_for_single_soc_dimm_temp(dimm_info->dimm_id,
                                                      dimm_info->dimm_temp_s0_dC,
                                                      dimm_info->dimm_temp_s1_dC);
                comp_metrics_for_single_soc_dimm_power(dimm_info->dimm_id, dimm_info->dimm_power_mW);
            }
        }
    } while (status.more_entries == true);

    if (update_max_die_temp)
    {
        // update the max die temp
        data_smpl_update_max_die_temp();
    }

    if (update_max_dimm_temp)
    {
        comp_metrics_for_max_dimm_temp(soc_info.latest_max_dimm_temp_dC);
        soc_info.latest_max_dimm_temp_dC = 0; // reset for next dimm entry parsing
    }

    // run algorithms to update the aggregated telemetry data, used to generate packaged telemetry events.
    comp_metrics_for_sample_period();
}

void data_smpl_update_metrics_for_single_core_during_rack_throttling(uint8_t core_id, uint64_t time_stamp_uS)
{
    uint8_t rack_priority_id = core[core_id].latest_rack_throttling_priority_id;

    if (core[core_id].latest_rack_priority_previous_timestamp_uS[rack_priority_id] != 0 &&
        time_stamp_uS > core[core_id].latest_rack_priority_previous_timestamp_uS[rack_priority_id])
    {
        // Get the Throttling time stamp now and subtract from previous
        uint64_t time_diff_uS = time_stamp_uS - core[core_id].latest_rack_priority_previous_timestamp_uS[rack_priority_id];

        comp_metrics_for_single_core_single_rack_throttle_update(core_id, rack_priority_id, time_diff_uS, false);
    }
}

void data_smpl_update_metrics_for_single_core_during_throttling(uint8_t core_id, uint64_t time_stamp_uS)
{
    int8_t i = 0;
    for (i = 0; i < NUMBER_OF_THROTTLE_TYPES; i++)
    {
        if (core[core_id].core_throttling_tracker[i])
        {
            if (core[core_id].latest_throttle_type_previous_timestamp_uS[i] != 0 &&
                time_stamp_uS > core[core_id].latest_throttle_type_previous_timestamp_uS[i])
            {
                // Get the Throttling time stamp now and subtract from previous
                uint64_t time_diff_uS = time_stamp_uS - core[core_id].latest_throttle_type_previous_timestamp_uS[i];
                // This is the per core and per type throttling residency in uS
                // TODO:https://azurecsi.visualstudio.com/Dev/_workitems/edit/2659000
                comp_metrics_for_single_core_single_throttle_update(core_id, i, time_diff_uS, false);
                if (i == THROTTLE_SOURCE_RACK_LIMIT)
                {
                    data_smpl_update_metrics_for_single_core_during_rack_throttling(core_id, time_stamp_uS);
                }
                // Use per throttle type accumualated residency for max and avg calculation.
                /* For core pstate- min, max avg calculation during throttle*/
                comp_metrics_for_single_core_throttling_pstate(
                    core_id,
                    i,
                    time_diff_uS,
                    computed_metrics_2_mins.cores[core_id].throttle_info[i].residency_mS,
                    core[core_id].latest_pstate);
            }
        }
        core[core_id].latest_throttle_type_previous_timestamp_uS[i] = time_stamp_uS;
    }
}

void data_smpl_update_comp_metrics_cores_states_for_no_pstate_entry(void)
{
    uint64_t timestamp_uS = 0;
    // calculate the timestamp and time difference
    // Note :  soc_info.latest_core_states_proc_timestamp_uS will be updated by helper here.
    uint64_t time_diff_uS =
        data_util_calc_time_diff(&soc_info.latest_core_states_proc_timestamp_uS, &timestamp_uS, PWR_TLM_CORE_UPDATE);

    // Go over all cores
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        /* Calculate  the general residency for the core*/
        /* Do not do any update for this core if the current packet timestamp is zero: disable core */
        // TODO:https://azurecsi.visualstudio.com/Dev/_workitems/edit/2659115
        if (core[core_id].current_pkt_timestamp_uS != 0)
        {

            if (core[core_id].throttling_status == NO_THROTTLING)
            {
                /*Update the current pstate residencies*/
                comp_metrics_for_single_core_single_pstate(core_id, core[core_id].latest_pstate, time_diff_uS, false);

                // Update the core - mpam - matrics
                comp_metrics_for_mpam(core_id, core[core_id].active_sample_mpam_id, core[core_id].latest_pstate);
            }
            else
            {
                // Check to update throttling and priorities
                data_smpl_update_metrics_for_single_core_during_throttling(core_id, timestamp_uS);
            }
        }
    }
}

void data_smpl_update_max_die_temp(void)
{
    soc_info.latest_max_die_temp_dC = MAX(soc_info.latest_max_tile_temp_dC, soc_info.latest_max_soc_top_temp_dC);

    comp_metrics_for_soc_max_temp(soc_info.latest_max_die_temp_dC);

    if (in_band_tlm_cmpnt_is_inst_record_enabled(INST_TELEMETRY_ELEMENT_SOC_MAX_TEMP) &&
        (die_2_die_exch_get_this_die_id() != PRIMARY_DIE_ID))
    {
        // only write on secondary dies
        die_2_die_exch_ib_write_inst_max_die_temp(soc_info.latest_max_die_temp_dC);
    }
}

// sensor fifo parsers

bool data_smpl_parse_tile_temperature_entry(tile_temp_t* temperature_data, uint8_t tile_index)
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
    if (temperature_data->temp0.temp_valid == 1)
    {
        // Check between which is bigger to log for tile core0
        uint16_t inst_temp_0_dC = PWR_TLM_FUSE_DOUT_TO_TEMP_DC(temperature_data->temp1.temp0,
                                                               tileDtsCoefficients[tile_index].k_val,
                                                               tileDtsCoefficients[tile_index].y_val);
        uint16_t inst_temp_1_dC = PWR_TLM_FUSE_DOUT_TO_TEMP_DC(temperature_data->temp1.temp1,
                                                               tileDtsCoefficients[tile_index].k_val,
                                                               tileDtsCoefficients[tile_index].y_val);
        uint16_t inst_temp_2_dC = PWR_TLM_FUSE_DOUT_TO_TEMP_DC(temperature_data->temp1.temp2,
                                                               tileDtsCoefficients[tile_index].k_val,
                                                               tileDtsCoefficients[tile_index].y_val);

        core[core_id].latest_max_value_dC = data_util_get_max_val(inst_temp_0_dC, inst_temp_1_dC, inst_temp_2_dC);

        // Check between which is bigger to log for tile core1
        inst_temp_0_dC = PWR_TLM_FUSE_DOUT_TO_TEMP_DC(temperature_data->temp1.temp3,
                                                      tileDtsCoefficients[tile_index].k_val,
                                                      tileDtsCoefficients[tile_index].y_val);
        inst_temp_1_dC = PWR_TLM_FUSE_DOUT_TO_TEMP_DC(temperature_data->temp2.temp4,
                                                      tileDtsCoefficients[tile_index].k_val,
                                                      tileDtsCoefficients[tile_index].y_val);
        inst_temp_2_dC = PWR_TLM_FUSE_DOUT_TO_TEMP_DC(temperature_data->temp2.temp5,
                                                      tileDtsCoefficients[tile_index].k_val,
                                                      tileDtsCoefficients[tile_index].y_val);

        core[core_id + 1].latest_max_value_dC = data_util_get_max_val(inst_temp_0_dC, inst_temp_1_dC, inst_temp_2_dC);

        // HNF channel is calculated the same as core_id so can re-use the same index
        soc_info.latest_hnf_max_temp_dC[core_id] =
            PWR_TLM_FUSE_DOUT_TO_TEMP_DC(temperature_data->temp2.temp6,
                                         tileDtsCoefficients[tile_index].k_val,
                                         tileDtsCoefficients[tile_index].y_val);

        soc_info.latest_hnf_max_temp_dC[core_id + 1] =
            PWR_TLM_FUSE_DOUT_TO_TEMP_DC(temperature_data->temp2.temp7,
                                         tileDtsCoefficients[tile_index].k_val,
                                         tileDtsCoefficients[tile_index].y_val);
    }

    // Also store the Max tile temperatures and its ID
    tile[tile_index].latest_max_temp_dC = PWR_TLM_FUSE_DOUT_TO_TEMP_DC(temperature_data->temp0.max_temp,
                                                                       tileDtsCoefficients[tile_index].k_val,
                                                                       tileDtsCoefficients[tile_index].y_val);

    if (tile[tile_index].latest_max_temp_dC > soc_info.latest_max_tile_temp_dC)
    {
        soc_info.latest_max_tile_temp_dC = tile[tile_index].latest_max_temp_dC;
    }

    tile[tile_index].latest_max_temp_sensor_index = temperature_data->temp0.max_id;

    return true;
}

bool data_smpl_parse_tile_voltage_entry(tile_voltage_t* voltage_data, uint8_t tile_index)
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
    core[core_id].latest_voltage_mV = DOUT2MILLIVOLTS(voltage_data->data.vcore0);
    core[core_id + 1].latest_voltage_mV = DOUT2MILLIVOLTS(voltage_data->data.vcore1);

    // Log the tile vcpu and vsys
    tile[tile_index].vcpu.latest_value_mV = DOUT2MILLIVOLTS(voltage_data->data.vcpu);
    tile[tile_index].vsys.latest_value_mV = DOUT2MILLIVOLTS(voltage_data->data.vsys);

    return true;
}

bool data_smpl_parse_core_current_entry(core_current_t* current_data, uint8_t core_index)
{
    bool valid_entry = false;

    uint64_t timestamp_uS = data_util_convert_systick_to_microseconds(current_data->timestamp);
    // Each index here refers to the core, check if correct
    if (core_index >= NUMBER_OF_CORES_PER_DIE)
    {
        FPFW_ET_LOG(LogInvalidCoreId, core_index);
        return valid_entry;
    }

    uint8_t core_id = core_index;
    core[core_id].current_pkt_timestamp_uS = timestamp_uS;
    if (current_data->timestamp == 0)
    {
        return valid_entry;
    }

    // Get the current conversions. Conversion factors for the currents needs to be fine tuned
    // by the SVT and Silicon team.
    // We treat the average current collected by the ODCM (during its measurement window)
    // as the instantaneous current for the core.
    core[core_id].latest_current_mA = (uint16_t)(current_data->data.avg * CORE_CURRENT_CONVERSION_FACTOR);

    //
    // The current data from SCF RAM also contains the average power
    // consumed by the core during the ODCM measurement window that
    // produced this entry.
    //
    // We take this average power (per ODCM measurement window),
    // convert  it to mW, and use it to update the min, max, and
    // average power values for the core (regardless of p state) for
    // our measurement window.
    //
    // We also use this average power (per ODCM measurement window)
    // to calculate the min, max, average power consumed by the core
    // for the states of the core (p state, c-state, m-mam id change)
    // over our measurement window.
    //

    /* Core power sample value in mW,  used for averaging , samples are  multiplied
    by a factor(CORE_POWER_MW_PER_BIT) to get in mW*/
    uint16_t core_power_mW = (uint16_t)(current_data->data.pwr * CORE_POWER_MW_PER_BIT);
    core[core_index].latest_power_mW = core_power_mW;

    // Log the mpam id for this core
    core[core_id].active_sample_mpam_id = current_data->data.mpam_id_low;

    valid_entry = true;
    // Note: We handled  state changes duing no throttling under pstate/cstate processing
    if (core[core_id].throttling_status == NO_THROTTLING)
    {
        return valid_entry;
    }
    // Got the current telemetry pstate
    core[core_id].pstate_from_current_pkt = current_data->data.pstate;
    core[core_id].latest_pstate = current_data->data.pstate;

    return valid_entry;
}

core_state_metrics_flags_t data_smpl_parse_core_states_entry(pstate_telem_t* pstate_telemetry)
{
    core_state_metrics_flags_t metrics = {0};
    uint8_t throttle_index = 0;
    // Convert Sensor Data into Pstate Entry
    uint8_t core_id = pstate_telemetry->data.core;
    uint8_t pstate = pstate_telemetry->data.pstate;
    uint8_t cstate = pstate_telemetry->data.cstate;

    uint64_t timestamp_uS = data_util_convert_systick_to_microseconds(pstate_telemetry->timestamp);
    // Note: update soc_info.latest_core_states_proc_timestamp_uS to keep it latest.
    //  this timestamp used as previous timestamp for all cores, when there is no pstate packet.
    //  maintained on soc data strcuture because used for all the cores.
    soc_info.latest_core_states_proc_timestamp_uS = timestamp_uS;
    if (core_id >= NUMBER_OF_CORES_PER_DIE)
    {
        FPFW_ET_LOG(LogInvalidCoreId, core_id);
        return metrics;
    }
    if (pstate >= NUMBER_OF_PSTATES)
    {
        FPFW_ET_LOG(LogInValidPstateId, pstate);
        return metrics;
    }
    if (cstate >= NUMBER_OF_CSTATES)
    {
        FPFW_ET_LOG(LogInValidCstateId, cstate);
        return metrics;
    }
    // Check for throttling indication first. If System is throttling, do not
    // take snapshots of Pstates and Cstates
    pstate_throttle_status_t status = pstate_telemetry->data.throttle_status;

    // check if status is same.
    if (pstate_telemetry->data.throttle_status == core[core_id].throttling_status &&
        pstate_telemetry->data.throttle_status != NO_THROTTLING)
    {
        FPFW_ET_LOG(LogCoreStateSameInValidEntry, status, core[core_id].throttling_status); // unexpected
        return metrics;
    }

    if (pstate_telemetry->data.throttle_status == CURRENT_THROTTLING_OVERRUN ||
        pstate_telemetry->data.throttle_status == ADPT_CLK_THROTTLING_OVERRUN)
    {
        metrics.overrun_count_change = true; // no need for further throttle processing
        metrics.core_id = core_id;
        core[core_id].latest_throttle_type = (pstate_telemetry->data.throttle_status == CURRENT_THROTTLING_OVERRUN)
                                                 ? THROTTLE_SOURCE_CURRENT_OVERRUN
                                                 : THROTTLE_SOURCE_ADAPTIVE_CLK_OVERRUN;
        return metrics;
    }

    switch (status)
    {
    case NO_THROTTLING:
        // Log pstate
        // Note :  frist check if there is a new pstate, becuase we update the latest pstate in the parser function data_smpl_parse_pstate_no_throttling.
        metrics.new_pstate = (core[core_id].latest_pstate != pstate) ? true : false;
        data_smpl_parse_pstate_no_throttling(pstate_telemetry, timestamp_uS);

        if (core[core_id].pstate_timestamp_uS != 0)
        {
            metrics.valid_entry_pstate = true;
            metrics.pstate_time_diff_uS = (timestamp_uS > core[core_id].pstate_timestamp_uS)
                                              ? timestamp_uS - core[core_id].pstate_timestamp_uS
                                              : 0;
            if (metrics.pstate_time_diff_uS == 0)
            {
                // TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2721348
                // FPFW_ET_LOG(LogCoreInValidTimeStamp, timestamp_uS, core[core_id].pstate_timestamp_uS, status);
            }
        }

        // update timestamp only after time diff calculation done for residency.
        core[core_id].pstate_timestamp_uS = timestamp_uS;

        // Log cstate
        // TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2651433
        data_smpl_parse_cstate_no_throttling(pstate_telemetry, timestamp_uS);
        if (core[core_id].cstate_timestamp_uS != 0)
        {
            metrics.valid_entry_cstate = true;
            metrics.new_ctstate = core[core_id].flags.cstate_change ? true : false;
            metrics.cstate_time_diff_uS = (timestamp_uS > core[core_id].cstate_timestamp_uS)
                                              ? timestamp_uS - core[core_id].cstate_timestamp_uS
                                              : 0;
            if (metrics.cstate_time_diff_uS == 0)
            {
                // TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2721348
                // FPFW_ET_LOG(LogCoreInValidTimeStamp, timestamp_uS, core[core_id].cstate_timestamp_uS, status);
            }
        }
        // update timestamp for cstate only after time diff calculation done for residency.
        core[core_id].cstate_timestamp_uS = timestamp_uS;
        metrics.core_id = core_id;
        break;
    case RACK_THROTTLING_START:
    case RACK_THROTTLING_END:
        metrics.rack_throttling_state_change = data_smpl_parse_rack_throttling(pstate_telemetry);
        metrics.rack_priority_start = (core[core_id].flags.rack_priority_change) ? true : false;
        uint8_t priority_id = core[core_id].latest_rack_throttling_priority_id;
        if (core[core_id].latest_rack_priority_previous_timestamp_uS[priority_id] != 0)
        {
            metrics.rack_throttle_time_diff_uS =
                (timestamp_uS > core[core_id].latest_rack_priority_previous_timestamp_uS[priority_id])
                    ? timestamp_uS - core[core_id].latest_rack_priority_previous_timestamp_uS[priority_id]
                    : 0;
            if (metrics.rack_throttle_time_diff_uS == 0)
            {
                // TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2721348
                /*FPFW_ET_LOG(LogCoreInValidTimeStamp,
                            timestamp_uS,
                            core[core_id].latest_rack_priority_previous_timestamp_uS[priority_id],
                            status);*/
            }
        }
        // update flag once , entry count updated.
        core[core_id].flags.rack_priority_change = 0;
        // update timestamp for throttle only after time diff calculation done for residency.
        core[core_id].latest_rack_priority_previous_timestamp_uS[priority_id] = timestamp_uS;
        metrics.core_id = core_id;
        break;

    default:
        // CURRENT_THROTTLING_START
        // TEMP_THROTTLING_START
        // SYS_FRC_PMIN_THROTTLING_START
        // ADPT_CLK_THROTTLING_START
        // CURRENT_THROTTLING_END
        // TEMP_THROTTLING_END
        // SYS_FRC_PMIN_THROTTLE_END
        // ADPT_CLK_THROTTLE_END
        // Throttling information is logged based on status code (status_index)
        metrics.throttling_state_change = data_smpl_parse_throttling_state_change(pstate_telemetry);
        metrics.throttle_start = (core[core_id].flags.throttling_type_change) ? true : false;
        throttle_index = core[core_id].latest_throttle_type;
        if (core[core_id].latest_throttle_type_previous_timestamp_uS[throttle_index] != 0)
        {
            metrics.throttle_time_diff_uS =
                (timestamp_uS > core[core_id].latest_throttle_type_previous_timestamp_uS[throttle_index])
                    ? timestamp_uS - core[core_id].latest_throttle_type_previous_timestamp_uS[throttle_index]
                    : 0;
        }
        // update flag once , entry count updated.
        core[core_id].flags.throttling_type_change = 0;
        // update timestamp for throttle only after time diff calculation done for residency.
        core[core_id].latest_throttle_type_previous_timestamp_uS[throttle_index] = timestamp_uS;
        metrics.core_id = core_id;
        break;
    }
    //! update  throttling status
    core[core_id].throttling_status = pstate_telemetry->data.throttle_status;
    return metrics;
}

void data_smpl_parse_vr_temperature_entry(vr_temp_t* vr_temperature)
{
    // Extract VR Temperature entries for all VR Rails
    for (uint8_t vr_index = 0; vr_index < MAX_NUM_OF_VR_RAILS; vr_index++)
    {
        soc_info.latest_rail_temperature_dC[vr_index] = vr_temperature->vr_temp_dC[vr_index];
    }
}

void data_smpl_parse_vr_current_entry(vr_current_t* vr_current)
{
    // Extract VR Current and voltage entries for all VR Rails
    for (uint8_t vr_index = 0; vr_index < MAX_NUM_OF_VR_RAILS; vr_index++)
    {
        soc_info.latest_rail_current_mA[vr_index] = vr_current->vr_current_mA[vr_index];
        soc_info.latest_rail_voltage_mV[vr_index] = vr_current->vr_voltage_mV[vr_index];
    }
}

void data_smpl_parse_pvt_temperature_entry(soc_pvt_temp_t* pvt_temperature)
{
    soc_info.latest_max_soc_top_temp_dC = 0;

    for (uint8_t pvt_index = 0; pvt_index < NUMBER_OF_SOC_TEMP_SENSORS; pvt_index++)
    {
        soc_info.latest_soc_top_temp_dC[pvt_index] = pvt_temperature->sensor_temp_dC[pvt_index];
        if (soc_info.latest_soc_top_temp_dC[pvt_index] > soc_info.latest_max_soc_top_temp_dC)
        {
            soc_info.latest_max_soc_top_temp_dC = soc_info.latest_soc_top_temp_dC[pvt_index];
        }
    }
}

bool data_smpl_parse_dimm_entry(sensor_ram_dimm_info_t* dimm_info)
{
    if (dimm_info->dimm_id < NUMBER_OF_DIMM_MODULES_PER_DIE)
    {
        /* Note :  dimm instantaneous record entry*/
        latest_dimm[dimm_info->dimm_id].temperature_dC = (dimm_info->dimm_temp_s0_dC > dimm_info->dimm_temp_s1_dC)
                                                             ? dimm_info->dimm_temp_s0_dC
                                                             : dimm_info->dimm_temp_s1_dC;

        if (latest_dimm[dimm_info->dimm_id].temperature_dC > soc_info.latest_max_dimm_temp_dC)
        {
            soc_info.latest_max_dimm_temp_dC = latest_dimm[dimm_info->dimm_id].temperature_dC;
        }

        latest_dimm[dimm_info->dimm_id].power_mW = dimm_info->dimm_power_mW;
        latest_dimm[dimm_info->dimm_id].memory_freq_id = dimm_info->dimm_memory_frequency_id;
        latest_dimm[dimm_info->dimm_id].throttling_flags = dimm_info->dimm_throttling;

        // TODO:https://azurecsi.visualstudio.com/Dev/_workitems/edit/2592610
        latest_dimm[dimm_info->dimm_id].threshold_dC = 0;
    }
    else
    {
        FPFW_ET_LOG(DIMMInfoInvalidDimmId, FPFW_STATUS_INVALID_ARGS);
        return false;
    }
    return true;
}

// parser helper functions

void data_smpl_parse_pstate_no_throttling(pstate_telem_t* pstate_telemetry, uint64_t timestamp_uS)
{
    // Power information per P State Per Core is updated based on current
    // telemetry (which also provides power information).
    // Convert Sensor Data into Pstate Entry
    uint8_t core_id = pstate_telemetry->data.core;
    // take snapshots of Pstates and Cstates
    // check to log the Core Pstate if there are changes
    if (pstate_telemetry->data.pstate != core[core_id].pstate_from_pstate_pkt)
    {
        core[core_id].pstate_from_pstate_pkt = pstate_telemetry->data.pstate;
    }

    // if previous throttling status was throttling, check all type of active throttling
    // end and update the tracker.
    if (core[core_id].throttling_status != NO_THROTTLING)
    {
        data_smpl_parse_throttling_state_change_exit_transition(core_id, timestamp_uS);
    }
    // Save the current plimit
    core[core_id].active_sample_plimit = pstate_telemetry->data.plimit;
    // update lastest pstate based on throttle status
    core[core_id].latest_pstate = core[core_id].pstate_from_pstate_pkt;
}

void data_smpl_parse_cstate_no_throttling(pstate_telem_t* cstate_telemetry, uint64_t timestamp_uS)
{
    // Power information per P State Per Core is updated based on current
    // telemetry (which also provides power information). See the update

    // Convert Sensor Data into Pstate Entry
    uint8_t core_id = cstate_telemetry->data.core;
    // Check for throttling indication first. If System is throttling, do not
    // take snapshots of Pstates and Cstates

    /* Log cstate */
    uint8_t new_cstate = cstate_telemetry->data.cstate;

    // Always reset cstate_change flag to 0 at the start to handle all error cases and early returns
    core[core_id].flags.cstate_change = 0;

    if (new_cstate >= NUMBER_OF_CSTATES)
    {
        FPFW_ET_LOG(LogInValidCstateId, new_cstate);
        return;
    }

    if (new_cstate != core[core_id].latest_cstate) // if there is a change in cstate, we have to handle it.
    {

        /*Note : There will be no valid case for CState to be changing between C1, C2 and C3 directly.
                Everything should go through C0 first before we can get into C1 or C2 or C3, so if there
                are cases such as C1 to C3 then we discard that kind of calculations.
                Hardware logic is responsible to make sure change happen in proper order.
                TODO: only logging warining but updating calculation, need to be updated
                based on real time scenario.
                https://azurecsi.visualstudio.com/Dev/_workitems/edit/2507082
        */
        // Case 1 :Invalid cstate change- return.
        if (new_cstate > core[core_id].latest_cstate + 1)
        {
            // TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2721348
            //  FPFW_ET_LOG(CstateUnexpectedLevelChange, core_id, new_cstate, core[core_id].latest_cstate);
            return;
        }
        // case 2 : First time entering in a cstate -if timestamp is 0 so continue and update entry and
        // timestamp. At first entry on start all core timestamp are 0 , so log entry,timestamp and state.
        if (core[core_id].cstate_timestamp_uS == 0)
        {
            core[core_id].flags.cstate_change = 1;
            core[core_id].latest_cstate = new_cstate;
            return;
        }
        if (timestamp_uS < core[core_id].cstate_timestamp_uS)
        {
            // return if timestamp < core[core_id].cstate_timestamp_uS return. eg. Network time sync issue.
            FPFW_ET_LOG(LogCstateValidTimeStamp);
            return;
        }
        // Case 3: valid case and moved forward for the residency update to compute module.
        //  TODO: Implement cstate entry/exit latency calculation.
        //  https://azurecsi.visualstudio.com/Dev/_workitems/edit/2492944

        //   update the changes
        core[core_id].flags.cstate_change = 1;
        core[core_id].latest_cstate = new_cstate;
    }
}

bool data_smpl_parse_throttling_state_change(pstate_telem_t* pstate_telemetry)
{
    uint8_t core_id = pstate_telemetry->data.core;
    pstate_throttle_status_t status = pstate_telemetry->data.throttle_status;
    uint64_t timestamp_uS = data_util_convert_systick_to_microseconds(pstate_telemetry->timestamp);

    // 1. There may be multiple throttling at the same time.e.g current throttle  and Temperature throttle.
    // 2. During throttle get the pstate from current pkt, at the time of logging throttling (pstate packet), we have processed
    //    the current telemtry packet already, hence the current pkt, it the most recent current telemetry packet.
    // 3. calculate residency and entry count .
    // 4. max pstate is the reported max pstate during that throttling residency.
    int8_t throttle_index = 0;
    /* throttle_index will always be within limits of 0 ~6 or -1, based on lkp tbl*/
    throttle_index = data_smpl_parse_throttling_state_change_get_index_from_status(status);
    if (throttle_index < 0)
    {
        FPFW_ET_LOG(LogCoreThrottleInValidStatus, status);
        return false;
    }
    // Check for throttle start/end event.
    if (pstate_telemetry->data.throttle_status != core[core_id].throttling_status)
    {
        // Store the current status for throttling information
        // If there is a throttling end event, process it to calculate the following:
        //    1. Update of the cores throttling residency according to the throttling type
        //    2. Store the latest core max pstate and average pstate during throttling.
        switch (status)
        {
        case CURRENT_THROTTLING_END:
        case TEMP_THROTTLING_END:
        case SYS_FRC_PMIN_THROTTLE_END:
        case ADPT_CLK_THROTTLE_END:
            if (core[core_id].latest_throttle_type_previous_timestamp_uS[throttle_index] == 0 ||
                timestamp_uS <= core[core_id].latest_throttle_type_previous_timestamp_uS[throttle_index])

            {
                // return if timestamp < core[core_id].throttle_info[throttle_index].previous_timestamp_uS return. eg. Network time sync issue.
                // Invalid timestamp : not to update the residency with wrong data, but update the exit count and end status.
                core[core_id].throttling_status = pstate_telemetry->data.throttle_status;
                // TODO:https://azurecsi.visualstudio.com/Dev/_workitems/edit/2681622
                // Add event trace in case of a invalid timestamp.
                return false;
            }
            core[core_id].core_throttling_tracker[throttle_index] = 0;
            // Record the current pstate
            core[core_id].pstate_from_pstate_pkt = pstate_telemetry->data.pstate;
            /* Note : on the pstate packet on which throttling ends. Whether normal throttling or rack
             throttling, pstate is valid hence we update the latest core pstate.*/
            core[core_id].latest_pstate = pstate_telemetry->data.pstate;
            core[core_id].flags.throttling_type_change = 0;
            break;
        case CURRENT_THROTTLING_START:
        case TEMP_THROTTLING_START:
        case SYS_FRC_PMIN_THROTTLING_START:
        case ADPT_CLK_THROTTLING_START:
            // A new throttling started update tracker: type of throttling is throttle_index
            core[core_id].core_throttling_tracker[throttle_index] = 1;
            core[core_id].flags.throttling_type_change = 1; /* Note : this is for an entry count update. */
            break;
        default:
            break;
        }
    }
    /* Note :  update throttle type */
    core[core_id].latest_throttle_type = throttle_index;
    return true;
}

int8_t data_smpl_parse_throttling_state_change_get_index_from_status(pstate_throttle_status_t status)
{
    /*Calculate source of the throttling,
     inputs =>  1-12
     outs => 0-6  [CURR,TEMP,RACK,VR_HOT,ADPT_CLK,CURR_OVERRUN,ADPT_OVERRUN]
     */
    int8_t ret = 0;
    if (status >= 1 && status <= 12)
    {
        ret = throttling_tbl[status].throttle_source - 1;
    }
    else
    {
        // if status =0 or > 0 ,we should not be here log event trace and return with -1;
        ret = -1;
    }
    return ret;
}

uint8_t data_smpl_get_active_throttling_for_single_core(uint8_t core_id)
{
    uint8_t active_throttlings = 0;
    for (int i = 0; i < NUMBER_OF_THROTTLE_TYPES; ++i)
    {
        if (core[core_id].core_throttling_tracker[i])
        {
            active_throttlings |= (1 << i);
        }
    }
    return active_throttlings;
}

void data_smpl_parse_throttling_state_change_exit_transition(uint8_t core_id, uint64_t timestamp_uS)
{
    int i = 0;
    // End all active throttling and update residency
    for (i = 0; i < NUMBER_OF_THROTTLE_TYPES; i++)
    {
        if (core[core_id].core_throttling_tracker[i])
        {
            uint64_t time_diff_uS = timestamp_uS - core[core_id].latest_throttle_type_previous_timestamp_uS[i];
            comp_metrics_for_single_core_single_throttle_update(core_id, i, time_diff_uS, false);
            // all other thottle end with a NO_THROTTLE status but RACK does not end, more info in telemetry spec V0.4
            if (i != THROTTLE_SOURCE_RACK_LIMIT)
            {
                core[core_id].core_throttling_tracker[i] = 0; // Mark as inactive
            }
        }
    }
}

bool data_smpl_parse_rack_throttling(pstate_telem_t* pstate_telemetry)
{
    uint8_t core_id = pstate_telemetry->data.core;
    pstate_throttle_status_t status = pstate_telemetry->data.throttle_status;
    uint8_t rack_priority_id = pstate_telemetry->data.vm_throttle_pri;
    if (rack_priority_id >= NUMBER_OF_RACK_PRIORITIES)
    {
        FPFW_ET_LOG(LogCoreRackThrottleInValidPriority, rack_priority_id);
        return false;
    }
    int8_t throttle_index = 0;

    /* throttle_index will always be within limits of 0 ~6 or -1, based on lkp tbl*/
    throttle_index = data_smpl_parse_throttling_state_change_get_index_from_status(status);
    if (throttle_index < 0)
    {
        FPFW_ET_LOG(LogCoreThrottleInValidStatus, status);
        return false;
    }

    if (core[core_id].latest_rack_throttling_priority_id != pstate_telemetry->data.vm_throttle_pri)
    {
        core[core_id].flags.rack_priority_change = 1;
        core[core_id].latest_throttle_type = throttle_index;
    }
    else
    {
        core[core_id].flags.rack_priority_change = 0;
    }

    if (status == RACK_THROTTLING_END)
    {
        core[core_id].core_throttling_tracker[throttle_index] = 0;
        // Record the current pstate
        core[core_id].pstate_from_pstate_pkt = pstate_telemetry->data.pstate;
        /* Note : on the pstate packet on which throttling ends. Whether normal throttling or rack throttling,
             pstate is valid hence we update the latest core pstate.*/
        core[core_id].latest_pstate = pstate_telemetry->data.pstate;
    }
    else
    {
        /*Note :  this RACK throttling start */
        core[core_id].core_throttling_tracker[throttle_index] = 1;
        core[core_id].latest_rack_throttling_priority_id = pstate_telemetry->data.vm_throttle_pri;
    }

    return true;
}

//----------------Power telemetry update manager  ----------------

//----------------Power telemetry resources reset when packages are completed  ----------------

void data_proc_tlm_cmpnt_pwr_pkg_completed(void)
{
    // Reset the computed per core metrics
    comp_metrics_reset_2_mins_metrics();
}

void data_proc_tlm_cmpnt_24hr_pkg_completed(void)
{
    comp_metrics_reset_24_hrs_metrics();
}