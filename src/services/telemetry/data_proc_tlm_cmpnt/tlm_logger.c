//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file tlm_logger.c
 * Consume sensor fifo data and log telemetry data
 */

/*------------- Includes -----------------*/
#include "telemetry_events_i.h"
#include "tlm_logger_i.h" // internal APIs

#include <FpFwAssert.h>
#include <assert.h> // IWYU pragma: keep for static_assert
#include <exec_tlm_cmpnt.h>
#include <fpfw_status.h>         // for FPFW_STATUS_SUCCEEDED, fpf...
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
    uint8_t throttle_source;
    uint8_t transition_event;
} throttling_properties_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

// ** May need to move to ARSM, ADO task
//  https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1978713
core_runtime_info_t core[NUMBER_OF_CORES_PER_DIE];
tile_runtime_info_t tile[NUMBER_OF_TILES_PER_DIE];
soc_runtime_info_t soc_info;
dts_tlm_coeff_t tileDtsCoefficients[NUMBER_OF_TILES_PER_DIE] = {0};
/* per core power samples's averaged value in mW,  used for averaging , samples are  averaged and multiplied
by a fator(22 , CORE_POWER_MW_PER_BIT) to get in mW*/
uint32_t core_pwr_samples_accumulation_mW[NUMBER_OF_CORES_PER_DIE] = {0}; // power samples values  in mW .
/* used only for MPAM*/
uint32_t pstate_accum_uS[NUMBER_OF_CORES_PER_DIE][NUMBER_OF_PSTATES] = {0};

static throttling_properties_t throttling_prop[] = {
    // throttle             transition event              // Status from HAS Document index value
    {THROTTLE_SOURCE_NONE, THROTTLE_TRNSN_NONE},          //   b0000 - None
    {THROTTLE_SOURCE_CURRENT, THROTTLE_TRNSN_START},      //   b0001 - Current Throttling Start
    {THROTTLE_SOURCE_TEMPERATURE, THROTTLE_TRNSN_START},  //   b0010 - Temperature Throttling Start
    {THROTTLE_SOURCE_RACK_LIMIT, THROTTLE_TRNSN_START},   //   b0011 - Rack Limit Throttling Start
    {THROTTLE_SOURCE_VR_HOT, THROTTLE_TRNSN_START},       //   b0100 - Sys_ForcePmin Throttling Start
    {THROTTLE_SOURCE_ADAPTIVE_CLK, THROTTLE_TRNSN_START}, //   b0101 - Adaptive Clocking Throttling Start
    {THROTTLE_SOURCE_CURRENT, THROTTLE_TRNSN_END},        //   b0110 - Current Throttling End
    {THROTTLE_SOURCE_TEMPERATURE, THROTTLE_TRNSN_END},    //   b0111 - Temperature Throttling End
    {THROTTLE_SOURCE_RACK_LIMIT, THROTTLE_TRNSN_END},     //   b1000 - Rack Limit Throttling End
    {THROTTLE_SOURCE_VR_HOT, THROTTLE_TRNSN_END},         //   b1001 - Sys_ForcePmin Throttling End
    {THROTTLE_SOURCE_ADAPTIVE_CLK, THROTTLE_TRNSN_END},   //   b1010 - Adaptive Clocking Throttling End
};

/*------------- Functions ----------------*/
void data_proc_tlm_cmpnt_clear_pwr_tlm_data(void)
{
    memset(core, 0, sizeof(core));
    memset(tile, 0, sizeof(tile));
    memset(&soc_info, 0, sizeof(soc_info));

    FPFW_ET_LOG(TelemetryDataCleared);
}

fpfw_status_t tlm_logger_init_fuse_dts_coeff_data(void)
{
    fpfw_status_t status =
        platform_power_fuses_get_dts_coeff_tile(tileDtsCoefficients,
                                                sizeof(tileDtsCoefficients) / sizeof(tileDtsCoefficients[0]));
    if (FPFW_STATUS_FAILED(status))
    {
        return status;
    }
    return FPFW_STATUS_SUCCESS;
}

// Based on the HAS, there are 3 temperature sensors for each Core in each tile
int16_t find_hot_core_temp(int16_t temp0, int16_t temp1, int16_t temp2)
{
    int16_t inst_temp = (temp0 > temp1) ? temp0 : temp1;
    inst_temp = (inst_temp > temp2) ? inst_temp : temp2;
    return inst_temp;
}

fpfw_status_t tlm_logger_log_tile_temperature(tile_temp_t* temperature_data, uint8_t tile_index)
{
    // For all details on the reference how this code was implemented, please
    // refer to the Power Management, Power Telemetry and Sensor Hardware Architecture Specifications (HAS)

    // Check first if our tile number is correct
    if (tile_index >= NUMBER_OF_TILES_PER_DIE)
    {
        return FPFW_STATUS_INVALID_ARGS;
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
        int16_t inst_temp_0_dC = PWR_TLM_DOUT2TEMP_FUSED_DC(temperature_data->temp1.temp0,
                                                            tileDtsCoefficients[tile_index].k_val,
                                                            tileDtsCoefficients[tile_index].y_val);
        int16_t inst_temp_1_dC = PWR_TLM_DOUT2TEMP_FUSED_DC(temperature_data->temp1.temp1,
                                                            tileDtsCoefficients[tile_index].k_val,
                                                            tileDtsCoefficients[tile_index].y_val);
        int16_t inst_temp_2_dC = PWR_TLM_DOUT2TEMP_FUSED_DC(temperature_data->temp1.temp2,
                                                            tileDtsCoefficients[tile_index].k_val,
                                                            tileDtsCoefficients[tile_index].y_val);

        core[core_id].temperature.latest_value_dC = find_hot_core_temp(inst_temp_0_dC, inst_temp_1_dC, inst_temp_2_dC);

        // Check between which is bigger to log for tile core1
        inst_temp_0_dC = PWR_TLM_DOUT2TEMP_FUSED_DC(temperature_data->temp1.temp3,
                                                    tileDtsCoefficients[tile_index].k_val,
                                                    tileDtsCoefficients[tile_index].y_val);
        inst_temp_1_dC = PWR_TLM_DOUT2TEMP_FUSED_DC(temperature_data->temp2.temp4,
                                                    tileDtsCoefficients[tile_index].k_val,
                                                    tileDtsCoefficients[tile_index].y_val);
        inst_temp_2_dC = PWR_TLM_DOUT2TEMP_FUSED_DC(temperature_data->temp2.temp5,
                                                    tileDtsCoefficients[tile_index].k_val,
                                                    tileDtsCoefficients[tile_index].y_val);
        core[core_id + 1].temperature.latest_value_dC = find_hot_core_temp(inst_temp_0_dC, inst_temp_1_dC, inst_temp_2_dC);

        // Log all the HNF Temperature, review raw HNF data for units.
        // TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2289685
        soc_info.hnf[core_id].latest_value_dC = temperature_data->temp2.temp6;
        soc_info.hnf[core_id + 1].latest_value_dC = temperature_data->temp2.temp7;
    }

    // Also store the Max tile temperatures and its ID
    tile[tile_index].active_sample_max_temperature_dC = temperature_data->temp0.max_temp;
    tile[tile_index].active_sample_max_id = temperature_data->temp0.max_id;

    return FPFW_STATUS_SUCCESS;
}

