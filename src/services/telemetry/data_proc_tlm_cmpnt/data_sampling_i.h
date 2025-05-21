//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file data_sampling_i.h
 *  Private header
 * NOTE: Some of the internal data storage structures use the exact same format
 * as those externalized via telemetry.  Therefore <telemetry_package_defs.h> is included to prevent duplication.
 */

#pragma once


/*----------- Nested includes ------------*/

#include <fpfw_status.h>
#include <sensor_fifo_service.h> // for sensor ram data structures
#include <stdint.h>
#include <telemetry_package_defs.h>

/*-- Symbolic Constant Macros (defines) --*/

#define MAX_BUFFER_ENTRIES 		 8
#define MAX_NUMBER_POWER_SAMPLE  100
#define DOUT2MILLIVOLTS(voltage) (voltage * 1000)
#define MICROSECONDS_TO_MILLISECONDS(time_diff_uS) ((time_diff_uS) / 1000)


// The current conversion factor is set by default as 26.5 per bit.
#ifndef CORE_CURRENT_CONVERSION_FACTOR
    #define CORE_CURRENT_CONVERSION_FACTOR 26.5F
#endif
// The power conversion factor is set by default as 22mW per bit.
#ifndef CORE_POWER_MW_PER_BIT
    #define CORE_POWER_MW_PER_BIT 22
#endif

#define ROUND_USEC_TO_MSEC(usec) ((usec + 500) / 1000)

/*-------------- Typedefs ----------------*/
typedef enum
{
    THROTTLE_TRNSN_NONE = 0,
    THROTTLE_TRNSN_END,
    THROTTLE_TRNSN_START,
} throttle_transition_event_t;

typedef enum
{
    THROTTLE_SOURCE_CURRENT=0,
    THROTTLE_SOURCE_TEMPERATURE,
    THROTTLE_SOURCE_RACK_LIMIT,
    THROTTLE_SOURCE_VR_HOT,
    THROTTLE_SOURCE_ADAPTIVE_CLK,
    THROTTLE_SOURCE_CURRENT_OVERRUN,
    THROTTLE_SOURCE_ADAPTIVE_CLK_OVERRUN,
} throttle_source_t;

// enum for type of power telemetry components soc,tile or core.
typedef enum
{
    PWR_TLM_SOC_UPDATE = 0,
    PWR_TLM_CORE_UPDATE,
    PWR_TLM_TILE_UPDATE,
} pwr_tlm_update_t;

/**
 *  @brief Core related runtime resources
 */

typedef struct {
    uint8_t pstate_id;
    uint32_t residency_uS;
    uint32_t entry_count;
    uint16_t frequency_Mhz;
    uint16_t max_power_mW;
    uint16_t min_power_mW;
    uint16_t avg_power_mW;
    uint16_t latest_value_mW;// latest power
} pwr_pstate_t;

typedef struct {
    uint8_t cstate_id;
    uint32_t residency_uS;
    uint32_t entry_count;
    uint16_t max_power_mW;
    uint16_t min_power_mW;
    uint16_t avg_power_mW;
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
    uint64_t cstate_timestamp_uS; //for cstate residency update.
    uint64_t pstate_timestamp_uS;
    uint64_t current_pkt_timestamp;
    uint8_t pstate_from_pstate_pkt; /* pstate from pstate packet*/
    uint8_t cstate_from_pstate_pkt; /* cstate from pstate packet from sensor fifo*/
    uint8_t active_sample_plimit;
    uint8_t ldo_voltage;
    uint8_t throttling_status;
    uint8_t throttle_event;
    uint8_t throttle_source;
    uint8_t throttling_priority_id;
    uint64_t throttle_previous_timestamp_uS[NUMBER_OF_THROTTLE_TYPES];
    uint32_t time_counter_uS; // for general residency calculation for the core in uS
    uint8_t pstate_from_current_pkt; /* pstate from current packet, during throttling */
    uint32_t average_pwr_mW; //running average of power samples values in mW .
    uint8_t num_pwr_samples;
    pwr_pstate_t pstate[NUMBER_OF_PSTATES];
    pwr_cstate_t cstate[NUMBER_OF_CSTATES];
    pwr_core_element_throttle_t throttle_info[NUMBER_OF_THROTTLE_TYPES];
    pwr_core_element_rack_priorities_t priorities[NUMBER_OF_RACK_PRIORITIES];
    uint16_t latest_voltage_mV;
    current_t current;
    uint16_t latest_power_mW;
    temperature_t temperature;
    uint16_t active_sample_mpam_id;
    bool core_throttling_tracker[NUMBER_OF_THROTTLE_TYPES];
} core_runtime_info_t;

