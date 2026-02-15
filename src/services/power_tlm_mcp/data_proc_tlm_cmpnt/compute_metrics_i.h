//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file compute_metrics_i.h
 * Update metrics for telemetry data
 */

#pragma once

/*----------- Nested includes ------------*/

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
    uint64_t m1_entry_count;
    uint64_t m2_entry_count;
    uint64_t m0_residency_count;
    uint64_t m1_residency_count;
    uint64_t m2_residency_count;
    uint64_t delivered_perf_count;
} die_mesh_pwr_metrics_t;

typedef struct {
    uint64_t timestamp_uS;
    uint32_t unaged_counter;
    uint32_t aged_counter;
    uint16_t voltage_mV;
    uint16_t temperature_dC;
    uint8_t counter_id;
} aging_counter_metrics_t;

typedef struct {
    uint64_t tx_residency_count;
    uint64_t rx_residency_count;
    uint64_t  bw_tx_flit_count ;
    uint64_t bw_rx_flit_count;;
    uint8_t  link_id;
} d2d_link_metrics_t;

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

typedef struct{
    uint32_t bin_count[NUMBER_OF_HS_VOLTAGE_SCALES][NUMBER_OF_HS_TEMP_SCALES];
} histogram_element_t;

typedef struct
{
    histogram_element_t histogram;
    aging_counter_metrics_t core_aging_counters[NUMBER_OF_AGING_COUNTER_PAIRS];
} computed_per_core_24_hrs_metrics_t;

typedef struct
{
    uint32_t pc3_residency_mS;  // Accumulated PC3 residency in milliseconds
    uint32_t pc4_residency_mS;  // Accumulated PC4 residency in milliseconds
} computed_per_soc_24_hrs_metrics_t;

typedef struct
{
    die_mesh_pwr_metrics_t die_mesh_pwr;
} computed_per_die_metrics_t;

typedef struct
{
    mma_u16_t temperature_s0_dC;
    mma_u16_t temperature_s1_dC;
    mma_u16_t power_mW;
    uint32_t duration_mS;
    uint32_t entry_counts;
    uint8_t throttle_source;
} computed_per_dimm_metrics_t;

typedef struct
{
    mma_u32_t core_power;
    mma_u16_t active_pstate;
    uint32_t  residency_uS;
    uint8_t nominal_pstate;
} computed_per_mpam_metrics_t;

typedef struct
{
    d2d_link_metrics_t d2d_link[NUMBER_OF_D2D_LINKS_STATE];
} computed_per_d2dss_interface_metrics_t;

typedef struct
{
    mma_u32_t current_mA;
    mma_u32_t voltage_mV;
    mma_u32_t temperature_dC;
} computed_per_rail_metrics_t;

typedef struct
{
    running_u32_avg_t memory_avg_pwr_mW; // in-band per die dimm power average metric
    computed_per_rail_metrics_t vr_rail[MAX_NUM_OF_VR_RAILS];
    computed_per_dimm_metrics_t dimm[NUMBER_OF_DIMMS_PER_DIE];
    mma_u16_t top_sensor_temp_dC[NUMBER_OF_SOC_TEMP_SENSORS];
    mma_u16_t hnf_temperature_dC[NUMBER_OF_HNF_CHANNELS_PER_DIE];
} computed_per_soc_metrics_t;

typedef struct {
    computed_per_core_metrics_t cores[NUMBER_OF_CORES_PER_DIE];
    computed_per_soc_metrics_t soc;
    computed_per_mpam_metrics_t mpam[NUMBER_OF_MPAMS];
    computed_per_d2dss_interface_metrics_t d2dss[NUMBER_OF_D2D_INTERFACES];
    computed_per_die_metrics_t mesh;
} computed_metrics_2_min_t;

typedef struct
{
    computed_per_core_24_hrs_metrics_t cores[NUMBER_OF_CORES_PER_DIE];
    computed_per_soc_24_hrs_metrics_t soc;
} computed_metrics_24_hrs_t;