fpfw_status_t tlm_logger_log_tile_voltage(tile_voltage_t* voltage_data, uint8_t tile_index)
{
    // For all details on the reference how this code was implemented, please
    // refer to the Power Management, Power Telemetry and Sensor Hardware Architecture Specifications (HAS)

    // Check first if our tile number is correct
    if (tile_index >= NUMBER_OF_TILES_PER_DIE)
    {
        return FPFW_STATUS_INVALID_ARGS;
    }

    // Since this is a tile voltage, log the core where the tile voltage belongs
    uint8_t core_id = tile_index * 2;
    core[core_id].voltage.latest_value_mV = DOUT2MILLIVOLTS(voltage_data->data.vcore0);
    core[core_id + 1].voltage.latest_value_mV = DOUT2MILLIVOLTS(voltage_data->data.vcore1);

    // Log the tile vcpu and vsys
    tile[tile_index].vcpu.latest_value_mV = DOUT2MILLIVOLTS(voltage_data->data.vcpu);
    tile[tile_index].vsys.latest_value_mV = DOUT2MILLIVOLTS(voltage_data->data.vsys);
    return FPFW_STATUS_SUCCESS;
}

fpfw_status_t tlm_logger_log_core_current(core_current_t* current_data, uint8_t core_index)
{
    // Each index here refers to the core, check if correct
    if (core_index >= NUMBER_OF_CORES_PER_DIE)
    {
        return FPFW_STATUS_INVALID_ARGS;
    }

    uint8_t core_id = core_index;
    core[core_id].current_pkt_timestamp = current_data->timestamp;
    if (current_data->timestamp == 0)
    {
        return FPFW_STATUS_SUCCESS;
    }

    // Get the current conversions. Conversion factors for the currents needs to be fine tuned
    //   by the SVT and Silicon team.
    current_t current;
    current.min_mA = (uint16_t)(current_data->data.min * CORE_CURRENT_CONVERSION_FACTOR);
    current.average_mA = (uint16_t)(current_data->data.avg * CORE_CURRENT_CONVERSION_FACTOR);
    current.max_mA = (uint16_t)(current_data->data.max * CORE_CURRENT_CONVERSION_FACTOR);

    // check for Minimum currents, where the initial minimum is 0
    if (core[core_id].current.min_mA > current.min_mA || core[core_id].current.min_mA == 0)
    {
        core[core_id].current.min_mA = current.min_mA;
    }

    // check for Maximum currents
    if (core[core_id].current.max_mA < current.max_mA)
    {
        core[core_id].current.max_mA = current.max_mA;
    }

    // Check for the change bit
    if (current_data->data.change == 1)
    {
        core[core_id].flags.id_change_bit = 1;
        core[core_id].num_pwr_samples = 0;
        core_pwr_samples_accumulation_mW[core_id] = 0;
        core[core_id].pstate_timestamp_uS = current_data->timestamp;
    }
    else
    {
        // Stuff the power readings here
        if (core[core_id].num_pwr_samples < MAX_NUMBER_POWER_SAMPLE)
        {
            // We will keep up to a number of samples defined
            core_pwr_samples_accumulation_mW[core_id] += current_data->data.pwr * CORE_POWER_MW_PER_BIT;
            core[core_id].num_pwr_samples++;
        }

        // NOTE: Based on pioneer throttling scenarios there are corner cases on the
        //   SCF RAM Current Telemetry packet that doesnt indicate the change bit set but
        //   a Pstate change happens at the end of the Current telemetry sampling. For this
        //   situation, we consider the power measurement to be valid and should belong to
        //   the previous pstate before the change. On situations where the core is throttling,
        //   pstate change packets may not appear or be delayed, while current telemetry may
        //   indicate a change on pstate first. - We may revisit this after the real silicon for
        //   KNG comes in https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1753817
        //
        if (core[core_id].pstate_from_current_pkt != current_data->data.pstate)
        {
            core[core_id].flags.pstate_change = 1;
            core[core_id].pstate_timestamp_uS = current_data->timestamp;
        }
    }

    // The average current reported from SCF is the average of the span of time
    //   of measurement window therefore, we will treat this as the
    //   instantaneous current @ "x" mS, depend  on the sampling rate, which will be in the range of 3 ~ 5mS.
    // TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2330778
    core[core_id].current.latest_value_mA = current.average_mA;
    // Log average
    core[core_id].current.average_mA = current.average_mA;

    // Log the Ldo Voltage at this instant
    core[core_id].ldo_voltage = current_data->data.volt;

    // Got the current telemetry pstate
    core[core_id].pstate_from_current_pkt = current_data->data.pstate;

    // Log the average power of the core in this instant
    core[core_id].active_sample_mpam_id = current_data->data.mpam_id_low;

    return FPFW_STATUS_SUCCESS;
}

fpfw_status_t update_core_pstate_timestamps(uint8_t core_id, uint8_t pstate, uint64_t timestamp_uS)
{
    // Update the residency of the previous pstate
    if (core[core_id].pstate_timestamp_uS != 0 && core[core_id].pstate_timestamp_uS < timestamp_uS)
    {
        //  obtain the time stamp difference @ uS
        uint64_t timestamp_diff_uS = timestamp_uS - core[core_id].pstate_timestamp_uS;
        core[core_id].pstate[pstate].residency_uS += timestamp_diff_uS;
        pstate_accum_uS[core_id][pstate] += timestamp_diff_uS; // for MPAM only .
    }

    core[core_id].pstate_timestamp_uS = timestamp_uS;
    return FPFW_STATUS_SUCCESS;
}

