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
#define DIMM_MAX_TEMP_MOVING_AVG_NUM_SAMPLES (2)
#define DIMM_TOTAL_PWR_MOVING_AVG_NUM_SAMPLES (2)
#define DIMM_TEMP_MOVING_AVG_NUM_SAMPLES (3)
#define DIMM_PWR_MOVING_AVG_NUM_SAMPLES (3)
#define VR_TEMP_MOVING_AVG_NUM_SAMPLES (10)

#define INVALID_PSTATE (0xFF)

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

typedef struct {
    uint32_t entry_count;
    uint32_t residency_uS;
    uint16_t overrun_count;
    mma_u16_t pstate;
} throttle_metrics_t;

typedef struct {
    uint32_t entry_count;
    uint32_t residency_uS;
} rack_priority_metrics_t;

typedef struct {
    uint64_t timestamp_uS;
    uint32_t unaged_counter;
    uint32_t aged_counter;
    uint16_t voltage_mV;
    uint16_t temperature_dC;
    uint8_t counter_id;
} aging_counter_metrics_t;

typedef struct
{
    uint64_t droop_count;
    pstate_metrics_t pstate[NUMBER_OF_PSTATES];
    cstate_metrics_t cstate[NUMBER_OF_CSTATES];
    throttle_metrics_t throttle_info[NUMBER_OF_THROTTLE_SOURCES];
    rack_priority_metrics_t rack_priorities[NUMBER_OF_RACK_THROTTLE_PRIORITIES];
    mma_u32_t temperature_dC; //number of samples exceeds 16 bits
    mma_u16_t power_mW;
    mma_u16_t voltage_mV;
    mma_u16_t current_mA;
    mma_u16_t vcpu_input_voltage_mV;
} computed_per_core_metrics_t;

typedef struct 
{
    aging_counter_metrics_t core_aging_counters[NUMBER_OF_AGING_COUNTER_PAIRS];
} computed_per_core_24_hrs_metrics_t;

typedef struct
{
    mma_u16_t temperature_s0_dC;
    mma_u16_t temperature_s1_dC;
    mma_u16_t power_mW;
} computed_per_dimm_metrics_t;

typedef struct
{
    mma_u32_t core_power;
} computed_per_mpam_metrics_t;

typedef struct
{
    uint16_t stub_metric;
} computed_per_tile_metrics_t;

typedef struct
{
    mma_u32_t current_mA;
    mma_u16_t voltage_mV;
    mma_u16_t temperature_dC;
} computed_per_rail_metrics_t;

typedef struct
{
    computed_per_rail_metrics_t vr_rail[MAX_NUM_OF_VR_RAILS];
    computed_per_dimm_metrics_t dimm[NUMBER_OF_DIMMS_PER_DIE];
    mma_u16_t top_sensor_temp_dC[NUMBER_OF_SOC_TEMP_SENSORS];
    mma_u16_t hnf_temperature_dC[NUMBER_OF_HNF_CHANNELS_PER_DIE];
} computed_per_soc_metrics_t;

typedef struct {
    computed_per_core_metrics_t cores[NUMBER_OF_CORES_PER_DIE];
    computed_per_soc_metrics_t soc;
} computed_metrics_2_min_t;

typedef struct
{
    computed_per_core_24_hrs_metrics_t cores[NUMBER_OF_CORES_PER_DIE];
} computed_metrics_24_hrs_t;

typedef struct
{
    computed_per_mpam_metrics_t mpam[NUMBER_OF_MPAMS];
    mma_u16_t max_soc_temp_dC;
} computed_metrics_d2d_2_min_t;

