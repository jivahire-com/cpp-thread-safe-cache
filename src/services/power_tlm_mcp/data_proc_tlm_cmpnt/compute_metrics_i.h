//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file compute_metrics_i.h
 * Update metrics for telemetry data
 */

#pragma once

/*----------- Nested includes ------------*/

#include "data_sampling_i.h" // internal APIs
#include "data_utilities_i.h"

#include <exec_tlm_cmpnt.h>
#include <fpfw_status.h>
#include <stdint.h>
#include <telemetry_package_defs.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MOVING_AVG_DURATION_MS (250)
#define TEMPERATURE_MOVING_AVG_NUM_SAMPLES (MOVING_AVG_DURATION_MS / DATA_AGGR_PKG_PERIOD_MS)
#define PSTATE_MOVING_AVG_NUM_SAMPLES (MOVING_AVG_DURATION_MS / DATA_AGGR_PKG_PERIOD_MS)
#define DIMM_MOVING_AVG_NUM_SAMPLES (2)
#define VR_TEMP_MOVING_AVG_NUM_SAMPLES (10)

/*-------------- Typedefs ----------------*/

//
// TODO: Some of these structs currently have stub metrics (to get the organization in place),
//       these will be replaced with actual metrics as each computed metric is added to
//       this design.
//

typedef struct
{
    uint32_t  residency_uS;
    uint32_t  entry_count;
    mma_u16_t  power_mW;//This is per pstate power metrics within a core.
} pstate_metrics_t;

typedef struct
{
    uint32_t  residency_uS;
    uint32_t  entry_count;
} cstate_metrics_t;

typedef struct
{
    pstate_metrics_t pstate[NUMBER_OF_PSTATES];
    cstate_metrics_t cstate[NUMBER_OF_CSTATES];
    pwr_core_element_throttle_t throttle_info[NUMBER_OF_THROTTLE_TYPES];
    pwr_core_element_rack_priorities_t rack_priorities[NUMBER_OF_RACK_PRIORITIES];
    mma_u16_t power_mW;
    mma_u16_t temperature_dC;
    mma_u16_t voltage_mV;
    mma_u16_t current_mA;
} computed_per_core_metrics_t;

typedef struct
{
    mma_u16_t temperature_s0_dC;
    mma_u16_t temperature_s1_dC;
    mma_u16_t power_mW;
} computed_per_dimm_metrics_t;

typedef struct
{
    uint16_t stub_metric;
} computed_per_mpam_metrics_t;

typedef struct
{
    uint16_t stub_metric;
} computed_per_tile_metrics_t;

typedef struct
{
    mma_u16_t voltage_mV;
    mma_u16_t current_mA;
    mma_u16_t temperature_dC;
} computed_per_rail_metrics_t;

typedef struct
{
    computed_per_rail_metrics_t vr_rail[MAX_NUM_OF_VR_RAILS];
    computed_per_dimm_metrics_t dimm[NUMBER_OF_DIMM_MODULES_PER_DIE];
    mma_u16_t top_sensor_temp_dC[NUMBER_OF_SOC_TEMP_SENSORS];
    mma_u16_t hnf_temperature_dC[NUMBER_OF_HNF_CHANNELS_PER_DIE];
} computed_per_soc_metrics_t;

typedef struct {
    computed_per_core_metrics_t cores[NUMBER_OF_CORES_PER_DIE];
    computed_per_mpam_metrics_t mpams[NUMBER_OF_CORES_PER_DIE];
    computed_per_tile_metrics_t tiles[NUMBER_OF_TILES_PER_DIE];
    computed_per_soc_metrics_t soc;
} computed_metrics_2_min_t;

typedef struct
{
    uint16_t stub_metric;
} computed_metrics_24_hrs_t;

typedef struct
{
    mma_u16_t max_soc_temp_dC;
} computed_metrics_d2d_2_min_t;