fpfw_status_t tlm_logger_log_core_pstate(pstate_telem_t* pstate_telemetry)
{
    // Power information per P State Per Core is updated based on current
    // telemetry (which also provides power information). See the update
    // in tlm_logger_chk_upd_pstate().

    // Convert Sensor Data into Pstate Entry
    uint8_t core_id = pstate_telemetry->data.core;
    if (core_id >= NUMBER_OF_CORES_PER_DIE)
    {
        return FPFW_STATUS_INVALID_ARGS;
    }

    // Check for throttling indication first. If System is throttling, do not
    // take snapshots of Pstates and Cstates
    if (pstate_telemetry->data.throttle_status == NO_THROTTLE)
    {
        // check to log the Core Pstate if there are changes
        if (pstate_telemetry->data.pstate != core[core_id].pstate_from_pstate_pkt)
        {
            core[core_id].pstate_from_pstate_pkt = pstate_telemetry->data.pstate;
            core[core_id].pstate[pstate_telemetry->data.pstate].entry_count++;
            core[core_id].pstate_timestamp_uS = pstate_telemetry->timestamp;
        }

        // Process other task only if we are not throttling
        if (core[core_id].flags.throttling_start_occurred == 0)
        {

            // **Place holder for processing cstate latency here
            // https://dev.azure.com/AzureCSI/Dev/_queries/edit/1755548/?triage=true
            // chk_and_update_cstate(core_id, pstate_telemetry->data.cstate, pstate_telemetry->timestamp);

            //! reset the throttling type to no throttling
            core[core_id].throttle_source = THROTTLE_SOURCE_NONE;
            core[core_id].throttle_trnsn_event = THROTTLE_TRNSN_NONE;
            core[core_id].nominal_pstate = pstate_telemetry->data.pstate;
        }

        // Save the current plimit
        core[core_id].active_sample_plimit = pstate_telemetry->data.plimit;
    }
    else
    {
        PSTATE_THROTTLE_STATUS_CODES prop_index = pstate_telemetry->data.throttle_status;
        core[core_id].throttle_source = throttling_prop[prop_index].throttle_source;
        core[core_id].throttle_trnsn_event = throttling_prop[prop_index].transition_event;

        // Check for throttle events start
        if (pstate_telemetry->data.throttle_status != core[core_id].throttling_status)
        {
            // Store the current status for throttling information

            core[core_id].flags.throttling_ev_change = 1;

            // If there is a throttling end event, process it to calculate the following:
            //    1. Update of the cores throttling residency according to the throttling type
            //    2. Update the runtime throttling counter which is used for the MPAM throttling calculation and update
            //    3. Store the latest core max pstate and average pstate during throttling.
            if (prop_index >= CURRENT_THROTTLE_END)
            {
                if (core[core_id].throttle_timestamp_uS != 0 && pstate_telemetry->timestamp > core[core_id].throttle_timestamp_uS)
                {
                    // Get the Throttling time stamp now and subtract from previous
                    uint64_t timestamp_now = (pstate_telemetry->timestamp - core[core_id].throttle_timestamp_uS) / 1000;

                    // This is the per core and per type throttling residency in uS
                    core[core_id].throttle_info[prop_index - CURRENT_THROTTLE_END].residency_mS += timestamp_now;

                    // This throttling counter will be used by the MPAM for VM Throttling
                    core[core_id].throttling_counter += timestamp_now;
                }
                core[core_id].throttle_info[prop_index - CURRENT_THROTTLE_END].exit_count++;

                // Record the current pstate
                core[core_id].pstate_from_pstate_pkt = pstate_telemetry->data.pstate;

                // Indicate that the throttling have at least ended once
                core[core_id].flags.throttling_end_occurred = 1;
                core[core_id].flags.throttling_start_occurred = 0;
            }
            else
            {
                core[core_id].throttle_info[prop_index - 1].entry_count++;

                // Indicate that the throttling start
                core[core_id].flags.throttling_start_occurred = 1;
                core[core_id].throttle_timestamp_uS = pstate_telemetry->timestamp;
            }
        }

        if (core[core_id].throttle_source == THROTTLE_SOURCE_RACK_LIMIT)
        {
            // Check if there was a priority id change
            if (core[core_id].throttling_priority_id != pstate_telemetry->data.vm_throttle_pri)
            {
                core[core_id].flags.rack_priority_change = 1;
                core[core_id].throttling_priority_id = pstate_telemetry->data.vm_throttle_pri;
            }
        }
    }

    //! set the throttling status
    core[core_id].throttling_status = pstate_telemetry->data.throttle_status;
    return FPFW_STATUS_SUCCESS;
}

void tlm_logger_log_vr_temp(vr_temp_t* vr_temperature)
{
    // Extract VR Temperature entries for all VR Rails
    for (uint8_t vr_index = 0; vr_index < MAX_NUM_OF_VR_RAILS; vr_index++)
    {
        soc_info.rail[vr_index].temperature.latest_value_dC = vr_temperature->vr_temp_dC[vr_index];
    }
}

void tlm_logger_log_vr_current(vr_current_t* vr_current)
{

    // Extract VR Current and voltage entries for all VR Rails
    for (uint8_t vr_index = 0; vr_index < MAX_NUM_OF_VR_RAILS; vr_index++)
    {
        soc_info.rail[vr_index].current.latest_value_mA = vr_current->vr_current_mA[vr_index];
        soc_info.rail[vr_index].voltage.latest_value_mV = vr_current->vr_voltage_mV[vr_index];
    }
}
// TODO : soc_pvt_temp_t only have :sensor_temp_dC entry need to check for min, max, avg etc as per schema.
// https://azurecsi.visualstudio.com/Dev/_workitems/edit/2305957
void tlm_logger_log_soc_pvt_temp(soc_pvt_temp_t* pvt_temperature)
{
    for (uint8_t pvt_index = 0; pvt_index < NUMBER_OF_SOC_TEMP_SENSORS; pvt_index++)
    {
        soc_info.sensor_temp[pvt_index].latest_value_dC = pvt_temperature->sensor_temp_dC[pvt_index];
    }
}

fpfw_status_t telmain_log_dimm_info(sensor_ram_dimm_info_t* dimm_info)
{
    // DIMM(Dual inline memory module)- Convert Sensor Data into DIMM Entries
    // Check for which DIMM channel is this entry, do not process anything beyond the valid DIMM index
    if (dimm_info->dimm_id < NUMBER_OF_DIMM_MODULES)
    {
        soc_info.dimm[dimm_info->dimm_id].s0.latest_value_dC = dimm_info->dimm_temp_s0_dC;
        soc_info.dimm[dimm_info->dimm_id].s1.latest_value_dC = dimm_info->dimm_temp_s1_dC;
        soc_info.dimm[dimm_info->dimm_id].dimm_throttling = dimm_info->dimm_throttling;
        soc_info.dimm[dimm_info->dimm_id].dimm_memory_frequency_id = dimm_info->dimm_memory_frequency_id;
        soc_info.dimm[dimm_info->dimm_id].dimm_power_mW = dimm_info->dimm_power_mW;
    }
    else
    {
        FPFW_ET_LOG(DIMMInfoInvalidDimmId, FPFW_STATUS_INVALID_ARGS);
        return FPFW_STATUS_INVALID_ARGS;
    }
    return FPFW_STATUS_SUCCESS;
}

void data_proc_tlm_cmpnt_aggregate_pwr_tlm_data(void)
{
    // Allocate enough buffer for 8 strides from sensor fifo. Please review this if the sensor fifo may have more entries, optimize when less
    uint64_t buffer_data[MAX_BUFFER_ENTRIES];
    sensor_ram_poll_status_t status;

    // NOTE: All sensor fifo API to check and poll data availability is guaranteed to return
    //  more_entries as false once all entries that was latched during the initial call has been
    //  consumed. This guarantees that we can exit the loops within this API

    // Check and poll for tile temperatures
    do
    {
        tile_temp_t* temperature_data = (tile_temp_t*)buffer_data;
        uint16_t tile_index;
        status = sensor_fifo_svc_poll_tile_temperature(temperature_data, &tile_index);
        if (status.curr_data_is_valid == true)
        {
            // process the tile temperature
            tlm_logger_log_tile_temperature(temperature_data, tile_index);
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
            tlm_logger_log_tile_voltage(voltage_data, tile_index);
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
            // process the tile voltage
            tlm_logger_log_core_current(current_data, core_index);
        }
    } while (status.more_entries == true);

    // Check and poll for core pstate packets
    do
    {
        pstate_telem_t* state_data = (pstate_telem_t*)buffer_data;
        status = sensor_fifo_svc_poll_core_pstate(state_data);
        if (status.curr_data_is_valid == true)
        {
            // process the tile voltage
            tlm_logger_log_core_pstate(state_data);
        }
    } while (status.more_entries == true);

    // Check and poll for VR Temperatures
    do
    {
        vr_temp_t* vr_temperature = (vr_temp_t*)buffer_data;
        status = sensor_fifo_svc_poll_vr_temperature(vr_temperature);
        if (status.curr_data_is_valid == true)
        {
            // process the tile voltage
            tlm_logger_log_vr_temp(vr_temperature);
        }
    } while (status.more_entries == true);

    // Check and poll for VR Current and Voltage
    do
    {
        vr_current_t* vr_current = (vr_current_t*)buffer_data;
        status = sensor_fifo_svc_poll_vr_current(vr_current);
        if (status.curr_data_is_valid == true)
        {
            // process the tile voltage
            tlm_logger_log_vr_current(vr_current);
        }
    } while (status.more_entries == true);

    do
    {
        soc_pvt_temp_t* pvt_temperature = (soc_pvt_temp_t*)buffer_data;
        status = sensor_fifo_svc_poll_soc_pvt_temperature(pvt_temperature);
        if (status.curr_data_is_valid == true)
        {
            // process the soc PVT temperature
            tlm_logger_log_soc_pvt_temp(pvt_temperature);
        }
    } while (status.more_entries == true);

    /*For soc PVT voltage, no requirement as per the Telemetry spec */
    do
    {
        sensor_ram_dimm_info_t* dimm_info = (sensor_ram_dimm_info_t*)buffer_data;
        status = sensor_fifo_svc_poll_dimm_info(dimm_info);
        if (status.curr_data_is_valid == true)
        {
            telmain_log_dimm_info(dimm_info);
        }
    } while (status.more_entries == true);

    // run algorithms to update the aggregated telemetry data, used to generate packaged telemetry events.
    data_proc_tlm_cmpnt_aggregate_update_mgr();
}