typedef struct
{
    uint16_t max_soc_temp_samples_dC[TEMPERATURE_MOVING_AVG_NUM_SAMPLES];
    moving_avg_u16_t max_soc_temp_mov_avg_dC;

    uint32_t soc_total_pwr_samples_mW[TEMPERATURE_MOVING_AVG_NUM_SAMPLES];
    moving_avg_u32_t soc_total_pwr_mov_avg_mW;

    uint16_t max_dimm_temp_samples_dC[DIMM_MAX_TEMP_MOVING_AVG_NUM_SAMPLES];
    moving_avg_u16_t max_dimm_temp_mov_avg_dC;

    uint32_t dimm_total_pwr_samples_mW[DIMM_TOTAL_PWR_MOVING_AVG_NUM_SAMPLES];
    moving_avg_u32_t dimm_total_pwr_mov_avg_mW;

    uint16_t max_vr_temp_samples_dC[VR_TEMP_MOVING_AVG_NUM_SAMPLES];
    moving_avg_u16_t max_vr_temp_mov_avg_dC;

    uint16_t pstate_samples[PSTATE_MOVING_AVG_NUM_SAMPLES];
    moving_avg_u16_t pstate_mov_avg;

    uint16_t dimm_chan_temp_samples[NUMBER_OF_DIMMS_PER_DIE][DIMM_TEMP_MOVING_AVG_NUM_SAMPLES];
    moving_avg_u16_t dimm_chan_temp_mov_avg[NUMBER_OF_DIMMS_PER_DIE];

    uint16_t dimm_chan_pwr_samples[NUMBER_OF_DIMMS_PER_DIE][DIMM_PWR_MOVING_AVG_NUM_SAMPLES];
    moving_avg_u16_t dimm_chan_pwr_mov_avg[NUMBER_OF_DIMMS_PER_DIE];

    uint16_t dimm_chan_max_temp_dC[NUMBER_OF_DIMMS_PER_DIE];

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

extern bool in_band_publishing_active;

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
 * @brief Helper function to determine if a core is active and in-band publishing is active
 *
 * @return true if both conditions are met, false otherwise
 */
bool comp_metrics_core_and_inband_publishing_active(uint8_t core_id);

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
 * @param[in] latest_vcpu_voltage_mV latest value of the vcpu voltage applied to this core's LDO from telmetry packet.
 *
 */
void comp_metrics_for_single_core_voltage(uint8_t core_id, uint16_t latest_value_mV, uint16_t latest_vcpu_voltage_mV);

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
                                uint32_t (*latest_rail_current_mA)[MAX_NUM_OF_VR_RAILS]);

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
 * SOC DIMM (Dual In-line Memory Module)
 * Updates DIMM Temperature and Power Consumption Metrics
 *
 * @param[in] dimm_idx - dimm_idx from SCF RAM
 * @param[in] latest_dimm_temp_s0_dC - latest temp for S0 in dC
 * @param[in] latest_dimm_temp_s1_dC - latest temp for S1 in dC
 * @param[in] latest_dimm_power_mW - latest dimm power in mW
 */
void comp_metrics_for_single_soc_dimm(uint8_t dimm_idx, uint16_t latest_dimm_temp_s0_dC, uint16_t latest_dimm_temp_s1_dC, uint16_t latest_dimm_power_mW);

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
 * @brief  Calculate pstate mma during throttling for a single core.
 *
 * @param[in] core_id
 * @param[in] throttle_source  Throttling type.
 * @param[in] latest_pstate - latest_pstate
 */
void comp_metrics_for_single_core_throttling_pstate(uint8_t core_id, uint8_t throttle_source, uint8_t latest_pstate);

/**
 * @brief This API used during cores's throttling for  overrun counter update.
 * @param[in] core_id
 * @param[in] throttle_source  for which type of throttle counter need to be updated.
 */
void comp_metrics_for_single_core_throttle_overrun(uint8_t core_id, uint8_t throttle_source);

/**
 * @brief  This API used during cores's throttling for residency update and entry count update.
 *
 * @param[in] core_id
 * @param[in] throttle_source  Throttle source that is active before the new source is set.
 * @param[in] time_diff_uS    time diff between current timestamp and previous time stamp
 * @param[in] throttle_start - New Throttle source started
 */
void comp_metrics_for_single_core_single_throttle_source(uint8_t core_id, uint8_t throttle_source, uint64_t timestamp_diff_uS, bool throttle_start);

/**
 * @brief  This API used during cores's rack throttling for residency update and entry count update.
 *
 * @param[in] core_id
 * @param[in] current_priority  Rack priority that is active before the new priority is set.
 * @param[in] time_diff_uS  time diff between current timestamp and previous time stamp
 * @param[in] new_priority  New rack priority that is set.
 * @param[in] rack_throttle_priority_start - a new rack throttling priority started..
 */
