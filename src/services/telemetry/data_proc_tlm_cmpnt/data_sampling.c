//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file tlm_logger.c
 * Consume sensor fifo data and log telemetry data
 */

/*------------- Includes -----------------*/
#include "compute_metrics_i.h"
#include "data_sampling_i.h" // internal APIs
#include "data_utilities_i.h"
#include "telemetry_events_i.h"

#include <FpFwAssert.h>
#include <assert.h> // IWYU pragma: keep for static_assert
#include <dvfs.h>
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
dts_tlm_coeff_t tileDtsCoefficients[NUMBER_OF_TILES_PER_DIE] = {0};

/* per core power samples's averaged value in mW,  used for averaging , samples are  averaged and multiplied
by a fator(22 , CORE_POWER_MW_PER_BIT) to get in mW*/
uint32_t core_pwr_samples_accumulation_mW[NUMBER_OF_CORES_PER_DIE] = {0}; // power samples values  in mW .
/* used only for MPAM*/
uint32_t pstate_accum_uS[NUMBER_OF_CORES_PER_DIE][NUMBER_OF_PSTATES] = {0};

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

    /* Initialize the data that doesn't update */
    data_smpl_init_constants();
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

void data_smpl_init_constants()
{
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        for (uint8_t cstate_id = 0; cstate_id < NUMBER_OF_CSTATES; cstate_id++)
        {
            core[core_id].cstate[cstate_id].cstate_id = cstate_id;
        }

        for (uint8_t pstate_id = 0; pstate_id < NUMBER_OF_PSTATES; pstate_id++)
        {
            core[core_id].pstate[pstate_id].pstate_id = pstate_id;
            core[core_id].pstate[pstate_id].frequency_Mhz = dvfs_get_freq_from_plimit(pstate_id);
        }
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
            data_smpl_parse_tile_temperature_entry(temperature_data, tile_index);
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
            data_smpl_parse_tile_voltage_entry(voltage_data, tile_index);
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
            // process the core current
            data_smpl_parse_core_current_entry(current_data, core_index);
        }
    } while (status.more_entries == true);

    // Check and poll for core pstate packets
    do
    {
        pstate_telem_t* state_data = (pstate_telem_t*)buffer_data;
        status = sensor_fifo_svc_poll_core_pstate(state_data);
        if (status.curr_data_is_valid == true)
        {
            // process the core states(pstate/cstate)
            data_smpl_parse_core_states_entry(state_data);
        }
    } while (status.more_entries == true);

    // Check and poll for VR Temperatures
    do
    {
        vr_temp_t* vr_temperature = (vr_temp_t*)buffer_data;
        status = sensor_fifo_svc_poll_vr_temperature(vr_temperature);
        if (status.curr_data_is_valid == true)
        {
            // process the VR Temperatures
            data_smpl_parse_vr_temperature_entry(vr_temperature);
        }
    } while (status.more_entries == true);

    // Check and poll for VR Current and Voltage
    do
    {
        vr_current_t* vr_current = (vr_current_t*)buffer_data;
        status = sensor_fifo_svc_poll_vr_current(vr_current);
        if (status.curr_data_is_valid == true)
        {
            // process the VR Current and Voltage
            data_smpl_parse_vr_current_entry(vr_current);
        }
    } while (status.more_entries == true);

    do
    {
        soc_pvt_temp_t* pvt_temperature = (soc_pvt_temp_t*)buffer_data;
        status = sensor_fifo_svc_poll_soc_pvt_temperature(pvt_temperature);
        if (status.curr_data_is_valid == true)
        {
            // process the soc PVT temperature
            data_smpl_parse_pvt_temperature_entry(pvt_temperature);
        }
    } while (status.more_entries == true);

    /*For soc PVT voltage, no requirement as per the Telemetry spec */

    do
    {
        sensor_ram_dimm_info_t* dimm_info = (sensor_ram_dimm_info_t*)buffer_data;
        status = sensor_fifo_svc_poll_dimm_info(dimm_info);
        if (status.curr_data_is_valid == true)
        {
            data_smpl_parse_dimm_entry(dimm_info);
        }
    } while (status.more_entries == true);

    // run algorithms to update the aggregated telemetry data, used to generate packaged telemetry events.
    comp_metrics_for_sample_period();
}