typedef struct
{
    uint16_t max_soc_temp_samples_dC[TEMPERATURE_MOVING_AVG_NUM_SAMPLES];
    moving_avg_u16_t max_soc_temp_mov_avg_dC;

    uint32_t soc_total_pwr_samples_mW[TEMPERATURE_MOVING_AVG_NUM_SAMPLES];
    moving_avg_u32_t soc_total_pwr_mov_avg_mW;

    uint16_t max_dimm_temp_samples_dC[DIMM_MOVING_AVG_NUM_SAMPLES];
    moving_avg_u16_t max_dimm_temp_mov_avg_dC;

    uint32_t dimm_total_pwr_samples_mW[DIMM_MOVING_AVG_NUM_SAMPLES];
    moving_avg_u32_t dimm_total_pwr_mov_avg_mW;

    uint16_t max_vr_temp_samples_dC[VR_TEMP_MOVING_AVG_NUM_SAMPLES];
    moving_avg_u16_t max_vr_temp_mov_avg_dC;

    uint16_t pstate_samples[PSTATE_MOVING_AVG_NUM_SAMPLES];
    moving_avg_u16_t pstate_mov_avg;

} computed_metrics_oob_t;


/*-- Declarations (Statics and globals) --*/

extern computed_metrics_2_min_t computed_metrics_2_mins;
extern computed_metrics_24_hrs_t computed_metrics_24_hrs;

extern computed_metrics_oob_t computed_metrics_oob;
// these metrics are used for die-to-die exchange but are cleared differently depending on the die
// The primary die may clear with the computed_metrics_2_min_t values, but secondary die will clear after
// copying to the die to die exchange
extern computed_metrics_d2d_2_min_t computed_metrics_d2d_2mins;

extern bool core_is_active[NUMBER_OF_CORES_PER_DIE];

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize the compute metrics module
 * @param   None
 * @return  None
 */
void comp_metrics_init(void);

/**
 * @brief Initialize the active cores based on the system configuration
 * @param   None
 * @return  None
 */
void comp_metrics_init_active_cores(void);

/**
 * @brief function updates various metrics for all cores, including PState residency and power,
 * CState residency, throttling status, core current, voltage, and temperature. It calculates the
 * time difference since the last update and uses this information to update the residency and metrics for each core.
 * @param   None
 * @return  None
 */
void comp_metrics_for_cores_for_sampling_period(void);

/**
 * @brief  function updates the minimum, maximum, and average current values for a specified core based on the provided
 * time difference and residency time
 *
 * @param[in] core_id  core for which the current values are being updated.
 * @param[in] latest_value_mA  Latest value from sensor fifo.
 */
void comp_metrics_for_single_core_current(uint8_t core_id, uint16_t latest_value_mA);

/**
 * @brief    function updates the minimum, maximum, and average voltage values for a specified core based
 * on the provided time difference and residency time
 *
 * @param[in] core_id  core for which the current values are being updated.
 * @param[in] latest_value_mV latest value of the core voltage from telmetry packet.
 *
 */
void comp_metrics_for_single_core_voltage(uint8_t core_id, uint16_t latest_value_mV);

/**
 * @brief    function updates the minimum, maximum, and average temperature values for
 * a specified core
 *
 * @param[in] core_id  core for which the current values are being updated.
 * @param[in] latest_temperature_dC  Most recent measured temeprature
 */
void comp_metrics_for_single_core_temperature(uint8_t core_id, uint16_t latest_temperature_dC);

/**
 * @brief function updates the minimum, maximum, and average power values for a specified core based on the provided
 *
 * @param[in] core_id  core for which the power values are being updated.
 * @param[in] latest_power_mW  The latest power value in mW
 *
 * @return None
 */
void comp_metrics_for_single_core_power(uint8_t core_id, uint16_t latest_power_mW);

/**
 * @brief function is intended to update the histogram data for a specified core
 *
 * @param[in] core_id
 */
void comp_metrics_for_single_core_histogram(uint8_t core_id);

/**
 * @brief function is intended to update the core power state (PState) based on the provided core ID and PState index.
 *
 * @param[in] core_id - The identifier of the core for which the PState is being updated.
 * @param[in] pstate_index - The index of the PState to be updated.
 */
void comp_metrics_for_single_core_pstate_power(uint8_t core_id, uint8_t pstate_index);