void data_proc_tlm_cmpnt_aggregate_inst_tlm_data(void)
{
}

void data_proc_tlm_cmpnt_aggregate_24hr_tlm_data(void)
{
}

void data_proc_tlm_cmpnt_get_pwr_core_pstate_data(uint16_t core_id, pwr_core_element_pstate_t (*pstate_array)[NUMBER_OF_PSTATES])
{
    // parameter check: core_id, check if correct
    if (core_id >= NUMBER_OF_CORES_PER_DIE || pstate_array == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_CORE_PSTATE);
    }
    else
    {
        for (uint16_t pstate_index = 0; pstate_index < NUMBER_OF_PSTATES; pstate_index++)
        {
            (*pstate_array)[pstate_index].pstate_id = core[core_id].pstate[pstate_index].pstate_id;
            (*pstate_array)[pstate_index].avg_power_mW = core[core_id].pstate[pstate_index].avg_power_mW;
            (*pstate_array)[pstate_index].min_power_mW = core[core_id].pstate[pstate_index].min_power_mW;
            (*pstate_array)[pstate_index].max_power_mW = core[core_id].pstate[pstate_index].max_power_mW;
            (*pstate_array)[pstate_index].frequency_Mhz = core[core_id].pstate[pstate_index].frequency_Mhz;
            (*pstate_array)[pstate_index].residency_mS =
                ROUND_USEC_TO_MSEC(core[core_id].pstate[pstate_index].residency_uS);
            (*pstate_array)[pstate_index].entry_count = core[core_id].pstate[pstate_index].entry_count;
        }
    }
}

void data_proc_tlm_cmpnt_get_pwr_core_cstate_data(uint16_t core_id, pwr_core_element_cstate_t (*cstate_array)[NUMBER_OF_CSTATES])
{
    FPFW_UNUSED(core_id);
    FPFW_UNUSED(cstate_array);
}

void data_proc_tlm_cmpnt_get_pwr_core_throttle_data(uint16_t core_id,
                                                    pwr_core_element_throttle_t (*throttle_array)[NUMBER_OF_THROTTLE_TYPES])
{
    // parameter check: core_id, check if correct
    if (core_id >= NUMBER_OF_CORES_PER_DIE || throttle_array == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_CORE_THROTTLE);
    }
    else
    {
        for (uint16_t throttle_index = 0; throttle_index < NUMBER_OF_THROTTLE_TYPES; throttle_index++)
        {
            (*throttle_array)[throttle_index].avg_pstate = core[core_id].throttle_info[throttle_index].avg_pstate;
            (*throttle_array)[throttle_index].entry_count = core[core_id].throttle_info[throttle_index].entry_count;
            (*throttle_array)[throttle_index].exit_count = core[core_id].throttle_info[throttle_index].exit_count;
            (*throttle_array)[throttle_index].max_pstate = core[core_id].throttle_info[throttle_index].max_pstate;
            (*throttle_array)[throttle_index].reserved = core[core_id].throttle_info[throttle_index].reserved;
            (*throttle_array)[throttle_index].residency_mS = core[core_id].throttle_info[throttle_index].residency_mS;
            (*throttle_array)[throttle_index].type_id = core[core_id].throttle_info[throttle_index].type_id;
        }
    }
}

void data_proc_tlm_cmpnt_get_pwr_core_rack_priority_data(uint16_t core_id,
                                                         pwr_core_element_rack_priorities_t (*rack_priority_array)[NUMBER_OF_RACK_PRIORITIES])
{

    // parameter check: core_id, check if correct
    if (core_id >= NUMBER_OF_CORES_PER_DIE || rack_priority_array == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_CORE_RACK_PRIORITIES);
    }
    else
    {
        for (uint16_t rack_index = 0; rack_index < NUMBER_OF_RACK_PRIORITIES; rack_index++)
        {
            (*rack_priority_array)[rack_index].priority_id = core[core_id].priorities[rack_index].priority_id;
            (*rack_priority_array)[rack_index].avg_pstate = core[core_id].priorities[rack_index].avg_pstate;
            (*rack_priority_array)[rack_index].entry_count = core[core_id].priorities[rack_index].entry_count;
            (*rack_priority_array)[rack_index].exit_count = core[core_id].priorities[rack_index].exit_count;
            (*rack_priority_array)[rack_index].max_pstate = core[core_id].priorities[rack_index].max_pstate;
            (*rack_priority_array)[rack_index].reserved = core[core_id].priorities[rack_index].reserved;
            (*rack_priority_array)[rack_index].residency_mS = core[core_id].priorities[rack_index].residency_mS;
        }
    }
}

void data_proc_tlm_cmpnt_get_pwr_core_voltage_data(uint16_t core_id, p_pwr_core_element_voltage_t voltage_data)
{
    // parameter check: core_id, check if correct
    if (core_id >= NUMBER_OF_CORES_PER_DIE || voltage_data == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_CORE_VOLTAGE);
    }
    else
    {
        voltage_data->latest_value_mV = core[core_id].voltage.latest_value_mV;
        voltage_data->average_mV = core[core_id].voltage.average_mV;
        voltage_data->max_mV = core[core_id].voltage.max_mV;
        voltage_data->min_mV = core[core_id].voltage.min_mV;
    }
}

void data_proc_tlm_cmpnt_get_pwr_core_current_data(uint16_t core_id, p_pwr_core_element_current_t current_data)
{
    // parameter check: core_id, check if correct
    if (core_id >= NUMBER_OF_CORES_PER_DIE || current_data == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_CORE_CURRENT);
    }
    else
    {
        current_data->latest_value_mA = core[core_id].current.latest_value_mA;
        current_data->average_mA = core[core_id].current.average_mA;
        current_data->max_mA = core[core_id].current.max_mA;
        current_data->min_mA = core[core_id].current.min_mA;
    }
}

void data_proc_tlm_cmpnt_get_pwr_core_temperature_data(uint16_t core_id, p_pwr_core_element_temperature_t temperature_data)
{
    // parameter check: core_id, check if correct
    if (core_id >= NUMBER_OF_CORES_PER_DIE || temperature_data == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_CORE_TEMPERATURE);
    }
    else
    {
        temperature_data->latest_value_dC = core[core_id].temperature.latest_value_dC;
        temperature_data->average_dC = core[core_id].temperature.average_dC;
        temperature_data->max_dC = core[core_id].temperature.max_dC;
        temperature_data->min_dC = core[core_id].temperature.min_dC;
    }
}