typedef struct {
    uint32_t time_counter_uS;
    uint16_t active_sample_max_temperature_dC;
    uint16_t max_tile_temperature_dC;
    uint8_t active_sample_max_id;
    uint8_t max_tile_id;
    voltage_t vcpu;
    voltage_t vsys;
} tile_runtime_info_t;

typedef struct {
    uint32_t time_counter_uS;//time counter for residency calculation, add time_diff on every iteration.
    uint32_t soc_pc3_residency_mS;
    uint32_t soc_pc4_residency_mS;
    pwr_soc_element_vr_rail_t rail[MAX_NUM_OF_VR_RAILS];
    pwr_soc_element_hnf_t hnf[NUMBER_OF_HNF_CHANNELS_PER_DIE];
    pwr_soc_element_sensor_temp_t sensor_temp[NUMBER_OF_SOC_TEMP_SENSORS];
} soc_runtime_info_t;

typedef struct {
    uint64_t residency_uS;//time counter for residency calculation, add time_diff on every iteration.
    uint64_t previous_soc_dimm_timestamp_uS;
    pwr_soc_element_dimm_temp_t      dimm_temp[NUMBER_OF_DIMM_MODULES];
    pwr_soc_element_dimm_power_t     dimm_pwr[NUMBER_OF_DIMM_MODULES];
    inst_soc_element_dimm_runtime_t  dimm_inst[NUMBER_OF_DIMM_MODULES];
} soc_runtime_dimm_info_t;

/**
 *  @brief Enum for Pstate message throttle status codes
 * // Status from KNG RMSSHASv0.p14 Document index value
 */
typedef enum _pstate_throttle_status_t
{
    NO_THROTTLING = 0,
    CURRENT_THROTTLING_START,
    TEMP_THROTTLING_START,
    RACK_THROTTLING_START,
    SYS_FRC_PMIN_THROTTLING_START,
    ADPT_CLK_THROTTLING_START,
    CURRENT_THROTTLING_END,
    TEMP_THROTTLING_END,
    RACK_THROTTLING_END,
    SYS_FRC_PMIN_THROTTLING_END,
    ADPT_CLK_THROTTLING_END,
    CURRENT_THROTTLING_OVERRUN,
    ADPT_CLK_THROTTLING_OVERRUN
} pstate_throttle_status_t;

/*-- Declarations (Statics and globals) --*/

extern core_runtime_info_t core[NUMBER_OF_CORES_PER_DIE];
extern tile_runtime_info_t tile[NUMBER_OF_TILES_PER_DIE];
extern soc_runtime_info_t soc_info;
extern soc_runtime_dimm_info_t soc_dimm;
extern uint32_t core_pwr_samples_accumulation_mW[NUMBER_OF_CORES_PER_DIE];
extern uint32_t pstate_accum_uS[NUMBER_OF_CORES_PER_DIE][NUMBER_OF_PSTATES];

/*--------- Function Prototypes ----------*/
/**
 * @brief Initialize power telemetry data.
 *
 * @return none
 */
void data_smpl_init(void);

/**
 * @brief Initialize the dts coeff data structure.
 *
 * @return none
 */
void data_smpl_init_dts_coefficients(void);

/**
 * @brief Initialize the static information, like cstate id, pstate, etc..
 *
 * @return none
 */
void data_smpl_init_constants();

/**
 * @brief Internal API to log tile/core/hnf temperature parameters
 *
 * @param[in] temperature_data - SCF RAM formatted resource for temperature packets
 * @param[in] tile_index - index to the tile id being referenced by the entry
 *
 * @return None
 */
void data_smpl_parse_tile_temperature_entry(tile_temp_t* temperature_data, uint8_t tile_index);

/**
 * @brief Internal API to log tile/core voltages
 *
 * @param[in] voltage_data - SCF RAM formatted resource for voltage packets
 * @param[in] tile_index - index to the tile id being referenced by the entry
 *
 * @return true if valid
 */
