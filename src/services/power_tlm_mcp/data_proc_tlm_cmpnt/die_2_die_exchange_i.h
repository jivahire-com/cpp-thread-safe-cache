//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file die_2_die_exchange_i.h
 * This file contains the private interface for the data exchange
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdint.h> // for uint8_t
#include <telemetry_package_defs.h>

/*-- Symbolic Constant Macros (defines) --*/
#define PRIMARY_DIE_ID (0)
#define NUMBER_OF_SECONDARY_DIES (1)
#define FIRST_SECONDARY_DIE_ID   (1)

#define NUMBER_OF_MEM_CONTROLLERS_PER_DIE (12)
//All the counters including untracked.
#define NUMBER_OF_MPAMS_PER_MEM_AND_UNTRACK_CTRLR (8)
#define NUMBER_OF_MPAMS_PER_MEM_CTRLR (7)

#define NUMBER_OF_DIMMS (6 )

/*-------------- Typedefs ----------------*/
typedef enum {
    ALL_CORES_MIXED_C3_C4_state = 0,
    ALL_CORES_IN_C3_state,
    ALL_CORES_IN_C4_state
} pkg_cstate_t;

typedef struct __attribute__((packed))
{
    uint16_t average_max_temp_dC;
    uint16_t num_samples;
    uint16_t peak_temp_dC;
} max_die_temps_t, *p_max_die_temps_t;

typedef struct __attribute__((packed))
{
    uint32_t sum;
    uint16_t num_samples;
} sliding_window_data_t, *p_sliding_window_data_t;

typedef struct __attribute__((packed))
{
    uint16_t average_pwr_mW;
    uint16_t average_temp_dC;
    uint16_t max_temp_dC;
} dimm_data_t, *p_dimm_data_t;

typedef struct __attribute__((packed))
{
    uint32_t latest_total_pwr_mW; // sum of all cores assigned to mpam
    uint8_t latest_pstate;
    union {
        struct {
            uint8_t active : 1;
            uint8_t throttling : 1;
            uint8_t reserved : 6;  // Padding bits for future use
        };
        uint8_t flags_byte;  // Access the entire byte directly
    };
} mpam_data_t, *p_mpam_data_t;

typedef struct __attribute__((packed))
{
    pkg_cstate_t pkg_cstate;   
} pkg_mon_data_t, *p_pkg_mon_data_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize the die to die exchange component.
 * This function initializes the die to die exchange component for a specific die.
 *
 * @param[in] die_id The ID of the die to initialize.
 */
void die_2_die_exch_init(uint8_t die_id);

/**
 * @brief Get the ID of the current die.
 * This function returns the ID of the die that is currently being processed.
 *
 * @return The ID of the current die.
 */
uint8_t die_2_die_exch_get_this_die_id(void);

/**
 * @brief Get the MPAM data sequence violation counts and clear them.
 * This function returns the number of reads without new data and writes without consumption for MPAM data exchange,
 * then clears the counters after reading.
 *
 * @param[out] reads_without_new_data Pointer to store the count of reads that occurred without a write since the last read.
 *                                    Can be NULL if this count is not needed.
 * @param[out] writes_without_consumption Pointer to store the count of writes that occurred without subsequent reads.
 *                                        Can be NULL if this count is not needed.
 */
void die_2_die_exch_get_mpam_data_missed_counts(uint32_t* reads_without_new_data, uint32_t* writes_without_consumption);

/**
 * @brief Write the maximum die temperature to the die to die exchange.
 * This function writes the maximum die temperature in deci-Celsius to the exchange. It writes the location for the the
 * die specified by `this_die_id`, which is initialized using `die_2_die_exch_init`.
 *
 * @param[in] max_die_temperature_dC The maximum die temperature in deci-Celsius.
 */
void die_2_die_exch_ib_write_inst_max_die_temp(uint16_t max_die_temperature_dC);

/**
 * @brief Read the maximum die temperature from the die to die exchange.
 * This function reads the maximum die temperature in deci-Celsius for a specific die.
 *
 * @param[in] die_id The ID of the die to read the temperature from.
 * @return The maximum die temperature in deci-Celsius.
 */