void data_proc_tlm_cmpnt_get_pwr_core_histogram_data(
    uint16_t core_id,
    pwr_core_element_histogram_t (*histogram_array)[NUMBER_OF_HS_VOLTAGE_SCALES][NUMBER_OF_HS_TEMP_SCALES])
{
    FPFW_UNUSED(core_id);
    FPFW_UNUSED(histogram_array);
}

void data_proc_tlm_cmpnt_get_pwr_soc_pc3_data(p_pwr_soc_element_pc3_t soc_pc3_data)
{
    FPFW_UNUSED(soc_pc3_data);
}

void data_proc_tlm_cmpnt_get_pwr_soc_vr_rail_data(uint16_t rail_id, p_pwr_soc_element_vr_rail_t rail_data)
{
    // parameter check: rail_id, check if correct
    if (rail_id >= MAX_NUM_OF_VR_RAILS || rail_data == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_SOC_VR_RAILS);
    }
    else
    {
        // get VR Current. voltage and temperature entry.
        rail_data->current.average_mA = soc_info.rail[rail_id].current.average_mA;
        rail_data->current.latest_value_mA = soc_info.rail[rail_id].current.latest_value_mA;
        rail_data->current.max_mA = soc_info.rail[rail_id].current.max_mA;
        rail_data->current.min_mA = soc_info.rail[rail_id].current.min_mA;
        // get VR Temperature
        rail_data->temperature.latest_value_dC = soc_info.rail[rail_id].temperature.latest_value_dC;
        rail_data->temperature.average_dC = soc_info.rail[rail_id].temperature.average_dC;
        rail_data->temperature.max_dC = soc_info.rail[rail_id].temperature.max_dC;
        rail_data->temperature.min_dC = soc_info.rail[rail_id].temperature.min_dC;
        // get VR voltage
        rail_data->voltage.latest_value_mV = soc_info.rail[rail_id].voltage.latest_value_mV;
        rail_data->voltage.average_mV = soc_info.rail[rail_id].voltage.average_mV;
        rail_data->voltage.max_mV = soc_info.rail[rail_id].voltage.max_mV;
        rail_data->voltage.min_mV = soc_info.rail[rail_id].voltage.min_mV;
    }
}

void data_proc_tlm_cmpnt_get_pwr_soc_hnf_data(uint16_t hnf_channel, p_pwr_soc_element_hnf_t hnf_data)
{
    // parameter check: hnf_channel, check if correct
    if (hnf_channel >= NUMBER_OF_HNF_CHANNELS_PER_DIE || hnf_data == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_SOC_HNF);
    }
    else
    {
        hnf_data->latest_value_dC = soc_info.hnf[hnf_channel].latest_value_dC;
        hnf_data->average_dC = soc_info.hnf[hnf_channel].average_dC;
        hnf_data->max_dC = soc_info.hnf[hnf_channel].max_dC;
        hnf_data->min_dC = soc_info.hnf[hnf_channel].min_dC;
    }
}

void data_proc_tlm_cmpnt_get_pwr_soc_dimm_data(uint16_t dimm_channel, p_pwr_soc_element_dimm_t dimm_data)
{
    // parameter check: dimm_channel, check if correct
    if (dimm_channel >= NUMBER_OF_DIMM_CHANNELS || dimm_data == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_SOC_DIMM);
    }
    else
    {
        // DIMM temperature s0
        dimm_data->s0.latest_value_dC = soc_info.dimm[dimm_channel].s0.latest_value_dC;
        // DIMM temperature s1
        dimm_data->s1.latest_value_dC = soc_info.dimm[dimm_channel].s1.latest_value_dC;
        // DIMM freq,power,throttle info
        dimm_data->dimm_memory_frequency_id = soc_info.dimm[dimm_channel].dimm_memory_frequency_id;
        dimm_data->dimm_power_mW = soc_info.dimm[dimm_channel].dimm_power_mW;
        dimm_data->dimm_throttling = soc_info.dimm[dimm_channel].dimm_throttling;
    }
}

void data_proc_tlm_cmpnt_get_pwr_soc_snsr_temp_data(uint16_t sensor_id, p_pwr_soc_element_sensor_temp_t sensor_temp_data)
{
    // parameter check: sensor_id, check if correct
    if (sensor_id >= NUMBER_OF_SOC_TEMP_SENSORS || sensor_temp_data == NULL)
    {
        FPFW_ET_LOG(DataPackagePWRrecordError, POWER_TELEMETRY_ELEMENT_SOC_SENSOR_TEMP);
    }
    else
    {
        sensor_temp_data->latest_value_dC = soc_info.sensor_temp->latest_value_dC;
        sensor_temp_data->average_dC = soc_info.sensor_temp->average_dC;
        sensor_temp_data->max_dC = soc_info.sensor_temp->max_dC;
        sensor_temp_data->min_dC = soc_info.sensor_temp->min_dC;
    }
}

void data_proc_tlm_cmpnt_get_pwr_mpam_pstate_data(uint16_t mpam_id,
                                                  pwr_element_mpam_pstate_t (*mpam_pstate_array)[NUMBER_OF_PSTATES])
{
    FPFW_UNUSED(mpam_id);
    FPFW_UNUSED(mpam_pstate_array);
}

void data_proc_tlm_cmpnt_get_pwr_soc_mpam_throttle_data(uint16_t mpam_id, p_pwr_element_mpam_throttle_t mpam_throttle_data)
{
    FPFW_UNUSED(mpam_id);
    FPFW_UNUSED(mpam_throttle_data);
}

void data_proc_tlm_cmpnt_get_inst_soc_core_summary_data(uint16_t core_id, p_inst_core_element_summary_t core_summary_data)
{
    // parameter check: core_id, check if correct
    if (core_id >= NUMBER_OF_CORES_PER_DIE || core_summary_data == NULL)
    {
        FPFW_ET_LOG(DataPackageInstRecordError, INST_TELEMETRY_ELEMENT_CORE);
    }
    else
    {
        // Pstate and Cstate(TODO)
        core_summary_data->pc_state_info.pstate_id = core[core_id].pstate->pstate_id;
        core_summary_data->pc_state_info.frequency_Mhz = core[core_id].pstate->frequency_Mhz;
        core_summary_data->pc_state_info.power_mW = core[core_id].average_pwr_mW;
        core_summary_data->pc_state_info.pstate_residency_mS = core[core_id].pstate->residency_uS;
        core_summary_data->pc_state_info.cstate_plimit = core[core_id].active_sample_plimit;
        // force latency to zero.
        core_summary_data->pc_state_info.cstate_entry_latency_uS = 0;
        core_summary_data->pc_state_info.cstate_exit_latency_uS = 0;
        // Throttling and priorities
        core_summary_data->throttle_info.throttle_type_priority_id = core[core_id].throttling_priority_id;
        core_summary_data->throttle_info.throttle_type_residency_mS = core[core_id].throttle_info->residency_mS;
        core_summary_data->throttle_info.throttle_priority_residency_mS = 0;
        core_summary_data->throttle_info.throttle_start_stop_id = 0;
        // Voltage, current and Temperature
        core_summary_data->vct_info.vct_voltage_mV = core[core_id].voltage.latest_value_mV;
        core_summary_data->vct_info.vct_temperature_dC = core[core_id].temperature.latest_value_dC;
        core_summary_data->vct_info.vct_current_mA = core[core_id].current.latest_value_mA;
        core_summary_data->vct_info.vct_max_temp_dC = core[core_id].temperature.max_dC;
    }
}