bool data_smpl_parse_tile_voltage_entry(tile_voltage_t* voltage_data, uint8_t tile_index);

/**
 * @brief Internal API to log core currents and power
 *
 * @param[in] current_data - SCF RAM formatted resource for core current packets
 * @param[in] core_index - index to the core id being referenced by the entry
 *
 * @return void
 */
void data_smpl_parse_core_current_entry(core_current_t* current_data, uint8_t core_index);

/**
 * @brief Internal API to log voltage regulator (VR) temperatures
 *
 * @param[in] vr_temperature - SCF RAM formatted resource for VR Temperatures
 *
 * @return None
 */
void data_smpl_parse_vr_temperature_entry(vr_temp_t* vr_temperature);

/**
 * @brief Internal API to log voltage regulator (VR) current and voltage
 *
 * @param[in] vr_current - SCF RAM formatted resource for VR Currents (and voltages)
 *
 * @return None
 */
void data_smpl_parse_vr_current_entry(vr_current_t* vr_current);

/**
 * @brief Internal API to log soc pvt temperature (SOC_TOP_TEMP)
 *
 * @param[in] soc_pvt_temp_t - SCF RAM formatted resource for soc_pvt_temp
 *
 * @return None
 */
void data_smpl_parse_pvt_temperature_entry(soc_pvt_temp_t* pvt_temperature);

/**
 * @brief Internal API to log soc dimm information (DIMM)
 *
 * @param[in] sensor_ram_dimm_info_t - SCF RAM formatted resource for dimm
 *
 * @return true if valid
 */
bool  data_smpl_parse_dimm_entry(sensor_ram_dimm_info_t* dimm_info);

/**
 * @brief Internal API to log pstate telemetry packets, for pstate, cstate and throttling info
 *
 * @param[in] pstate_telemetry - SCF RAM formatted resource for pstate packets
 *        NOTE: The Pstate packet contains the core id reference internally.
 *
 * @return None
 */
void data_smpl_parse_pstate_no_throttling(pstate_telem_t* pstate_telemetry);

/**
 * @brief Internal API to log cstate telemetry.
 * @param[in] cstate_telemetry - SCF RAM formatted resource for pstate packets
 *        (IMPORTANT : pstate telemetry packet provide both p state/c state))
 *        NOTE: The Pstate packet contains the core id reference internally.
 * @return None
 */
void data_smpl_parse_cstate_no_throttling(pstate_telem_t* cstate_telemetry);

/**
 * @brief update rack throttling
 *
 * @param[in] pstate_telemetry  provide pstate tlm pkt.
 * @param throttle_index - source of throttle , in this case rack.
 * @param core_id
 */
void data_smpl_parse_rack_throttling(pstate_telem_t* pstate_telemetry, int throttle_index, uint8_t core_id);

/**
 * @brief Internal API to log states-pstate,cstate and also log core throttling telemetry.
 * @param[in] pstate_telemetry - SCF RAM formatted resource for pstate packets
 *        (IMPORTANT : pstate telemetry packet provide both p state/c state and throttling status))
 *
 * @return void
 */
void data_smpl_parse_core_states_entry(pstate_telem_t* pstate_telemetry);

/**
 * @brief  calculate throttling index based on throttle status in telemetry pkt.
 *
 * @param[in] status  1-12 if success or -1 in case of a error.
 * @return int return index for throttle info array.
 */
int8_t data_smpl_parse_throttling_get_index_from_status(pstate_throttle_status_t status);

/**
 * @brief log the throttling states, based on pstate pkt and start/end event
 * from pkt.
 *
 * @param[in] pstate_telemetry
 * @return None
 */
void data_smpl_parse_throttling(pstate_telem_t* pstate_telemetry);

/**
 * @brief This api clear the throttling tracker.
 *
 * @param[in] core_id
 * @param[in] timestamp_uS
 */
void data_smpl_parse_throttling_exit_transition(uint8_t core_id, uint64_t timestamp_uS);

/**
 * @brief function reset the cores data after a collection window .
 *
 */
void data_smpl_reset_core_data(void);

/**
 * @brief function reset the soc data after a collection window .
 *
 */
void data_smpl_reset_soc_data(void);

/**
 * @brief function reset the tiles data after a collection window .
 *
 */
void data_smpl_reset_tile_data(void);