/**
 * @brief  for each rail, updates the minimum, maximum, and average current values
 * updates the minimum, maximum, and average voltage values
 * updates the total power of all of the rails
 *
 * @param[in] latest_voltage_mV  Array of latest vr rail voltages
 * @param[in] latest_current_mA Array of latest vr rail currents
 */
void comp_metrics_for_soc_rails(uint16_t (*latest_rail_voltage_mV)[MAX_NUM_OF_VR_RAILS],
                                uint16_t (*latest_rail_current_mA)[MAX_NUM_OF_VR_RAILS]);

/**
 * @brief  function updates the minimum, maximum, and average temperature values for
 * a specified SOC VR (Voltage Regulator) rail based on the provided time difference and residency time
 *
 * @param[in] latest_rail_temperature_dC Array of latest vr rail temperatures.
 */
void comp_metrics_for_soc_rail_temperature(uint16_t (*latest_rail_temperature_dC)[MAX_NUM_OF_VR_RAILS]);

/**
 * @brief function updates the minimum, maximum, and average temperature values for
 * a specified SOC HNF channel
 *
 * @param[in] hnf_channel The identifier of the HNF for which the temperature values are being updated.
 * @param[in] latest_temperature_dC  Latest temperature for hnf channel.
 */
void comp_metrics_for_single_hnf_channel(uint8_t hnf_channel, uint16_t latest_temperature_dC);

/**
 * @brief function updates the minimum, maximum, and average temperature values for soc temp sensors
 * @param[in] latest_soc_top_temp_dC  Array of latest soc top temperatures.
 */
void comp_metrics_for_soc_top_temp_sensor(uint16_t (*latest_soc_top_temp_dC)[NUMBER_OF_SOC_TEMP_SENSORS]);

/**
 * @brief function updates the maximum SOC temperature value based on the latest maximum SOC temperature
 * in degrees Celsius (dC).
 *
 * @param[in] latest_max_soc_temp_dC - The latest maximum SOC temperature in dC.
 */
void comp_metrics_for_soc_max_temp(uint16_t latest_max_soc_temp_dC);

/**
 * @brief  function updates the minimum, maximum, and average temperature values for a specified
 * SOC DIMM (Dual In-line Memory Module) based on the provided time difference and residency time.
 * It updates the temperature data for both S0 and S1 sensors of the DIMM module. It utilizes
 * the data_util_calc_mma_res function to perform the calculations.
 *
 * @param[in] dimm_id - dimm_id from SCF RAM
 * @param[in] latest_dimm_temp_s0_dC - latest temp for S0 in dC
 * @param[in] latest_dimm_temp_s1_dC - latest temp for S1 in dC
 */
void comp_metrics_for_single_soc_dimm_temp(uint8_t dimm_id, uint16_t latest_dimm_temp_s0_dC, uint16_t latest_dimm_temp_s1_dC );

/**
 * @brief  function updates the minimum, maximum, and average power values for a specified
 *         DIMM module.
 * @param[in] dimm_id - dimm_id from SCF RAM
 * @param[in] latest_dimm_power_mW - latest dimm power in mW
 */
void comp_metrics_for_single_soc_dimm_power(uint8_t dimm_id, uint16_t latest_dimm_power_mW);

/**
 * @brief  This API used to update the maximum DIMM temperature in dC.
 *
 * @param[in] latest_max_dimm_temp_dC - latest maximum DIMM temperature in dC.
 */
void comp_metrics_for_max_dimm_temp(uint16_t latest_max_dimm_temp_dC);

/**
 * @brief  This API used to update the maximum DIMM power in mW.
 *
 * @param[in] dimm_total_pwr_mW - latest total DIMM power in mW.
 */
void comp_metrics_for_total_dimm_pwr(uint32_t dimm_total_pwr_mW);

/**
 * @brief  This API used during cores are throttling, MMA calculation is triggerd by this APIs
 *
 * @param[in] core_id
 * @param[in] throttle_index  Throttling type.
 * @param[in] time_diff_uS    time diff between current timestamp and previous time stamp
 * @param[in] latest_pstate - latest_pstate
 */