void data_proc_tlm_cmpnt_get_inst_soc_rail_data(uint16_t rail_id, p_inst_soc_element_rail_t rail_data)
{
    // parameter check: core_id, check if correct
    if (rail_id >= MAX_NUM_OF_VR_RAILS || rail_data == NULL)
    {
        FPFW_ET_LOG(DataPackageInstRecordError, INST_TELEMETRY_ELEMENT_SOC_RAILS);
    }
    else
    {
        // Create Voltage, Current and Temperature Information
        rail_data->current_mA = soc_info.rail[rail_id].current.average_mA;
        rail_data->temperature_dC = soc_info.rail[rail_id].temperature.latest_value_dC;
        rail_data->voltage_mV = soc_info.rail[rail_id].voltage.latest_value_mV;
    }
}

void data_proc_tlm_cmpnt_get_inst_soc_dimm_runtime_data(uint16_t dimm_module, p_inst_soc_element_dimm_runtime_t dimm_data)
{
    // parameter check: dimm_channel, check if correct
    if (dimm_module >= NUMBER_OF_DIMM_CHANNELS || dimm_data == NULL)
    {
        FPFW_ET_LOG(DataPackageInstRecordError, INST_TELEMETRY_ELEMENT_SOC_DIMM_RT);
    }
    else
    {
        // DIMM temperature s0
        dimm_data->temp_s0_latest_dC = soc_info.dimm[dimm_module].s0.latest_value_dC;
        // DIMM temperature s1
        dimm_data->temp_s1_latest_dC = soc_info.dimm[dimm_module].s1.latest_value_dC;
        // DIMM freq,power,throttle info
        dimm_data->memory_freq_id = soc_info.dimm[dimm_module].dimm_memory_frequency_id;
        dimm_data->power_mW = soc_info.dimm[dimm_module].dimm_power_mW;
        // TODO: investigate flag :https://azurecsi.visualstudio.com/Dev/_workitems/edit/2297112
        dimm_data->throttling_flags = soc_info.dimm[dimm_module].dimm_throttling;
    }
}

void data_proc_tlm_cmpnt_get_inst_soc_dimm_config_data(p_inst_soc_element_dimm_config_t dimm_cfg)
{
    FPFW_UNUSED(dimm_cfg);
}

void data_proc_tlm_cmpnt_get_inst_soc_snsr_temp_data(uint16_t sensor_id, p_inst_soc_element_sensor_temp_t sensor_temp_data)
{
    // perf_soc_temp_fill_data for sensor
    if (sensor_id >= NUMBER_OF_SOC_TEMP_SENSORS || sensor_temp_data == NULL)
    {
        FPFW_ET_LOG(DataPackageInstRecordError, INST_TELEMETRY_ELEMENT_SOC_TEMP_SENSOR);
    }
    else
    {
        sensor_temp_data->latest_value_dC = soc_info.sensor_temp->latest_value_dC;
        sensor_temp_data->average_dC = soc_info.sensor_temp->average_dC;
        sensor_temp_data->max_dC = soc_info.sensor_temp->max_dC;
        sensor_temp_data->min_dC = soc_info.sensor_temp->min_dC;
    }
}

void data_proc_tlm_cmpnt_get_inst_core_amu_data(uint16_t core_id, p_inst_core_element_amu_counters_t amu_data)
{
    FPFW_UNUSED(core_id);
    FPFW_UNUSED(amu_data);
}

//----------------Power telemetry update manager  ----------------

void tlm_update_mpam_residency(uint8_t core_id, uint16_t mpam_id, uint8_t pstate)
{

    FPFW_UNUSED(core_id);
    FPFW_UNUSED(mpam_id);
    FPFW_UNUSED(pstate);
    // Update the core - mpam - pstate instantaneous power
    // TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2319779
}

void tlm_update_pstate_core_power(uint8_t core_id, uint8_t pstate_index)
{

    pwr_pstate_t* pstate = &core[core_id].pstate[pstate_index];
    uint16_t latest_value_mW = pstate->latest_value_mW;

    // Update min, max, and average power values
    if (latest_value_mW < pstate->min_power_mW || pstate->min_power_mW == 0)
    {
        pstate->min_power_mW = latest_value_mW;
    }
    if (latest_value_mW > pstate->max_power_mW)
    {
        pstate->max_power_mW = latest_value_mW;
    }

    // Calculate the weighted average power value
    if (core[core_id].num_pwr_samples <= 1)
    {
        pstate->avg_power_mW = latest_value_mW;
    }
    else
    {
        uint32_t weighted_previous_average = (uint32_t)pstate->avg_power_mW * (core[core_id].num_pwr_samples - 1);
        uint32_t weighted_latest_value = latest_value_mW;
        uint32_t new_avg_power_mW = (weighted_previous_average + weighted_latest_value) / core[core_id].num_pwr_samples;

        // Ensure the calculated average does not exceed UINT16_MAX
        if (new_avg_power_mW > UINT16_MAX)
        {
            new_avg_power_mW = UINT16_MAX;
            FPFW_ET_LOG(PstatePWRUpdateMMAvgOverflow);
        }

        // Check if the calculated average goes lower but not lower than the new value
        if (new_avg_power_mW < latest_value_mW)
        {
            pstate->avg_power_mW = latest_value_mW;
        }
        else
        {
            pstate->avg_power_mW = new_avg_power_mW;
        }
    }
}

void tlm_calculate_mma_res(uint16_t* mma_min, uint16_t* mma_max, uint16_t* mma_average, uint16_t* mma_latest_value, uint32_t time_diff_uS, uint32_t residency_uS)
{
    // Check parameter bounds
    if (time_diff_uS == 0 || time_diff_uS > UINT32_MAX || residency_uS > UINT32_MAX)
    {
        // FPFW_ET_LOG(DataUpdateMMAWrongInValidTimeStamp); //TODO: re-enable once  test systems support it
    }
    // Calculate the average
    if (*mma_latest_value != 0)
    {
        // Check for maximum
        if (*mma_latest_value > *mma_max)
        {
            *mma_max = *mma_latest_value;
        }
        // Check for minimum
        if ((*mma_latest_value < *mma_min || *mma_min == 0))
        {
            *mma_min = *mma_latest_value;
        }
        // Check to calculate average
        if ((residency_uS > time_diff_uS) && (residency_uS != 0) && (*mma_average != 0))
        {
            uint32_t weighted_previous_average = (uint32_t)*mma_average * (residency_uS - time_diff_uS);
            uint32_t weighted_latest_value = (uint32_t)*mma_latest_value * time_diff_uS;
            uint32_t average = (weighted_previous_average + weighted_latest_value) / residency_uS;
            // Ensure the calculated average does not exceed UINT16_MAX
            if (average > UINT16_MAX)
            {
                average = UINT16_MAX;
                FPFW_ET_LOG(DataUpdateMMAvgOverflow);
            }
            *mma_average = (uint16_t)average;
        }
        else
        {
            *mma_average = *mma_latest_value;
        }
    }
}