// sensor fifo parsers

void data_smpl_parse_tile_temperature_entry(tile_temp_t* temperature_data, uint8_t tile_index)
{
    // For all details on the reference how this code was implemented, please
    // refer to the Power Management, Power Telemetry and Sensor Hardware Architecture Specifications (HAS)

    // Check first if our tile number is correct
    if (tile_index >= NUMBER_OF_TILES_PER_DIE)
    {
        FPFW_ET_LOG(LogInvalidTileId, tile_index);
        return;
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
        uint16_t inst_temp_0_dC = PWR_TLM_DOUT2TEMP_FUSED_DC(temperature_data->temp1.temp0,
                                                             tileDtsCoefficients[tile_index].k_val,
                                                             tileDtsCoefficients[tile_index].y_val);
        uint16_t inst_temp_1_dC = PWR_TLM_DOUT2TEMP_FUSED_DC(temperature_data->temp1.temp1,
                                                             tileDtsCoefficients[tile_index].k_val,
                                                             tileDtsCoefficients[tile_index].y_val);
        uint16_t inst_temp_2_dC = PWR_TLM_DOUT2TEMP_FUSED_DC(temperature_data->temp1.temp2,
                                                             tileDtsCoefficients[tile_index].k_val,
                                                             tileDtsCoefficients[tile_index].y_val);

        core[core_id].temperature.latest_value_dC = data_util_get_max_val(inst_temp_0_dC, inst_temp_1_dC, inst_temp_2_dC);

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
        core[core_id + 1].temperature.latest_value_dC =
            data_util_get_max_val(inst_temp_0_dC, inst_temp_1_dC, inst_temp_2_dC);

        // Log all the HNF Temperature, review raw HNF data for units.
        // TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2289685
        soc_info.hnf[core_id].latest_value_dC = temperature_data->temp2.temp6;
        soc_info.hnf[core_id + 1].latest_value_dC = temperature_data->temp2.temp7;
    }

    // Also store the Max tile temperatures and its ID
    tile[tile_index].active_sample_max_temperature_dC = temperature_data->temp0.max_temp;
    tile[tile_index].active_sample_max_id = temperature_data->temp0.max_id;
}

void data_smpl_parse_tile_voltage_entry(tile_voltage_t* voltage_data, uint8_t tile_index)
{
    // For all details on the reference how this code was implemented, please
    // refer to the Power Management, Power Telemetry and Sensor Hardware Architecture Specifications (HAS)

    // Check first if our tile number is correct
    if (tile_index >= NUMBER_OF_TILES_PER_DIE)
    {
        FPFW_ET_LOG(LogInvalidTileId, tile_index);
        return;
    }

    // Since this is a tile voltage, log the core where the tile voltage belongs
    uint8_t core_id = tile_index * 2;
    core[core_id].voltage.latest_value_mV = DOUT2MILLIVOLTS(voltage_data->data.vcore0);
    core[core_id + 1].voltage.latest_value_mV = DOUT2MILLIVOLTS(voltage_data->data.vcore1);

    // Log the tile vcpu and vsys
    tile[tile_index].vcpu.latest_value_mV = DOUT2MILLIVOLTS(voltage_data->data.vcpu);
    tile[tile_index].vsys.latest_value_mV = DOUT2MILLIVOLTS(voltage_data->data.vsys);
}

