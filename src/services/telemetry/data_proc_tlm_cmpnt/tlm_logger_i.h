//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file tlm_logger_i.h
 *  Private header
 * NOTE: Some of the internal data storage structures use the exact same format
 * as those externalized via telemetry.  Therefore <telemetry_package_defs.h> is included to prevent duplication.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif


/*----------- Nested includes ------------*/

#include <fpfw_status.h>
#include <sensor_fifo_service.h> // for sensor ram data structures
#include <stdint.h>
#include <telemetry_package_defs.h>

/*-- Symbolic Constant Macros (defines) --*/

#define MAX_BUFFER_ENTRIES 		 8
#define MAX_NUMBER_POWER_SAMPLE  100
#define DOUT2MILLIVOLTS(voltage) (voltage * 1000)


// The current conversion factor is set by default as 26.5 per bit.
#ifndef CORE_CURRENT_CONVERSION_FACTOR
    #define CORE_CURRENT_CONVERSION_FACTOR 26.5F
#endif

/*-------------- Typedefs ----------------*/
typedef enum
{
    THROTTLE_TRNSN_NONE = 0,
    THROTTLE_TRNSN_END,
    THROTTLE_TRNSN_START,
} throttle_transition_event_t;

typedef enum
{
    THROTTLE_SOURCE_NONE = 0,
    THROTTLE_SOURCE_CURRENT,
    THROTTLE_SOURCE_TEMPERATURE,
    THROTTLE_SOURCE_RACK_LIMIT,
    THROTTLE_SOURCE_VR_HOT,
    THROTTLE_SOURCE_ADAPTIVE_CLK,
} throttle_source_t;

/**
 *  @brief Core related runtime resources
 */

typedef struct {
    uint8_t pstate_id;
    uint32_t residency;
    uint32_t entry_count;
    uint16_t frequency;
    uint16_t max_power;
    uint16_t min_power;
    uint16_t avg_power;
} pwr_pstate_t;

typedef struct {
    uint8_t cstate_id;
    uint32_t residency;
    uint32_t entry_count;
    uint16_t max_power;
    uint16_t min_power;
    uint16_t avg_power;
} pwr_cstate_t;

typedef struct {
    uint8_t pstate_change : 1;
    uint8_t cstate_change : 1;
    uint8_t throttling_ev_change : 1;
    uint8_t rack_priority_change : 1;
    uint8_t id_change_bit : 1;
    uint8_t throttling_start_occurred : 1;
    uint8_t throttling_end_occurred : 1;
    uint8_t reserved : 1;
} core_control_flags_t;

typedef struct {
    core_control_flags_t flags;
    uint64_t cstate_timestamp;
    uint64_t pstate_timestamp;
    uint64_t throttle_timestamp;
    uint64_t current_pkt_timestamp;
    uint64_t throttling_counter;
    uint8_t current_pstate;
    uint8_t current_cstate;
    uint8_t current_plimit;
    uint8_t ldo_voltage;
    uint8_t throttling_status;
    uint8_t throttle_trnsn_event;
    uint8_t throttle_source;
    uint8_t throttling_priority_id;
    uint8_t current_tel_pstate;
    uint8_t current_power;
    uint8_t power_index;
    pwr_pstate_t pstate[NUMBER_OF_PSTATES];
    pwr_cstate_t cstate[NUMBER_OF_CSTATES];
    pwr_core_event_throttle_t throttle_info[NUMBER_OF_THROTTLE_TYPES];
    pwr_core_event_rack_priorities_t priorities[NUMBER_OF_RACK_PRIORITIES];
    voltage_t voltage;
    current_t current;
    temperature_t temperature;
    uint16_t current_mpam_id;
    uint8_t nominal_pstate;
} core_runtime_info_t;

typedef struct {
    uint16_t current_max_temperature;
    uint16_t max_tile_temperature;
    uint8_t current_max_id;
    uint8_t max_tile_id;
    voltage_t vcpu;
    voltage_t vsys;
} tile_runtime_info_t;

typedef struct {
    uint32_t soc_pc3_residency;
    uint32_t soc_pc4_residency;
    pwr_soc_event_vr_rail_t rail[MAX_NUM_OF_VR_RAILS];
    pwr_soc_event_hnf_t hnf[NUMBER_OF_HNF_CHANNELS_PER_DIE];
} soc_runtime_info_t;

/*--------- Function Prototypes ----------*/


/**
 * @brief Internal API to log tile/core/hnf temperature parameters
 *
 * @param temperature_data - SCF RAM formatted resource for temperature packets
 * @param tile_index - index to the tile id being referenced by the entry
 *
 * @return fpfw_status_t
 */
fpfw_status_t tlm_logger_log_tile_temperature(tile_temp_t* temperature_data, uint8_t tile_index);

/**
 * @brief Internal API to log tile/core voltages
 *
 * @param voltage_data - SCF RAM formatted resource for voltage packets
 * @param tile_index - index to the tile id being referenced by the entry
 *
 * @return fpfw_status_t
 */
fpfw_status_t tlm_logger_log_tile_voltage(tile_voltage_t* voltage_data, uint8_t tile_index);

/**
 * @brief Internal API to log core currents and power
 *
 * @param current_data - SCF RAM formatted resource for core current packets
 * @param core_index - index to the core id being referenced by the entry
 *
 * @return fpfw_status_t
 */
fpfw_status_t tlm_logger_log_core_current(core_current_t* current_data, uint8_t core_index);

/**
 * @brief helper function to update the pstate runtime timestamp
 *
 * @param core_id - core that is referenced to that owns this timestamp
 * @param pstate - pstate that is reference to where it needs to be updated
 * @param timestamp - timestamp used for the update
 *
 * @return fpfw_status_t
 */
fpfw_status_t update_core_pstate_timestamps(uint8_t core_id, uint8_t pstate, uint64_t timestamp);

/**
 * @brief Internal API to log voltage regulator (VR) temperatures
 *
 * @param vr_temperature - SCF RAM formatted resource for VR Temperatures
 *
 * @return None
 */
void tlm_logger_log_vr_temp(vr_temp_t* vr_temperature);

/**
 * @brief Internal API to log voltage regulator (VR) current and voltage
 *
 * @param vr_current - SCF RAM formatted resource for VR Currents (and voltages)
 *
 * @return None
 */
void tlm_logger_log_vr_current(vr_current_t* vr_current);

/**
 * @brief Internal API to log pstate telemetry packets, for pstate, cstate and throttling info
 *
 * @param pstate_telemetry - SCF RAM formatted resource for pstate packets
 *        NOTE: The Pstate packet contains the core id reference internally.
 *
 * @return fpfw_status_t
 */
fpfw_status_t tlm_logger_log_core_pstate(pstate_telem_t* pstate_telemetry);


#ifdef __cplusplus
}
#endif