uint16_t die_2_die_exch_ib_read_inst_max_die_temp_dC(uint8_t die_id);

/**
 * @brief Write the total memory power to the die to die exchange.
 * This function writes the total memory power in milliwatts to the exchange. It writes the location for the
 * die specified by `this_die_id`, which is initialized using `die_2_die_exch_init`.
 *
 * @param[in] total_memory_power_mW The total memory power in milliwatts.
 */
void die_2_die_exch_ib_write_total_memory_power_mW(uint32_t total_memory_power_mW);

/**
 * @brief Read the total memory power from the die to die exchange.
 * This function reads the total memory power in milliwatts for a specific die.
 *
 * @param[in] die_id The ID of the die to read the memory power from.
 * @param[out] total_memory_power_mW  in milliwatts.
 */
uint32_t  die_2_die_exch_ib_read_total_memory_power_mW(uint8_t die_id);

/**
 * @brief Write the maximum die temperature for the power package to the die to die exchange.
 * This function writes the average maximum temperature, number of samples, and peak temperature for the power package
 * to the exchange. It writes the location for the die specified by `this_die_id`, which is initialized using
 * `die_2_die_exch_init`.
 *
 * @param[in] average_max_temp_dC The average maximum temperature in deci-Celsius.
 * @param[in] num_samples The number of samples used to calculate the average.
 * @param[in] peak_temp_dC The peak temperature in deci-Celsius.
 */
void die_2_die_exch_ib_write_pwr_pkg_max_die_temp(uint16_t average_max_temp_dC, uint16_t num_samples, uint16_t peak_temp_dC);

/**
 * @brief Read the maximum die temperature for the power package from the die to die exchange.
 * This function reads the maximum die temperature for the power package for a specific die.
 *
 * @param[in] die_id The ID of the die to read the temperature from.
 * @param[out] max_die_temperature Pointer to a structure to store the maximum die temperature data.
 */
void die_2_die_exch_ib_read_pwr_pkg_max_die_temp_dC(uint8_t die_id, p_max_die_temps_t max_die_temperature);

/**
 * @brief Write the MPAM data to the die to die exchange.
 * This function writes the MPAM data for all MPAMs to the exchange. It writes the location for the die
 * specified by `this_die_id`, which is initialized using `die_2_die_exch_init`.
 *
 * @param[in] mpam_data_array Pointer to an array of MPAM data structures.
 */
void die_2_die_exch_ib_write_pwr_pkg_mpam_data(mpam_data_t (*mpam_data_array)[NUMBER_OF_MPAMS]);

/**
 * @brief Read the MPAM data from the die to die exchange.
 * This function reads the MPAM data for all MPAMs from a specific die.
 *
 * @param[in] die_id The ID of the die to read the MPAM data from.
 * @param[out] mpam_data_array Pointer to an array to store the MPAM data structures.
 */
void die_2_die_exch_ib_read_pwr_pkg_mpam_data(uint8_t die_id, mpam_data_t (*mpam_data_array)[NUMBER_OF_MPAMS]);

/**
 * @brief Write the package monitor data to the die to die exchange.
 * This function writes the package monitor data (PC3/PC4 durations) to the exchange. It writes the location for the
 * die specified by `this_die_id`, which is initialized using `die_2_die_exch_init`.
 *
 * @param[in] pkg_mon_data The package monitor data.
 */
void die_2_die_exch_ib_write_pwr_pkg_mon_data(p_pkg_mon_data_t pkg_mon_data);

/**
 * @brief Read the package monitor data from the die to die exchange.
 * This function reads the package monitor data for a specific die.
 *
 * @param[in] die_id The ID of the die to read the package monitor data from.
 * @param[out] pkg_mon_data Pointer to a structure to store the package monitor data.
 */
void die_2_die_exch_ib_read_pwr_pkg_mon_data(uint8_t die_id, p_pkg_mon_data_t pkg_mon_data);

/**
 * @brief Write the MPAM memory counters to the die to die exchange.
 * This function writes (replaces) the MPAM memory counters in the die to die exchange. 
 * The read API will clear the values for the next package write.
 *
 * @param[in] mpam_counters Pointer to a flattened array of MPAM memory counters [12][8].
 * @param[in] unallocated Pointer to the unallocated mpam counter value.
 */