void data_smpl_parse_core_current_entry(core_current_t* current_data, uint8_t core_index)
{
    // Each index here refers to the core, check if correct
    if (core_index >= NUMBER_OF_CORES_PER_DIE)
    {
        FPFW_ET_LOG(LogInvalidCoreId, core_index);
        return;
    }

    uint8_t core_id = core_index;
    core[core_id].current_pkt_timestamp = current_data->timestamp;
    if (current_data->timestamp == 0)
    {
        return;
    }

    // Get the current conversions. Conversion factors for the currents needs to be fine tuned
    // by the SVT and Silicon team.
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
        // SCF RAM Current Telemetry packet that doesn't indicate the change bit set but
        // a Pstate change happens at the end of the Current telemetry sampling. For this
        // situation, we consider the power measurement to be valid and should belong to
        // the previous pstate before the change. On situations where the core is throttling,
        // pstate change packets may not appear or be delayed, while current telemetry may
        // indicate a change on pstate first. - We may revisit this after the real silicon for
        // KNG comes in https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1753817
        //
        if (core[core_id].pstate_from_current_pkt != current_data->data.pstate)
        {
            core[core_id].flags.pstate_change = 1;
            core[core_id].pstate_timestamp_uS = current_data->timestamp;
        }
    }

    // The average current reported from SCF is the average of the span of time
    // of measurement window therefore, we will treat this as the
    // instantaneous current @ "x" mS, depend  on the sampling rate, which will be in the range of 3 ~ 5mS.
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
}

void data_smpl_parse_core_states_entry(pstate_telem_t* pstate_telemetry)
{
    // Convert Sensor Data into Pstate Entry
    uint8_t core_id = pstate_telemetry->data.core;
    if (core_id >= NUMBER_OF_CORES_PER_DIE)
    {
        FPFW_ET_LOG(LogCoreThrottleInValidCoreId, core_id);
        return;
    }
    // Check for throttling indication first. If System is throttling, do not
    // take snapshots of Pstates and Cstates
    pstate_throttle_status_t status = pstate_telemetry->data.throttle_status;

    switch (status)
    {
    case NO_THROTTLING:
        // log pstate
        data_smpl_parse_pstate_no_throttling(pstate_telemetry);
        // log cstate
        data_smpl_parse_cstate_no_throttling(pstate_telemetry);
        break;
    case CURRENT_THROTTLING_START:
    case TEMP_THROTTLING_START:
    case SYS_FRC_PMIN_THROTTLING_START:
    case ADPT_CLK_THROTTLING_START:
    case CURRENT_THROTTLING_END:
    case TEMP_THROTTLING_END:
    case SYS_FRC_PMIN_THROTTLE_END:
    case ADPT_CLK_THROTTLE_END:
    case CURRENT_THROTTLING_OVERRUN: // TODO:https://azurecsi.visualstudio.com/Dev/_workitems/edit/2571672
    case ADPT_CLK_THROTTLING_OVERRUN:
        // Throttling information is logged based on status code (status_index)
        data_smpl_parse_throttling(pstate_telemetry);
        break;

    case RACK_THROTTLING_START:
    case RACK_THROTTLING_END:
        // TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2561396
        break;
    default:
        break;
    }
    //! update  throttling status
    core[core_id].throttling_status = pstate_telemetry->data.throttle_status;
}

void data_smpl_parse_vr_temperature_entry(vr_temp_t* vr_temperature)
{
    // Extract VR Temperature entries for all VR Rails
    for (uint8_t vr_index = 0; vr_index < MAX_NUM_OF_VR_RAILS; vr_index++)
    {
        soc_info.rail[vr_index].temperature.latest_value_dC = vr_temperature->vr_temp_dC[vr_index];
    }
}

void data_smpl_parse_vr_current_entry(vr_current_t* vr_current)
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
void data_smpl_parse_pvt_temperature_entry(soc_pvt_temp_t* pvt_temperature)
{
    for (uint8_t pvt_index = 0; pvt_index < NUMBER_OF_SOC_TEMP_SENSORS; pvt_index++)
    {
        soc_info.sensor_temp[pvt_index].latest_value_dC = pvt_temperature->sensor_temp_dC[pvt_index];
    }
}

