//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file  data_proc_tlm_cmpnt.h
 * Internal public interface for the data processing component
 */

#pragma once

/*----------- Nested includes ------------*/
#include <telemetry_package_defs.h>

/*-- Symbolic Constant Macros (defines) --*/



/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize the data processing component.
 * @param[in] die_id The ID of the die to initialize.
 */
void data_proc_tlm_cmpnt_init(uint8_t die_id);

/**
 * @brief Notification on telemetry collection transitioning to disabled or enabled.
 *
 * @param[in] enable true if enabled, false if disabled.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_enable_disable_transition(bool enable);

/**
 * @brief Primary power data collection entry point.  Read raw data from sensor fifo and update telemetry aggregation structures.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_process_input_data(void);

/**
 * @brief Call prior to generating an instantaneous telemetry package.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_prepare_data_for_inst_sample(void);

/**
 * @brief Call prior to generating a power telemetry package.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg(void);

/**
 * @brief Call prior to generating a 24 hour telemetry package.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_prepare_data_for_24hr_pkg(void);

/**
 * @brief Get the core pstate data for the specified core.
 *
 * @param[in] core_id - The core id to get the pstate data for.
 * @param[out] pstate_array - Pointer to the array to store the pstate data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_pwr_core_pstate_data(uint16_t core_id, pwr_core_element_pstate_t (*pstate_array)[NUMBER_OF_PSTATES]);

/**
 * @brief Get the core cstate data for the specified core.
 *
 * @param[in] core_id - The core id to get the cstate data for. 0 .. NUMBER_OF_CORES_PER_DIE-1
 * @param[out] cstate_array - Pointer to the array to store the cstate data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_pwr_core_cstate_data(uint16_t core_id, pwr_core_element_cstate_t (*cstate_array)[NUMBER_OF_CSTATES]);

/**
 * @brief Get the core throttle data for the specified core.
 *
 * @param[in] core_id - The core id to get the throttle data for. 0 .. NUMBER_OF_CORES_PER_DIE-1
 * @param[out] throttle_array - Pointer to the array to store the throttle data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_pwr_core_throttle_data(uint16_t core_id, pwr_core_element_throttle_t (*throttle_array)[NUMBER_OF_THROTTLE_TYPES]);

/**
 * @brief Get the core rack priority data for the specified core.
 *
 * @param[in] core_id - The core id to get the rack priority data for. 0 .. NUMBER_OF_CORES_PER_DIE-1
 * @param[out] rack_priority_array - Pointer to the array to store the rack priority data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_pwr_core_rack_priority_data(uint16_t core_id, pwr_core_element_rack_priorities_t (*rack_priority_array)[NUMBER_OF_RACK_PRIORITIES]);

/**
 * @brief Get the core voltage data for the specified core.
 *
 * @param[in] core_id - The core id to get the voltage data for. 0 .. NUMBER_OF_CORES_PER_DIE-1
 * @param[out] voltage_data - Pointer to the structure to store the voltage data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_pwr_core_voltage_data(uint16_t core_id, p_pwr_core_element_voltage_t voltage_data);

/**
 * @brief Get the core current data for the specified core.
 *
 * @param[in] core_id - The core id to get the current data for. 0 .. NUMBER_OF_CORES_PER_DIE-1
 * @param[out] current_data - Pointer to the structure to store the current data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_pwr_core_current_data(uint16_t core_id, p_pwr_core_element_current_t current_data);

/**
 * @brief Get the core temperature data for the specified core.
 *
 * @param[in] core_id - The core id to get the temperature data for. 0 .. NUMBER_OF_CORES_PER_DIE-1
 * @param[out] temperature_data - Pointer to the structure to store the temperature data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_pwr_core_temperature_data(uint16_t core_id, p_pwr_core_element_temperature_t temperature_data);

/**
 * @brief Get the core power data for the specified core.
 *
 * @param core_id[in] - The core id to get the power data for. 0 .. NUMBER_OF_CORES_PER_DIE-1
 * @param power_data[out] - Pointer to the structure to store the power data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_pwr_core_power_data(uint16_t core_id, p_pwr_core_element_power_t power_data);

/**
 * @brief Get the core histogram data for the specified core.
 *
 * @param[in] core_id - The core id to get the histogram data for. 0 .. NUMBER_OF_CORES_PER_DIE-1
 * @param[out] histogram_array - Pointer to the two dimensional array to store the histogram data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_pwr_core_histogram_data(uint16_t core_id, pwr_core_element_histogram_t (*histogram_array)[NUMBER_OF_HS_VOLTAGE_SCALES][NUMBER_OF_HS_TEMP_SCALES]);

/**
 * @brief Get the soc pc3 data for the specified soc.
 *
 * @param[out] soc_pkg_mon_data - Pointer to the structure to store the soc pc3 data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_pwr_soc_pkg_mon_data(p_pwr_soc_element_pkg_monitor_t soc_pkg_mon_data);

/**
 * @brief Get the soc VR rail data for the specified rail.
 *
 * @param[in] rail_id - The rail id to get the VR rail data for. 0 .. MAX_NUM_OF_VR_RAILS-1
 * @param[out] rail_data - Pointer to the structure to store the rail data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_pwr_soc_vr_rail_data(uint16_t rail_id, p_pwr_soc_element_vr_rail_t rail_data);

/**
 * @brief Get the soc hnf data for the specified hnf channel.
 *
 * @param[in] hnf_channel - The hnf channel to get the hnf data for. 0 .. NUMBER_OF_HNF_CHANNELS_PER_DIE-1
 * @param[out] hnf_data - Pointer to the structure to store the hnf data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_pwr_soc_hnf_data(uint16_t hnf_channel, p_pwr_soc_element_hnf_t hnf_data);

/**
 * @brief Get the soc dimm temperature data for the specified dimm module.
 *
 * @param[in] dimm_module - The dimm module to get the dimm data for. 0 .. NUMBER_OF_DIMM_MODULES_PER_DIE-1
 * @param[out] dimm_data - Pointer to the structure to store the dimm temperature data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_pwr_soc_temp_dimm_data(uint16_t dimm_module, p_pwr_soc_element_dimm_temp_t dimm_data);

/**
 * @brief Get the soc dimm power data for the specified dimm module.
 *
 *
 * @param[in] dimm_module - The dimm module to get the dimm data for. 0 .. NUMBER_OF_DIMM_MODULES_PER_DIE-1
 * @param[out] dimm_data - Pointer to the structure to store the dimm power data in.
 */