void die_2_die_exch_ib_write_pwr_pkg_mpam_mem_counters(uint64_t *mpam_counters, uint64_t* unallocated);

/**
 * @brief Read the MPAM memory counters from the die to die exchange.
 * This function reads the MPAM memory counters and clears the values for the next package write.
 *
 * @param[out] mpam_counters Pointer to a flattened array to store the MPAM memory counters [12][8].
 * @param[out] unallocated Pointer to the unallocated mpam counter value.
 */
void die_2_die_exch_ib_read_pwr_pkg_mpam_mem_counters(uint64_t *mpam_counters, uint64_t* unallocated);

/**
 * @brief Write the maximum die temperature window to the die to die exchange.
 * This function writes the summation of die temperatures and the number of samples to the exchange. It writes the
 * location for the die specified by `this_die_id`, which is initialized using `die_2_die_exch_init`.
 *
 * @param[in] summation_dC The summation of die temperatures in deci-Celsius.
 * @param[in] num_samples The number of samples used to calculate the summation.
 */
void die_2_die_exch_oob_write_window_max_die_temp(uint32_t summation_dC, uint16_t num_samples);

/**
 * @brief Read the maximum die temperature window from the die to die exchange.
 * This function reads the maximum die temperature window for a specific die.
 *
 * @param[in] die_id The ID of the die to read the temperature window from.
 * @param[out] max_die_temp_window Pointer to a structure to store the maximum die temperature window data.
 */
void die_2_die_exch_oob_read_window_max_die_temp(uint8_t die_id, p_sliding_window_data_t max_die_temp_window);

/**
 * @brief Write the SoC power window to the die to die exchange.
 * This function writes the summation of SoC power and the number of samples to the exchange. It writes the location for
 * the die specified by `this_die_id`, which is initialized using `die_2_die_exch_init`.
 *
 * @param[in] summation_mW The summation of SoC power in milliwatts.
 * @param[in] num_samples The number of samples used to calculate the summation.
 */
void die_2_die_exch_oob_write_window_soc_pwr(uint32_t summation_mW, uint16_t num_samples);

/**
 * @brief Read the SoC power window from the die to die exchange.
 * This function reads the SoC power window for a specific die.
 *
 * @param[in] die_id The ID of the die to read the SoC power window from.
 * @param[out] die_soc_pwr_window Pointer to a structure to store the SoC power window data.
 */
void die_2_die_exch_oob_read_window_soc_pwr(uint8_t die_id, p_sliding_window_data_t die_soc_pwr_window);

/**
 * @brief Write the maximum DIMM temperature to the die to die exchange.
 * This function writes the summation of DIMM temperatures and the number of samples to the exchange.
 *
 * @param[in] summation_dC The summation of DIMM temperatures in deci-Celsius.
 * @param[in] num_samples The number of samples used to calculate the summation.
 */
void die_2_die_exch_oob_write_window_max_dimm_temp(uint32_t summation_dC, uint16_t num_samples);

/**
 * @brief Read the maximum DIMM temperature window from the die to die exchange.
 * This function reads the maximum DIMM temperature window for a specific die.
 *
 * @param[in] die_id The ID of the die to read the temperature window from.
 * @param[out] max_dimm_temp_window Pointer to a structure to store the maximum DIMM temperature window data.
 */
void die_2_die_exch_oob_read_window_max_dimm_temp(uint8_t die_id, p_sliding_window_data_t max_dimm_temp_window);

/**
 * @brief Write the Dimm power window to the die to die exchange.
 * This function writes the summation of Dimm power and the number of samples to the exchange. It writes the location for
 * the die specified by `this_die_id`, which is initialized using `die_2_die_exch_init`.
 *
 * @param[in] summation_mW The summation of SoC power in milliwatts.
 * @param[in] num_samples The number of samples used to calculate the summation.
 */
void die_2_die_exch_oob_write_window_dimm_pwr(uint32_t summation_mW, uint16_t num_samples);