void data_smpl_parse_dimm_entry(sensor_ram_dimm_info_t* dimm_info)
{
    // TODO: update via https://azurecsi.visualstudio.com/Dev/_workitems/edit/2592133

    // DIMM(Dual inline memory module)- Convert Sensor Data into DIMM Entries
    // Check for which DIMM channel is this entry, do not process anything beyond the valid DIMM index
    if (dimm_info->dimm_id < NUMBER_OF_DIMM_MODULES)
    {
        soc_info.dimm[dimm_info->dimm_id].s0.latest_value_dC = dimm_info->dimm_temp_s0_dC;
        soc_info.dimm[dimm_info->dimm_id].s1.latest_value_dC = dimm_info->dimm_temp_s1_dC;
    }
    else
    {
        FPFW_ET_LOG(DIMMInfoInvalidDimmId, FPFW_STATUS_INVALID_ARGS);
    }
}

// parser helper functions

void data_smpl_parse_pstate_no_throttling(pstate_telem_t* pstate_telemetry)
{
    // Power information per P State Per Core is updated based on current
    // telemetry (which also provides power information). See the update
    // in tlm_logger_chk_upd_pstate().

    // Convert Sensor Data into Pstate Entry
    uint8_t core_id = pstate_telemetry->data.core;
    if (core_id >= NUMBER_OF_CORES_PER_DIE)
    {
        FPFW_ET_LOG(LogCoreThrottleInValidCoreId, core_id);
        return;
    }

    // take snapshots of Pstates and Cstates
    // check to log the Core Pstate if there are changes
    if (pstate_telemetry->data.pstate != core[core_id].pstate_from_pstate_pkt)
    {
        core[core_id].pstate_from_pstate_pkt = pstate_telemetry->data.pstate;
        core[core_id].pstate[pstate_telemetry->data.pstate].entry_count++;
        // TODO: Investigation task to check if we need an API for timestamp conversion :
        // https://azurecsi.visualstudio.com/Dev/_workitems/edit/2571666
        core[core_id].pstate_timestamp_uS = pstate_telemetry->timestamp;
    }

    // if previous throttling status was throttling, check all type of active throttling
    // end and update the tracker.
    if (core[core_id].throttling_status != NO_THROTTLING)
    {
        data_smpl_parse_throttling_exit_transition(core_id, pstate_telemetry->timestamp);
    }
    // Save the current plimit
    core[core_id].active_sample_plimit = pstate_telemetry->data.plimit;
}