void tlm_update_cstate(uint8_t core_id, uint64_t time_stamp_uS)
{
    uint8_t cstate_index = core[core_id].cstate_from_pstate_pkt;
    if (core[core_id].flags.cstate_change == 0)
    {
        uint64_t cstate_time_diff_uS = 0;
        // Update the cstate residency, cstate power not needed for kingsgate.
        if (core[core_id].cstate_timestamp_uS != 0 && (time_stamp_uS > core[core_id].cstate_timestamp_uS))
        {
            cstate_time_diff_uS = time_stamp_uS - core[core_id].cstate_timestamp_uS;
            core[core_id].cstate[cstate_index].residency_uS += cstate_time_diff_uS;
        }
    }
    else
    {
        // Clear the cstate change indicator for this core
        core[core_id].flags.cstate_change = 0;
    }
    core[core_id].cstate_timestamp_uS = time_stamp_uS;
}

void tlm_update_core_current(uint8_t core_id, uint32_t time_diff_uS, uint32_t residency_uS)
{
    /* For core current :min, max avg calculation*/
    // Update the core current min, max average.
    tlm_calculate_mma_res(&core[core_id].current.min_mA,
                          &core[core_id].current.max_mA,
                          &core[core_id].current.average_mA,
                          &core[core_id].current.latest_value_mA,
                          time_diff_uS,
                          residency_uS);
}
void tlm_update_core_voltage(uint8_t core_id, uint32_t time_diff_uS, uint32_t residency_uS)
{
    /* For core voltage :min, max avg calculation*/
    tlm_calculate_mma_res(&core[core_id].voltage.min_mV,
                          &core[core_id].voltage.max_mV,
                          &core[core_id].voltage.average_mV,
                          &core[core_id].voltage.latest_value_mV,
                          time_diff_uS,
                          residency_uS);
}

void tlm_update_core_temperature(uint8_t core_id, uint32_t time_diff_uS, uint32_t residency_uS)
{
    /* For core temperature :min, max avg calculation*/
    tlm_calculate_mma_res(&core[core_id].temperature.min_dC,
                          &core[core_id].temperature.max_dC,
                          &core[core_id].temperature.average_dC,
                          &core[core_id].temperature.latest_value_dC,
                          time_diff_uS,
                          residency_uS);
}

void tlm_update_tile_vcpu(uint8_t tile_id, uint32_t time_diff_uS, uint32_t residency_uS)
{
    /* For tile vcpu :min, max avg calculation*/
    // Update the vcpu voltage  min, max average
    tlm_calculate_mma_res(&tile[tile_id].vcpu.min_mV,
                          &tile[tile_id].vcpu.max_mV,
                          &tile[tile_id].vcpu.average_mV,
                          &tile[tile_id].vcpu.latest_value_mV,
                          time_diff_uS,
                          residency_uS);
}

void tlm_update_tile_vsys(uint8_t tile_id, uint32_t time_diff_uS, uint32_t residency_uS)
{
    /* For tile vsys :min, max avg calculation*/
    // Update the vsys voltage min, max average
    tlm_calculate_mma_res(&tile[tile_id].vsys.min_mV,
                          &tile[tile_id].vsys.max_mV,
                          &tile[tile_id].vsys.average_mV,
                          &tile[tile_id].vsys.latest_value_mV,
                          time_diff_uS,
                          residency_uS);
}

/* SOC VR rails update */
void tlm_update_soc_rails_voltage(uint8_t vr_index, uint32_t time_diff_uS, uint32_t residency_uS)
{
    /* For soc vr rail voltage :min, max avg calculation*/
    // Update the rail voltage min, max average
    tlm_calculate_mma_res(&soc_info.rail[vr_index].voltage.min_mV,
                          &soc_info.rail[vr_index].voltage.max_mV,
                          &soc_info.rail[vr_index].voltage.average_mV,
                          &soc_info.rail[vr_index].voltage.latest_value_mV,
                          time_diff_uS,
                          residency_uS);
}

void tlm_update_soc_rails_current(uint8_t vr_index, uint32_t time_diff_uS, uint32_t residency_uS)
{
    /* For soc vr rail current:min, max avg calculation*/
    // Update the rail current min, max average
    tlm_calculate_mma_res(&soc_info.rail[vr_index].current.min_mA,
                          &soc_info.rail[vr_index].current.max_mA,
                          &soc_info.rail[vr_index].current.average_mA,
                          &soc_info.rail[vr_index].current.latest_value_mA,
                          time_diff_uS,
                          residency_uS);
}

void tlm_update_soc_rails_temperature(uint8_t vr_index, uint32_t time_diff_uS, uint32_t residency_uS)
{
    /* For soc vr rail temperature:min, max avg calculation*/
    // Update the vr rails temperature min, max average
    tlm_calculate_mma_res(&soc_info.rail[vr_index].temperature.min_dC,
                          &soc_info.rail[vr_index].temperature.max_dC,
                          &soc_info.rail[vr_index].temperature.average_dC,
                          &soc_info.rail[vr_index].temperature.latest_value_dC,
                          time_diff_uS,
                          residency_uS);
}

void tlm_update_soc_hnf_temperature(uint8_t hnf_index, uint32_t time_diff_uS, uint32_t residency_uS)
{
    /* For soc hnf temperature :min, max avg calculation*/
    // Update the hnf temp min, max average
    tlm_calculate_mma_res(&soc_info.hnf[hnf_index].min_dC,
                          &soc_info.hnf[hnf_index].max_dC,
                          &soc_info.hnf[hnf_index].average_dC,
                          &soc_info.hnf[hnf_index].latest_value_dC,
                          time_diff_uS,
                          residency_uS);
}

void tlm_update_soc_pvt_temperature(uint8_t pvt_index, uint32_t time_diff_uS, uint32_t residency_uS)
{
    /* For soc pvt temperature :min, max avg calculation*/
    // Update the soc pvt temp min, max average
    // Store new values
    tlm_calculate_mma_res(&soc_info.sensor_temp[pvt_index].min_dC,
                          &soc_info.sensor_temp[pvt_index].max_dC,
                          &soc_info.sensor_temp[pvt_index].average_dC,
                          &soc_info.sensor_temp[pvt_index].latest_value_dC,
                          time_diff_uS,
                          residency_uS);
}

void tlm_update_soc_dimm_info(uint8_t dimm_module_index, uint32_t time_diff_uS, uint32_t residency_uS)
{
    /* For soc dimm info :min, max avg calculation :Update each temperature data for S0 and S1*/
    // Update the soc dimm info min, max average
    tlm_calculate_mma_res(&soc_info.dimm[dimm_module_index].s0.min_dC,
                          &soc_info.dimm[dimm_module_index].s0.max_dC,
                          &soc_info.dimm[dimm_module_index].s0.average_dC,
                          &soc_info.dimm[dimm_module_index].s0.latest_value_dC,
                          time_diff_uS,
                          residency_uS);
    // Update each temperature data for S1
    tlm_calculate_mma_res(&soc_info.dimm[dimm_module_index].s1.min_dC,
                          &soc_info.dimm[dimm_module_index].s1.max_dC,
                          &soc_info.dimm[dimm_module_index].s1.average_dC,
                          &soc_info.dimm[dimm_module_index].s1.latest_value_dC,
                          time_diff_uS,
                          residency_uS);
}