/**
 * @brief Read the Dimm power window from the die to die exchange.
 * This function reads the Dimm power window for a specific die.
 *
 * @param[in] die_id The ID of the die to read the Dimm power window from.
 * @param[out] die_dimm_pwr_window Pointer to a structure to store the Dimm power window data.
 */
void die_2_die_exch_oob_read_window_dimm_pwr(uint8_t die_id, p_sliding_window_data_t die_dimm_pwr_window);

/**
 * @brief Write the maximum VR temperature to the die to die exchange.
 * This function writes the summation of VR temperatures and the number of samples to the exchange.
 *
 * @param[in] summation_dC The summation of VR temperatures in deci-Celsius.
 * @param[in] num_samples The number of samples used to calculate the summation.
 */
void die_2_die_exch_oob_write_window_max_vr_temp(uint32_t summation_dC, uint16_t num_samples);

/**
 * @brief Read the maximum VR temperature window from the die to die exchange.
 * This function reads the maximum VR temperature window for a specific die.
 *
 * @param[in] die_id The ID of the die to read the temperature window from.
 * @param[out] max_vr_temp_window Pointer to a structure to store the maximum VR temperature window data.
 */
void die_2_die_exch_oob_read_window_max_vr_temp(uint8_t die_id, p_sliding_window_data_t max_die_temp_window);

/**
 * @brief Write the average P-state to the die to die exchange.
 * This function writes the summation of P-state values and the number of samples to the exchange.
 *
 * @param[in] summation The summation of P-state values.
 * @param[in] num_samples The number of samples used to calculate the summation.
 */
void die_2_die_exch_oob_write_window_avg_pstate(uint32_t summation, uint16_t num_samples);

/**
 * @brief Read the average P-state from the die to die exchange.
 * This function reads the average P-state for a specific die.
 *
 * @param[in] die_id The ID of the die to read the P-state from.
 * @param[out] avg_pstate_window Pointer to a structure to store the average P-state data.
 */
void die_2_die_exch_oob_read_avg_pstate(uint8_t die_id, p_sliding_window_data_t avg_pstate_window);

/**
 * @brief Write the DIMM channel information to the die to die exchange.
 * This function writes the average power, average temperature, and maximum temperature for a specific DIMM channel.
 * Note: the data is written per channel, as that is how the data is collected, but all channels are read at the same time.
 *
 * @param[in] channel The DIMM channel to write the information for (0 to NUMBER_OF_DIMMS-1).
 * @param[in] average_pwr_mW The average power in milliwatts.
 * @param[in] average_temp_dC The average temperature in deci-Celsius.
 * @param[in] latest_temp_dC The current temperature in deci-Celsius.
 */
void die_2_die_exch_oob_write_dimm_info(uint8_t channel, uint16_t average_pwr_mW, uint16_t average_temp_dC, uint16_t latest_temp_dC);

/**
 * @brief Read the DIMM channel average temperature from the die to die exchange.
 * @param[in] die_id The ID of the die to read the information from.
 * @param[in] channel The DIMM channel to read the information for (0 to NUMBER_OF_DIMMS-1).
 * @return The average temperature in deci-Celsius for the specified channel.
 */
uint16_t die_2_die_exch_oob_read_dimm_avg_temp_dC(uint8_t die_id, uint8_t channel);

/**
 * @brief Read the DIMM channel maximum temperature from the die to die exchange.
 * @param[in] die_id The ID of the die to read the information from.
 * @param[in] channel The DIMM channel to read the information for (0 to NUMBER_OF_DIMMS-1).
 * @return The maximum temperature in deci-Celsius for the specified channel.
 */
uint16_t die_2_die_exch_oob_read_dimm_max_temp_dC(uint8_t die_id, uint8_t channel);

/**
 * @brief Read the DIMM channel average power from the die to die exchange.
 * @param[in] die_id The ID of the die to read the information from.
 * @param[in] channel The DIMM channel to read the information for (0 to NUMBER_OF_DIMMS-1).
 * @return The average power in milliwatts for the specified channel.
 */
uint16_t die_2_die_exch_oob_read_dimm_avg_pwr_mW(uint8_t die_id, uint8_t channel);