void data_smpl_parse_cstate_no_throttling(pstate_telem_t* cstate_telemetry)
{
    // Power information per P State Per Core is updated based on current
    // telemetry (which also provides power information). See the update

    // Convert Sensor Data into Pstate Entry
    uint8_t core_id = cstate_telemetry->data.core;
    if (core_id >= NUMBER_OF_CORES_PER_DIE)
    {
        FPFW_ET_LOG(LogCoreThrottleInValidCoreId, core_id);
        return;
    }

    // Check for throttling indication first. If System is throttling, do not
    // take snapshots of Pstates and Cstates
    if (cstate_telemetry->data.throttle_status == NO_THROTTLE)
    {
        /* Log cstate */
        uint8_t new_cstate = cstate_telemetry->data.cstate;
        uint64_t timestamp_us = cstate_telemetry->timestamp;

        if (new_cstate != core[core_id].cstate_from_pstate_pkt) // if there is a change in cstate, we have to handle it.
        {

            /*Note : There will be no valid case for CState to be changing between C1, C2 and C3 directly.
                    Everything should go through C0 first before we can get into C1 or C2 or C3, so if there
                    are cases such as C1 to C3 then we discard that kind of calculations.
                    Hardware logic is responsible to make sure change happen in proper order.
                    TODO: only logging warining but updating calculation, need to be updated
                    based on real time scenario.
                    https://azurecsi.visualstudio.com/Dev/_workitems/edit/2507082
            */
            // case 1 : Invalid cstate change- return.
            if (new_cstate > core[core_id].cstate_from_pstate_pkt + 1)
            {
                FPFW_ET_LOG(CstateUnexpectedLevelChange);
                return;
            }
            // case 2 : First time entering in a cstate -if timestamp is 0 so continue and update entry and
            // timestamp. At first entry on start all core timestamp are 0 , so log entry,timestamp and state.
            if (core[core_id].cstate_timestamp_uS == 0)
            {
                core[core_id].flags.cstate_change = 1;
                core[core_id].cstate_from_pstate_pkt = new_cstate;
                core[core_id].cstate_timestamp_uS = timestamp_us;
                // Update the entry count
                core[core_id].cstate[new_cstate].entry_count++;
                return;
            }
            // normal cstate entry/residency update.
            if (timestamp_us > core[core_id].cstate_timestamp_uS)
            {
                // Update the previous Cstate residency
                uint64_t cstate_time_diff_uS = 0;
                uint8_t cstate_index = core[core_id].cstate_from_pstate_pkt;
                cstate_time_diff_uS = timestamp_us - core[core_id].cstate_timestamp_uS;
                core[core_id].cstate[cstate_index].residency_uS += cstate_time_diff_uS;
            }
            else
            {
                // case 3 : return if timestamp < core[core_id].cstate_timestamp_uS return. eg. Network time sync issue.
                FPFW_ET_LOG(LogCstateValidTimeStamp);
                return;
            }
            // TODO: Implement cstate entry/exit latency calculation.
            // https://azurecsi.visualstudio.com/Dev/_workitems/edit/2492944
            //  Indicate the changes
            core[core_id].flags.cstate_change = 1;
            core[core_id].cstate_from_pstate_pkt = new_cstate;
            core[core_id].cstate_timestamp_uS = timestamp_us;
            // Update the entry count
            core[core_id].cstate[new_cstate].entry_count++;
        }
    }
}

void data_smpl_parse_throttling(pstate_telem_t* pstate_telemetry)
{
    uint8_t core_id = pstate_telemetry->data.core;
    pstate_throttle_status_t status = pstate_telemetry->data.throttle_status;

    // 1. There may be multiple throttling at the same time.e.g Rack throttle  and Temperature throttle.
    // 2. During throttle get the pstate from current pkt, at the time of logging throttling (pstate packet), we have processed
    //    the current telemtry packet already, hence the current pkt, it the most recent current telemetry packet.
    // 3. calculate residency and entry count .
    // 4. max pstate is the reported max pstate during that throttling residency.

    uint64_t time_diff_uS = 0;
    int8_t throttle_index = 0;

    /* throttle_index will always be within limits of 0 ~6 or -1, based on lkp tbl*/
    throttle_index = data_smpl_parse_throttling_get_index_from_status(status);
    if (throttle_index < 0)
    {
        FPFW_ET_LOG(LogCoreThrottleInValidStatus, status);
        return;
    }
    // Check for throttle start/end event.
    if (pstate_telemetry->data.throttle_status != core[core_id].throttling_status)
    {
        // Store the current status for throttling information
        // If there is a throttling end event, process it to calculate the following:
        //    1. Update of the cores throttling residency according to the throttling type
        //    2. Store the latest core max pstate and average pstate during throttling.
        if (status >= CURRENT_THROTTLING_END)
        {
            if (!(core[core_id].throttle_previous_timestamp_uS[throttle_index] != 0 &&
                  pstate_telemetry->timestamp > core[core_id].throttle_previous_timestamp_uS[throttle_index]))
            {
                // return if timestamp < core[core_id].throttle_info[throttle_index].previous_timestamp_uS return. eg. Network time sync issue.
                // Invalid timestamp : not to update the residency with wrong data, but update the exit count and end status.
                core[core_id].throttling_status = pstate_telemetry->data.throttle_status;
                FPFW_ET_LOG(LogCoreThrottleValidTimeStamp, status);
                return;
            }
            // Get the Throttling time stamp now and subtract from previous
            time_diff_uS = (pstate_telemetry->timestamp - core[core_id].throttle_previous_timestamp_uS[throttle_index]);

            // This is the per core and per type throttling residency in uS
            core[core_id].throttle_info[throttle_index].residency_mS += MICROSECONDS_TO_MILLISECONDS(time_diff_uS);
            core[core_id].core_throttling_tracker[throttle_index] = 0;
            // Record the current pstate
            core[core_id].pstate_from_pstate_pkt = pstate_telemetry->data.pstate;
        }
        else
        {
            core[core_id].core_throttling_tracker[throttle_index] = 1;
            core[core_id].throttle_info[throttle_index].entry_count++;
            core[core_id].throttle_previous_timestamp_uS[throttle_index] = pstate_telemetry->timestamp;
        }
    }

    // dummy implementation for rack throttle update.
    data_smpl_parse_rack_throttling(pstate_telemetry, throttle_index, core_id);
}