void tlm_update_pstate(uint8_t core_id, uint64_t time_stamp_uS)
{
    // Check if the current core has a pstate update
    // Update the PState Residency
    uint8_t pstate_index;
    // Check first if we are throttling
    if (core[core_id].throttling_status == NO_THROTTLE)
    {
        /* get pstate from pstate packet/pstate fifo */
        pstate_index = core[core_id].pstate_from_pstate_pkt;
    }
    else
    {
        /* in case of throttle pstate is not reported by pstate packet, get from current telemetry packet*/
        pstate_index = core[core_id].pstate_from_current_pkt;
    }

    if (core[core_id].flags.id_change_bit == 0 && core[core_id].flags.pstate_change == 0)
    {
        /*Update the current pstate residencies*/
        update_core_pstate_timestamps(core_id, pstate_index, time_stamp_uS);
        // Check if there are power samples for this core
        if (core[core_id].num_pwr_samples > 0)
        {
            /* average of all the sample reported so far*/
            core[core_id].average_pwr_mW = (core_pwr_samples_accumulation_mW[core_id] / core[core_id].num_pwr_samples);
        }
        /* core[core_id].average_power_samples_value has running average of the power samples, update latest_value_mW */
        core[core_id].pstate[pstate_index].latest_value_mW = core[core_id].average_pwr_mW;
        if (core[core_id].average_pwr_mW)
        {
            tlm_update_pstate_core_power(core_id, pstate_index);
        }
        // Update the core - mpam - pstate instantaneous power
        tlm_update_mpam_residency(core_id, core[core_id].active_sample_mpam_id, pstate_index);
    }
    else
    {
        // Clear the pstate change indicator for this core
        core[core_id].flags.id_change_bit = 0;
        core[core_id].flags.pstate_change = 0;
        pstate_accum_uS[core_id][pstate_index] = 0;
    }
}

void tlm_update_throttling(uint8_t core_id, uint64_t time_stamp_uS)
{
    FPFW_UNUSED(core_id);
    FPFW_UNUSED(time_stamp_uS);
    /* TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2319985 */
}

void tlm_update_core_histogram(uint8_t core_id)
{
    FPFW_UNUSED(core_id);
    // TODO:https://azurecsi.visualstudio.com/Dev/_workitems/edit/2319991
}

static uint64_t tlm_calculate_time_diff(uint64_t* previous_timestamp_uS, uint64_t* time_stamp_uS, pwr_tlm_update_t update_type)
{
    uint64_t temp_stamp_uS = exec_tlm_cmpnt_get_timestamp_microseconds();
    uint64_t time_diff_uS = 0;

    // Calculate the general residency for the core
    if (*previous_timestamp_uS != 0 && (temp_stamp_uS > *previous_timestamp_uS))
    {
        time_diff_uS = temp_stamp_uS - *previous_timestamp_uS;

        if (update_type == PWR_TLM_SOC_UPDATE)
        {
            soc_info.time_counter_uS += time_diff_uS;
        }
    }

    *previous_timestamp_uS = temp_stamp_uS;
    *time_stamp_uS = temp_stamp_uS;

    return time_diff_uS;
}

void tlm_soc_component_update(void)
{
    static uint64_t previous_soc_timestamp_uS = 0;
    uint64_t time_stamp_uS = 0;

    // calculate the timestamp and time difference
    uint64_t time_diff_uS = tlm_calculate_time_diff(&previous_soc_timestamp_uS, &time_stamp_uS, PWR_TLM_SOC_UPDATE);

    // Update the Rail information
    for (uint8_t vr_index = 0; vr_index < MAX_NUM_OF_VR_RAILS; vr_index++)
    {
        tlm_update_soc_rails_voltage(vr_index, time_diff_uS, soc_info.time_counter_uS);
        tlm_update_soc_rails_current(vr_index, time_diff_uS, soc_info.time_counter_uS);
        tlm_update_soc_rails_temperature(vr_index, time_diff_uS, soc_info.time_counter_uS);
    }

    // Update the HNF Temperature information'
    for (uint8_t hnf_index = 0; hnf_index < NUMBER_OF_HNF_CHANNELS_PER_DIE; hnf_index++)
    {
        tlm_update_soc_hnf_temperature(hnf_index, time_diff_uS, soc_info.time_counter_uS);
    }

    // Update the PVT Sensor information'
    for (uint8_t pvt_index = 0; pvt_index < NUMBER_OF_SOC_TEMP_SENSORS; pvt_index++)
    {
        tlm_update_soc_pvt_temperature(pvt_index, time_diff_uS, soc_info.time_counter_uS);
    }

    // Update for DIMM temperature
    for (uint8_t dimm_module_index = 0; dimm_module_index < NUMBER_OF_DIMM_MODULES; dimm_module_index++)
    {
        // Update each temperature data for S0 and S1.
        tlm_update_soc_dimm_info(dimm_module_index, time_diff_uS, soc_info.time_counter_uS);
    }

    // Check current cstate for all cores
    // only if ALL cores are in pc3, increment residency
    // TODO:https://azurecsi.visualstudio.com/Dev/_workitems/edit/2023433
}

void tlm_core_component_update(void)
{
    static uint64_t previous_core_timestamp_uS = 0;
    uint64_t time_stamp_uS = 0;
    // calculate the timestamp and time difference
    uint64_t time_diff_uS = tlm_calculate_time_diff(&previous_core_timestamp_uS, &time_stamp_uS, PWR_TLM_CORE_UPDATE);

    // Go over all cores
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        /* Calculate  the general residency for the core*/
        core[core_id].time_counter_uS += time_diff_uS;

        /* Do not do any update for this core if the current packet timestamp is zero */
        if (core[core_id].current_pkt_timestamp != 0)
        {
            /* update pstate residency and power */
            tlm_update_pstate(core_id, time_stamp_uS);

            /* update cstate residency only, power not required for KNG.*/
            tlm_update_cstate(core_id, time_stamp_uS);

            // Check to update throttling and priorities
            tlm_update_throttling(core_id, time_stamp_uS);

            /* update Core current*/
            tlm_update_core_current(core_id, time_diff_uS, core[core_id].time_counter_uS);
        }

        /* Note that even if a core is disabled, voltage and temperature sensors
            are still running on those disabled cores */

        // Check to update Core Voltage
        tlm_update_core_voltage(core_id, time_diff_uS, core[core_id].time_counter_uS);

        // Check to update Core temperature
        tlm_update_core_temperature(core_id, time_diff_uS, core[core_id].time_counter_uS);

        // update residency to generate volt/temp histogram
        tlm_update_core_histogram(core_id);
    }
}

void tlm_tile_component_update(void)
{
    static uint64_t previous_tile_timestamp_uS = 0;
    uint64_t time_stamp_uS = 0;

    uint64_t time_diff_uS = tlm_calculate_time_diff(&previous_tile_timestamp_uS, &time_stamp_uS, PWR_TLM_TILE_UPDATE);

    // Go over all tiles
    for (uint8_t tile_id = 0; tile_id < NUMBER_OF_TILES_PER_DIE; tile_id++)
    {
        // update the time counter

        tile[tile_id].time_counter_uS += time_diff_uS;
        if (tile[tile_id].active_sample_max_temperature_dC > tile[tile_id].max_tile_temperature_dC)
        {
            // Update the new Max tile temperature
            tile[tile_id].max_tile_temperature_dC = tile[tile_id].active_sample_max_temperature_dC;
            tile[tile_id].max_tile_id = tile[tile_id].active_sample_max_id;
        }

        // Update the tile Vcpu and Vsys MMA
        tlm_update_tile_vcpu(tile_id, time_diff_uS, tile[tile_id].time_counter_uS);
        tlm_update_tile_vsys(tile_id, time_diff_uS, tile[tile_id].time_counter_uS);
    }
}

void data_proc_tlm_cmpnt_aggregate_update_mgr(void)
{
    tlm_core_component_update();
    tlm_tile_component_update();
    tlm_soc_component_update();
}