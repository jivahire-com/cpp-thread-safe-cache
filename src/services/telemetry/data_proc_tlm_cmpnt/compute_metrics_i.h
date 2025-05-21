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

#include <fpfw_status.h>
#include <stdint.h>
#include <telemetry_package_defs.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

//
// TODO: Some of these structs currently have stub metrics (to get the organization in place),
//       these will be replaced with actual metrics as each computed metric is added to
//       this design.
//

typedef struct
{
    mma_u16_t power_mW;
    mma_u16_t voltage_mV;
} computed_per_core_metrics_t;

typedef struct
{
    mma_u16_t temperature_s0_dC;
    mma_u16_t temperature_s1_dC;
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
    computed_per_dimm_metrics_t dimm[NUMBER_OF_DIMM_MODULES];
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

/*-- Declarations (Statics and globals) --*/

extern computed_metrics_2_min_t computed_metrics_2_mins;
extern computed_metrics_24_hrs_t computed_metrics_24_hrs;

/*--------- Function Prototypes ----------*/

/**
 * @brief Comput metrics every sample period
 * @param   None
 * @return  None
 */
void comp_metrics_for_sample_period(void);

/**
 * @brief function updates various metrics for all cores, including PState residency and power,
 * CState residency, throttling status, core current, voltage, and temperature. It calculates the
 * time difference since the last update and uses this information to update the residency and metrics for each core.
 * @param   None
 * @return  None
 */
void comp_metrics_for_cores_for_sampling_period(void);

/**
 * @brief function updates various metrics for all tiles, including the maximum tile temperature,
 * vCPU voltage, and system voltage (Vsys). It calculates the time difference since the last update
 * and uses this information to update the residency and metrics for each tile.
 * @param   None
 * @return  None
 */
void comp_metrics_for_tiles_for_sampling_period(void);

/**
 * @brief The comp_metrics_for_soc_for_sampling_period function updates various SOC (System on Chip) components,
 * including VR (Voltage Regulator) rails, HNF temperatures, PVT (Process, Voltage, Temperature) sensors
 * and DIMM (Dual In-line Memory Module) temperatures. It calculates the time difference
 * since the last update and uses this information to update the residency and metrics for each component.
 * @param   None
 * @return  None
 */
void comp_metrics_for_soc_for_sampling_period(void);

/**
 * @brief The comp_metrics_for_single_core_pstate function updates the power state (PState) residency and power metrics
 * for a specified core based on the provided timestamp. It handles both throttling and
 * non-throttling scenarios and updates the minimum, maximum, and average power values
 * for the current PState.
 *
 * @param[in] core_id -The identifier of the core for which the PState is being updated.
 * @param[in] time_stamp_uS -The current timestamp in microseconds.
 */
void comp_metrics_for_single_core_pstate(uint8_t core_id, uint64_t time_stamp_uS);

/**
 * @brief comp_metrics_for_single_core_throttling from runtime update manager.
 *
 * @param[in] core_id
 * @param[in] time_stamp_uS -system time stamp
 */
void comp_metrics_for_single_core_throttling(uint8_t core_id, uint64_t time_stamp_uS);

/**
 * @brief  function updates the minimum, maximum, and average current values for a specified core based on the provided
 * time difference and residency time
 *
 * @param[in] core_id  core for which the current values are being updated.
 * @param[in] time_diff_uS  time_diff_uS: The time difference in microseconds between the current and previous measurements.
 * @param[in] residency_uS  The total residency time in microseconds over which the average is calculated.
 */
void comp_metrics_for_single_core_current(uint8_t core_id, uint32_t time_diff_uS, uint32_t residency_uS);

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
 * a specified core based on the provided time difference and residency time
 *
 * @param[in] core_id  core for which the current values are being updated.
 * @param[in] time_diff_uS  time_diff_uS: The time difference in microseconds between the current and previous measurements.
 * @param[in] residency_uS  The total residency time in microseconds over which the average is calculated.
 */
void comp_metrics_for_single_core_temperature(uint8_t core_id, uint32_t time_diff_uS, uint32_t residency_uS);

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
 * @brief    function updates the minimum, maximum, and average voltage values for the
 * vCPU of a specified tile based on the provided time difference and residency time
 *
 * @param[in] core_id  core for which the current values are being updated.
 * @param[in] time_diff_uS  time_diff_uS: The time difference in microseconds between the current and previous measurements.
 * @param[in] residency_uS  The total residency time in microseconds over which the average is calculated.
 */
void comp_metrics_for_single_tile_vcpu(uint8_t tile_id, uint32_t time_diff_uS, uint32_t residency_uS);

/**
 * @brief    function updates the minimum, maximum, and average voltage values for the
 * vSYS of a specified tile based on the provided time difference and residency time
 *
 * @param[in] core_id  core for which the current values are being updated.
 * @param[in] time_diff_uS  time_diff_uS: The time difference in microseconds between the current and previous measurements.
 * @param[in] residency_uS  The total residency time in microseconds over which the average is calculated.
 */
void comp_metrics_for_single_tile_vsys(uint8_t tile_id, uint32_t time_diff_uS, uint32_t residency_uS);

/**
 * @brief function updates the minimum, maximum, and average voltage values for
 * a specified SOC VR (Voltage Regulator) rail based on the provided time difference and residency time
 *
 * @param[in] vr_index The identifier of the VR rail for which the voltage values are being updated.
 * @param[in] time_diff_uS
 * @param[in] residency_uS
 */
void comp_metrics_for_soc_rail_voltage(uint8_t vr_index, uint32_t time_diff_uS, uint32_t residency_uS);

/**
 * @brief  function updates the minimum, maximum, and average current values for
 * a specified SOC VR (Voltage Regulator) rail based on the provided time difference and residency time
 *
 * @param[in] vr_index
 * @param[in] time_diff_uS
 * @param[in] residency_uS
 */
void comp_metrics_for_soc_rail_current(uint8_t vr_index, uint32_t time_diff_uS, uint32_t residency_uS);

/**
 * @brief  function updates the minimum, maximum, and average temperature values for
 * a specified SOC VR (Voltage Regulator) rail based on the provided time difference and residency time
 *
 * @param[in] vr_index - The identifier of the VR rail for which the voltage values are being updated.
 * @param[in] time_diff_uS
 * @param[in] residency_uS
 */
void comp_metrics_for_soc_rail_temperature(uint8_t vr_index, uint32_t time_diff_uS, uint32_t residency_uS);

/**
 * @brief function updates the minimum, maximum, and average temperature values for
 * a specified SOC HNF  based on the provided time difference and residency time
 *
 * @param[in] hnf_index The identifier of the HNF for which the temperature values are being updated.
 * @param[in] time_diff_uS  The time difference in microseconds between the current and previous measurements
 * @param[in] residency_uS  The total residency time in microseconds over which the average is calculated.
 */
void comp_metrics_for_single_hnf_channel(uint8_t hnf_index, uint32_t time_diff_uS, uint32_t residency_uS);

/**
 * @brief function updates the minimum, maximum, and average temperature values for
 * a specified SOC PVT  sensor based on
 * the provided time difference and residency time.
 * @param[in] pvt_index  The identifier of the PVT sensor for which the temperature values are being updated.
 * @param[in] time_diff_uS The time difference in microseconds between the current and previous measurements
 * @param[in] residency_uS The total residency time in microseconds over which the average is calculated.
 */
void comp_metrics_for_single_soc_temp_sensor(uint8_t pvt_index, uint32_t time_diff_uS, uint32_t residency_uS);

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
 * @brief  This API used during cores are throttling, MMA calculation is triggerd by this APIs
 *
 * @param[in] core_id
 * @param[in] throttle_index  Throttling type.
 * @param[in] time_diff_uS    time diff between current timestamp and previous time stamp
 * @param[in] residency_mS  - residency in mS
 */
void comp_metrics_for_single_core_throttling_pstate(uint8_t core_id, int8_t throttle_index, uint32_t time_diff_uS, uint32_t residency_mS);

/**
 * @brief function is intended to update the histogram data for a specified core
 *
 * @param[in] core_id
 */
void comp_metrics_for_single_core_histogram(uint8_t core_id);

/**
 * @brief helper function to update the pstate runtime timestamp
 *
 * @param[in] core_id - core that is referenced to that owns this timestamp
 * @param[in] pstate - pstate that is reference to where it needs to be updated
 * @param[in] timestamp - timestamp used for the update
 *
 * @return fpfw_status_t
 */
fpfw_status_t comp_metrics_for_single_core_single_pstate(uint8_t core_id, uint8_t pstate, uint64_t timestamp);

/**
 * @brief function is intended to update the core power state (PState) based on the provided core ID and PState index.
 *
 * @param[in] core_id - The identifier of the core for which the PState is being updated.
 * @param[in] pstate_index - The index of the PState to be updated.
 */
void comp_metrics_for_single_core_pstate_power(uint8_t core_id, uint8_t pstate_index);

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