int8_t data_smpl_parse_throttling_get_index_from_status(pstate_throttle_status_t status)
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

void data_smpl_parse_throttling_exit_transition(uint8_t core_id, uint64_t timestamp_uS)
{
    int i = 0;
    // End all active throttling and update residency
    for (i = 0; i < NUMBER_OF_THROTTLE_TYPES; i++)
    {
        if (core[core_id].core_throttling_tracker[i])
        {
            uint64_t time_diff_uS = timestamp_uS - core[core_id].throttle_previous_timestamp_uS[i];
            core[core_id].throttle_info[i].residency_mS += time_diff_uS / 1000; // Convert to milliseconds
            core[core_id].core_throttling_tracker[i] = 0;                       // Mark as inactive
        }
    }
}

void data_smpl_parse_rack_throttling(pstate_telem_t* pstate_telemetry, int throttle_index, uint8_t core_id)
{
    // TODO:https://azurecsi.visualstudio.com/Dev/_workitems/edit/2561396
    //  Update me as per above task requirement: if throttling is RACK, update throttling_priority_id.
    // check if throttle index is THROTTLE_SOURCE_RACK_LIMIT and if new vm priority is different than
    // previous one update priority id.
    // update  core[core_id].throttling_priority_id from pstate packet with pstate_telemetry->data.vm_throttle_pri
    FPFW_UNUSED(core_id);
    FPFW_UNUSED(pstate_telemetry);
    FPFW_UNUSED(throttle_index);
}

//----------------Power telemetry update manager  ----------------

//----------------Power telemetry resources reset when packages are completed  ----------------
void data_proc_tlm_cmpnt_pwr_pkg_completed(void)
{
    data_smpl_reset_core_data();
    data_smpl_reset_soc_data();
    data_smpl_reset_tile_data();
}

void data_proc_tlm_cmpnt_24hr_pkg_completed(void)
{
    // TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2601165
    //  Implement the reset function for the power telemetry data for 24hr specific record.
}