typedef struct
{
    mma_u32_t mpam_mem_pwr_mW[NUMBER_OF_MPAMS];
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
extern computed_metrics_24_hrs_t* p_computed_metrics_24_hrs;
#define computed_metrics_24_hrs (*p_computed_metrics_24_hrs)

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
 * @param[in] is_single_die_system True if the system is single die, false if dual die.
 * @return  None
 */
void comp_metrics_init(bool is_single_die_system);

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
 * @param[in] core_id  pertinent core
 * @param[in] core_voltage_mV  latest core voltage in mV
 * @param[in] core_temperature_dC  latest core temperature in dC
 */
void comp_metrics_for_single_core_histogram(uint8_t core_id, uint16_t core_voltage_mV, uint16_t core_temperature_dC);

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
 * @param[in] entry_count - number of throttle entries since last dimm packet
 * @param[in] throttle_duration_mS - total throttle duration since last dimm packet
 * @param[in] throttle_source - latest memory throttle source
 */
void comp_metrics_for_single_soc_dimm(uint8_t dimm_idx, uint16_t latest_dimm_temp_s0_dC, uint16_t latest_dimm_temp_s1_dC, uint16_t latest_dimm_power_mW, uint16_t entry_count, uint8_t throttle_duration_mS, uint8_t throttle_source);

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
void comp_metrics_for_mpam_data( mpam_data_t (*mpam_data_array)[NUMBER_OF_MPAMS]);

/**
 * @brief Update MPAM residency and nominal pstate metrics for a specific MPAM.
 *
 * @param[in] mpam_id - The identifier of the MPAM for which the residency and nominal pstate are being updated.
 * @param[in] residency_uS - The residency time in microseconds.
 * @param[in] nominal_pstate - The nominal pstate value.
 */
void comp_metrics_for_mpam_throttling(uint8_t mpam_id, uint32_t residency_uS, uint8_t nominal_pstate);

 /**
 * @brief Update the per-die mesh telemetry metrics based on the provided telemetry data.
 * 
 * @param[in] m1_entry_count - M1 entry count
 * @param[in] m2_entry_count - M2 entry count
 * @param[in] m0_residency_count - M0 residency count
 * @param[in] m1_residency_count - M1 residency count
 * @param[in] m2_residency_count - M2 residency count
 * @param[in] delivered_perf_count - Delivered performance count
 */
void comp_metrics_for_per_die_mesh_tlm(uint32_t m1_entry_count, uint32_t m2_entry_count, uint32_t m0_residency_count, uint32_t m1_residency_count, uint32_t m2_residency_count, uint32_t delivered_perf_count);

/**
 * @brief Update D2DSS interface metrics for all links of a specified D2DSS interface.
 *
 * @param[in] d2dss_id - The identifier of the D2DSS interface.
 * @param[in] tx_res_counter_diff - Array of transmit residency counter differences for each link.
 * @param[in] rx_res_counter_diff - Array of receive residency counter differences for each link.
 * @param[in] bw_tx_flit_counter_diff - Array of bandwidth transmit flit counter differences for each link.
 * @param[in] bw_rx_flit_counter_diff - Array of bandwidth receive flit counter differences for each link.
 */
void comp_metrics_for_single_d2dss_interface_all_links(uint8_t  d2dss_id,
                                                    uint64_t (*tx_res_counter_diff)[NUMBER_OF_D2D_LINKS_STATE],
                                                    uint64_t (*rx_res_counter_diff)[NUMBER_OF_D2D_LINKS_STATE],
                                                    uint64_t (*bw_tx_flit_counter_diff)[NUMBER_OF_D2D_LINKS_STATE],
                                                    uint64_t (*bw_rx_flit_counter_diff)[NUMBER_OF_D2D_LINKS_STATE]);
                                                    
/**
 * @brief Update SoC-level PC3/PC4 residency metrics based on core C-state transitions.
 *
 * @param[in] current_cstate - The current C-state of the core
 * @param[in] duration_mS - The duration in milliseconds spent in the current C-state
 */
void comp_metrics_for_soc_package_cstate(uint8_t pkg_cstate, uint32_t duration_mS);

/**
 * @brief Get the current running average DIMM power consumption for in-band VM
 * memory power publishing on Die 0.
 * 
 * This API is used by the in-band VM memory power publishing flow. It returns
 * the current running average DIMM power consumption, expressed in milliwatts
 * (mW), obtained from the computed metrics structure on a per-die basis.
 *
 * Die 1 updates the in-band DIMM power average using the metrics generated by
 * the comp_metrics_for_total_dimm_pwr() function. 
 *
 * @return int32_t -  Running average DIMM power consumption in milliwatts (mW).
 */
uint32_t comp_metrics_get_memory_avg_pwr_mW(void);