void comp_metrics_for_single_core_throttling_pstate(uint8_t core_id, int8_t throttle_index, uint32_t time_diff_uS, uint8_t latest_pstate);

/**
 * @brief This API used during cores's throttling for  overrun counter update.
 * @param[in] core_id
 * @param[in] throttle_index  for which type of throttle counter need to be updated.
 */
void comp_metrics_for_single_core_single_throttle_overrun_count_update(uint8_t core_id, uint8_t throttle_index);

/**
 * @brief  This API used during cores's throttling for residency update and entry count update.
 *
 * @param[in] core_id
 * @param[in] throttle_index  Throttling type.
 * @param[in] time_diff_uS    time diff between current timestamp and previous time stamp
 * @param[in] throttle_start - a new throttling started.
 */
void comp_metrics_for_single_core_single_throttle_update(uint8_t core_id, uint8_t throttle_index, uint64_t timestamp_diff_uS, bool throttle_start);

/**
 * @brief  This API used during cores's rack throttling for residency update and entry count update.
 *
 * @param[in] core_id
 * @param[in] priority_id  Rack priority id
 * @param[in] time_diff_uS    time diff between current timestamp and previous time stamp
 * @param[in] rack_throttle_priority_start - a new rack throttling priority started..
 */
void comp_metrics_for_single_core_single_rack_throttle_update(uint8_t core_id, uint8_t priority_id, uint64_t timestamp_diff_uS, bool rack_throttle_priority_start);

/**
 * @brief helper function to update the pstate runtime timestamp
 *
 * @param[in] core_id - core that is referenced to that owns this timestamp
 * @param[in] pstate - pstate that is reference to where it needs to be updated
 * @param[in] timestamp_diff_uS diff of timestamp from previous and new .
 * @param[in] update_pstate_entry new entry in this state  .
 *
 * @return none
 */
void comp_metrics_for_single_core_single_pstate(uint8_t core_id, uint8_t pstate, uint64_t timestamp_diff_uS, uint8_t update_pstate_entry);


/**
 * @brief helper function to update the average pstate for the soc, used by out of band telemetry
 *
 * @param[in] pstate  Array of latest core pstates.
 *
 * @return none
 */
void comp_metrics_for_soc_avg_pstate(uint8_t (*pstate)[NUMBER_OF_CORES_PER_DIE]);

/**
 * @brief helper function to update the cstate compute metrics
 * @param[in] core_id  core that is referenced to that owns this timestamp
 * @param[in] cstate  -cstate that is reference to where it needs to be updated
 * @param[in] timestamp_diff_uS diff of timestamp from previous and new .
 * @param[in] update_cstate_entry new entry in this state  .
 */
void comp_metrics_for_single_core_single_cstate(uint8_t core_id, uint8_t cstate, uint64_t timestamp_diff_uS, uint8_t update_cstate_entry);

/**
 * @brief function is intended to update the core power state (PState) based on the provided core ID and PState index.
 *
 * @param[in] core_id - The identifier of the core for which the PState is being updated.
 * @param[in] pstate_index - The index of the PState to be updated.
 * @param[in] latest_power_mW
 */
void comp_metrics_for_single_core_power_per_pstate(uint8_t core_id, uint8_t pstate_index,  uint16_t latest_power_mW);

/**
 * @brief function is intended to update the MPAM residency for a specified core and MPAM ID.
 *
 * @param[in] core_id - The identifier of the core for which the MPAM residency is being updated.
 * @param[in] mpam_id - The identifier of the MPAM for which the residency is being updated.
 * @param[in] pstate - The PState associated with the MPAM residency update.
 */
void comp_metrics_for_mpam(uint8_t core_id, uint16_t mpam_id, uint8_t pstate);

/**
 * @brief Resets all metrics that get reported every two minutes.
 *
 * @return none
 */
void comp_metrics_reset_2_mins_metrics();

/**
 * @brief Resets all metrics that get reported every 24 hours.
 *
 * @return none
 */
void comp_metrics_reset_24_hrs_metrics();