void data_smpl_reset_core_data()
{
    /* reset global data for each core */
    for (uint8_t core_id = 0; core_id < NUMBER_OF_CORES_PER_DIE; core_id++)
    {
        // create a local copy of each core data and restore selectively
        // TODO: optimize core_runtime_info_t struct : https://azurecsi.visualstudio.com/Dev/_workitems/edit/2602180
        core_runtime_info_t temp_core = core[core_id];

        /* Reset all the core data for core_id*/
        memset(&core[core_id], 0, sizeof(core[core_id]));

        /* Restore data from the local copy. This is data that either does not change or should be kept
        for the next collection window. */

        core[core_id].cstate_timestamp_uS = temp_core.cstate_timestamp_uS;
        core[core_id].pstate_timestamp_uS = temp_core.pstate_timestamp_uS;
        core[core_id].current_pkt_timestamp = temp_core.current_pkt_timestamp;
        core[core_id].flags = temp_core.flags;
        core[core_id].pstate_from_pstate_pkt = temp_core.pstate_from_pstate_pkt;
        core[core_id].cstate_from_pstate_pkt = temp_core.cstate_from_pstate_pkt;
        core[core_id].ldo_voltage = temp_core.ldo_voltage;
        core[core_id].throttling_status = temp_core.throttling_status;
        core[core_id].throttle_event = temp_core.throttle_event;
        core[core_id].throttle_source = temp_core.throttle_source;
        core[core_id].throttling_priority_id = temp_core.throttling_priority_id;
        core[core_id].pstate_from_current_pkt = temp_core.pstate_from_current_pkt;
        core[core_id].average_pwr_mW = temp_core.average_pwr_mW;
        core[core_id].voltage.latest_value_mV = temp_core.voltage.latest_value_mV;
        core[core_id].current.latest_value_mA = temp_core.current.latest_value_mA;
        core[core_id].temperature.latest_value_dC = temp_core.temperature.latest_value_dC;

        for (uint8_t pstate_index = 0; pstate_index < NUMBER_OF_PSTATES; pstate_index++)
        {
            core[core_id].pstate[pstate_index].frequency_Mhz = dvfs_get_freq_from_plimit(pstate_index);
        }
        for (uint8_t throttle_index = 0; throttle_index < NUMBER_OF_THROTTLE_TYPES; throttle_index++)
        {
            core[core_id].throttle_previous_timestamp_uS[throttle_index] =
                temp_core.throttle_previous_timestamp_uS[throttle_index];
            core[core_id].core_throttling_tracker[throttle_index] = temp_core.core_throttling_tracker[throttle_index];
        }
    }
}

void data_smpl_reset_soc_data(void)
{
    soc_runtime_info_t soc_info_temp;
    // TODO: optimize soc_runtime_info_t struct : https://azurecsi.visualstudio.com/Dev/_workitems/edit/2602180
    memcpy(&soc_info_temp, &soc_info, sizeof(soc_runtime_info_t));
    memset(&soc_info, 0, sizeof(soc_runtime_info_t));

    /* Restore selective data from the local copy */
    for (uint8_t rail_id = 0; rail_id < MAX_NUM_OF_VR_RAILS; rail_id++)
    {
        soc_info.rail[rail_id].voltage.latest_value_mV = soc_info_temp.rail[rail_id].voltage.latest_value_mV;
        soc_info.rail[rail_id].temperature.latest_value_dC = soc_info_temp.rail[rail_id].temperature.latest_value_dC;
        soc_info.rail[rail_id].current.latest_value_mA = soc_info_temp.rail[rail_id].current.latest_value_mA;
    }

    for (uint8_t hnf_channel = 0; hnf_channel < NUMBER_OF_HNF_CHANNELS_PER_DIE; hnf_channel++)
    {
        soc_info.hnf[hnf_channel].latest_value_dC = soc_info_temp.hnf[hnf_channel].latest_value_dC;
    }

    for (uint8_t pvt_index = 0; pvt_index < NUMBER_OF_SOC_TEMP_SENSORS; pvt_index++)
    {
        soc_info.sensor_temp[pvt_index].latest_value_dC = soc_info_temp.sensor_temp[pvt_index].latest_value_dC;
    }
}

void data_smpl_reset_tile_data(void)
{
    // Reset data for tiles
    for (uint8_t tile_id = 0; tile_id < NUMBER_OF_TILES_PER_DIE; tile_id++)
    {

        tile_runtime_info_t temp_tile = tile[tile_id];
        memset(&tile[tile_id], 0, sizeof(tile[tile_id]));

        // Restore selective tile data
        tile[tile_id].active_sample_max_temperature_dC = temp_tile.active_sample_max_temperature_dC;
        tile[tile_id].max_tile_temperature_dC = temp_tile.max_tile_temperature_dC;
        tile[tile_id].active_sample_max_id = temp_tile.active_sample_max_id;
        tile[tile_id].max_tile_id = temp_tile.max_tile_id;
    }
}