void comp_metrics_for_single_core_single_rack_priority(uint8_t core_id, uint8_t current_priority, uint64_t timestamp_diff_uS,  uint8_t new_priority, bool rack_throttle_priority_start);

/**
 * @brief helper function to update the pstate runtime timestamp
 *
 * @param[in] core_id  core that is referenced to that owns this timestamp
 * @param[in] current_pstate  current pstate of the core
 * @param[in] timestamp_diff_uS diff of timestamp from previous and new
 * @param[in] new_pstate  only valid when update_pstate_entry is true
 * @param[in] update_pstate_entry new entry in this state
 *
 * @return none
 */
void comp_metrics_for_single_core_single_pstate(uint8_t core_id, uint8_t current_pstate, uint64_t timestamp_diff_uS, uint8_t new_pstate, bool update_pstate_entry);

/**
 * @brief helper function to update the average pstate for the soc, used by out of band telemetry
 *
 * @param[in] pstate  Array of latest core pstates. Invalid pstates are set to INVALID_PSTATE.
 *
 * @return none
 */
void comp_metrics_for_soc_avg_pstate(uint8_t (*pstate)[NUMBER_OF_CORES_PER_DIE]);

/**
 * @brief helper function to update the cstate compute metrics
 * @param[in] core_id  core that is referenced to that owns this timestamp
 * @param[in] current_cstate  current cstate of the core
 * @param[in] timestamp_diff_uS diff of timestamp from previous packet
 * @param[in] new_cstate  only valid when update_cstate_entry is true
 * @param[in] update_cstate_entry new entry in this state
 */
void comp_metrics_for_single_core_single_cstate(uint8_t core_id, uint8_t current_cstate, uint64_t timestamp_diff_uS, uint8_t new_cstate, bool update_cstate_entry);

/**
 * @brief function is intended to update the core power state (PState) based on the provided core ID and PState index.
 *
 * @param[in] core_id  The identifier of the core for which the PState is being updated.
 * @param[in] current_pstate  current pstate of the core
 * @param[in] latest_power_mW  The latest power value in mW
 */
void comp_metrics_for_single_core_power_per_pstate(uint8_t core_id, uint8_t current_pstate,  uint16_t latest_power_mW);

/**
 * @brief Resets all local die metrics that get reported every two minutes.
 *
 * @return none
 */
void comp_metrics_reset_local_2_min_metrics();

/**
 * @brief Resets all metrics that get reported every two minutes for die-to-die exchange.
 *
 * @return none
 */
void comp_metrics_reset_d2d_2_min_metrics();

/**
 * @brief Resets all metrics that get reported every 24 hours.
 *
 * @return none
 */
void comp_metrics_reset_24_hrs_metrics();

/**
 * @brief read droop counts and update compute metrics reported every 2 mins.
 *
 * @return none
 */
void comp_metrics_for_cores_droop_counts(void);

/**
 * @brief function update aging counters metrics for a specified core.
 *
 * @param[in] core_id - The identifier of the core for which the MPAM residency is being updated.
 * @param[in] latest_voltage_mV - latest core voltage 
 * @param[in] latest_max_value_dC -latest max temperature for the core
 * @param[in] this_pwr_pkg_timestamp_uS - timestamp when this package is getting created.
 * @param[in] latest_aged_counter latest aged counter value  for the core_id 
 * @param[in] latest_unaged_counter latest aged counter for the core_id 
 * @param[in] counter_id  the counter id for which we are reading value .
 */
void comp_metrics_for_single_core_aging_counters( uint8_t core_id, uint16_t latest_voltage_mV, uint16_t latest_max_value_dC, uint64_t this_pwr_pkg_timestamp_uS, uint32_t latest_aged_counter, uint32_t latest_unaged_counter, uint8_t counter_id);

/**
 * @brief Update MPAM power metrics for all MPAMs based on the provided power values.
 *
 * @param[in] mpam_power_mW  Array of latest MPAM power values in mW.
 */
void comp_metrics_for_mpam_power( uint32_t (*mpam_power_mW)[NUMBER_OF_MPAMS]);