void data_proc_tlm_cmpnt_get_pwr_soc_power_dimm_data(uint16_t dimm_module, p_pwr_soc_element_dimm_power_t dimm_data);

/**
 * @brief Get the soc sensor temperature data for the specified sensor.
 *
 * @param[in] sensor_id - The sensor id to get the sensor data for. 0 .. NUMBER_OF_SOC_TEMP_SENSORS-1
 * @param[out] sensor_temp_data - Pointer to the structure to store the sensor temperature data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_pwr_soc_snsr_temp_data(uint16_t sensor_id, p_pwr_soc_element_sensor_temp_t sensor_temp_data);

/**
 * @brief Get the soc maximum temperature data.
 *
 * @param[out] max_temp_data - Pointer to the structure to store the maximum temperature data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_pwr_soc_max_temp_data(p_pwr_soc_element_max_soc_temp_t max_temp_data);

/**
 * @brief Get the mpam pstate data for the specified mpam.
 *
 * @param[in] mpam_id - The mpam_id to get the data for. 0 .. NUMBER_OF_MPAMS-1
 * @param[out] mpam_pstate_array - Pointer to the array to store the mpam pstate data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_pwr_mpam_pstate_data(uint16_t mpam_id, pwr_soc_element_mpam_pstate_t (*mpam_pstate_array)[NUMBER_OF_PSTATES]);

/**
 * @brief Get the mpam thottle data for the specified mpam.
 *
 * @param[in] mpam_id - The mpam_id to get the data for. 0 .. NUMBER_OF_MPAMS-1
 * @param[out] mpam_throttle_data - Pointer to the location to store the mpam throttle data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_pwr_soc_mpam_throttle_data(uint16_t mpam_id, p_pwr_soc_element_mpam_throttle_t mpam_throttle_data);

/**
 * @brief Get the core summary instantaneous data for the specified core.
 *
 * @param[in] core_id - The core id to get the data for. 0 .. NUMBER_OF_CORES_PER_DIE-1
 * @param[out] core_summary_data - Pointer to the location to store the core summary data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_inst_soc_core_summary_data(uint16_t core_id, p_inst_core_element_summary_t core_summary_data);

/**
 * @brief Get the soc VR rail data for the specified rail.
 *
 * @param[in] rail_id - The rail id to get the data for. 0 .. MAX_NUM_OF_VR_RAILS-1
 * @param[out] rail_data - Pointer to the location to store the rail data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_inst_soc_rail_data(uint16_t rail_id, p_inst_soc_element_rail_t rail_data);

/**
 * @brief Get the soc instantaneous dimm data for the specified dimm module.
 *
 * @param[in] dimm_module - The dimm module to get the data for. 0 .. NUMBER_OF_DIMM_MODULES_PER_DIE-1
 * @param[out] dimm_data - Pointer to the location to store the dimm data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_inst_soc_dimm_runtime_data(uint16_t dimm_module, p_inst_soc_element_dimm_runtime_t dimm_data);

/**
 * @brief Get the soc instantaneous sensor temperature data for the specified sensor.
 *
 * @param[in] sensor_id - The sensor id to get the data for. 0 .. NUMBER_OF_SOC_TEMP_SENSORS-1
 * @param[out] sensor_data - Pointer to the location to store the sensor data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_inst_soc_snsr_temp_data(uint16_t sensor_id, p_inst_soc_element_die_temp_t sensor_temp_data);

/**
 * @brief Get the soc maximum temperature data.
 *
 * @param[out] max_temp_data - Pointer to the location to store the maximum temperature data in.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_get_inst_soc_max_temp_data(p_inst_soc_element_max_temp_t max_temp_data);

/**
 * @brief Clear the power telemetry data. Used for testing purposes.
 *
 * @return None
 */
void data_proc_tlm_cmpnt_clear_pwr_tlm_data(void);

/**
 * @brief This API clear the aggregated telemetry data after the collection window,
 * @return None
 */
void data_proc_tlm_cmpnt_pwr_pkg_completed(void);

/**
 * @brief This API clear the aggregated 24hr telemetry data after the collection.
 * @return None
 */
void data_proc_tlm_cmpnt_24hr_pkg_completed(void);

/**
 * @brief Received notification from primary die to copy data to exchange that requires data from all dies.
 *
 *
 * @return None
 */
void data_proc_tlm_cmpnt_received_prep_pwr_pkg_from_prim_core(void);

/**
 * @brief Get the out-of-band critical sensor maximum SoC temperature.
 *
 * @return The maximum SoC temperature in degrees deci-Celsius.
 */
uint16_t data_proc_tlm_cmpnt_get_oob_crit_max_soc_temp_dC(void);

/**
 * @brief Get the out-of-band critical sensor SOC power in milli-watts.
 *
 * @return Total SOC power in milli-watts.
 */
uint32_t data_proc_tlm_cmpnt_get_oob_soc_pwr_mW(void);

/**
 * @brief Get the out-of-band total dimm power.
 *
 * @return The total Dimm power in milliwatts.
 */
uint32_t data_proc_tlm_cmpnt_get_oob_dimm_total_pwr_mW(void);

/**
 * @brief Get the out-of-band critical sensor maximum DIMM temperature.
 *
 * @return The maximum DIMM temperature in degrees deci-Celsius.
 */
uint16_t data_proc_tlm_cmpnt_get_oob_crit_max_dimm_temp_dC(void);

/**
 * @brief Get the out-of-band critical sensor maximum VR temperature.
 *
 * @return The maximum VR temperature in degrees deci-Celsius.
 */
uint16_t data_proc_tlm_cmpnt_get_oob_crit_max_vr_temp_dC(void);

