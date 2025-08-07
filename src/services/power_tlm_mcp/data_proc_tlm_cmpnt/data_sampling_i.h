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

#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <fpfw_status.h>
#include <pvt_struct.h>
#include <sensor_fifo_service.h> // for sensor ram data structures
#include <stdint.h>
#include <telemetry_package_defs.h>

/*-- Symbolic Constant Macros (defines) --*/

#define MAX_BUFFER_ENTRIES 		 8
#define MAX_NUMBER_POWER_SAMPLE  100


#define DOUT2MILLIVOLTS(dout) ((uint16_t)(DOUT2VOLTS((dout)) * 1000.0f))
#define MILLIVOLTS2DOUT(mv) (VOLTS2DOUT(((mv) / 1000.0f)))

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

#define CSTATE_C0 (0)
#define CSTATE_C1 (1)
#define CSTATE_C2 (2)
#define CSTATE_C3 (3)
#define CSTATE_C4 (4)

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

/**
 *  @brief Core related runtime resources
 */

typedef struct {
    uint8_t pkt_pstate_is_valid : 1; // true if pstate is valid in the current packet
    uint8_t pkt_pstate_is_pending_invalid : 1; // set on the transition of pstate from valid to invalid, wrap up metrics
    uint8_t pkt_cstate_is_valid : 1; // true if cstate is valid in the current packet
} core_status_flags_t;

typedef struct {
    uint64_t cstate_res_timestamp_uS; // timestamp of last cstate residency metrics update
    uint64_t pstate_res_timestamp_uS; // timestamp of last pstate residency metrics update
    uint64_t pstate_pwr_res_timestamp_uS; // timestamp of last pstate power residency metrics update
    uint64_t latest_throttle_type_previous_timestamp_uS[NUMBER_OF_THROTTLE_TYPES];
    uint64_t latest_rack_priority_previous_timestamp_uS[NUMBER_OF_RACK_PRIORITIES];
    uint32_t time_counter_uS; // for general residency calculation for the core in uS
    uint16_t latest_throttle_type; /* This is throttle index or source, e.g throttle_source_t */
    uint16_t latest_voltage_mV;
    uint16_t latest_current_mA;
    uint16_t latest_power_mW;
    uint16_t latest_max_value_dC;
    uint16_t active_sample_mpam_id;
    uint8_t latest_cstate; /* cstate from pstate packet from sensor fifo*/
    uint8_t active_sample_plimit;
    uint8_t throttling_status;/* this is throttling status, e.g pstate_throttle_status_t */
    uint8_t throttle_event;
    uint8_t throttle_source;
    uint8_t latest_rack_throttling_priority_id;
    uint8_t pstate_from_pstate_pkt; /* pstate from pstate packet*/
    uint8_t pstate_from_current_pkt; /* pstate from current packet, during throttling */
    uint8_t latest_pstate; //either pstate_from_pstate_pkt or pstate_from_current_pkt
    bool core_throttling_tracker[NUMBER_OF_THROTTLE_TYPES];
    core_status_flags_t status_flags;//reset for every poll period
} core_runtime_info_t;

typedef struct {
    uint16_t latest_vcpu_voltage_mV;
    uint16_t latest_vsys_voltage_mV;
    uint16_t latest_max_temp_dC;
    uint8_t latest_max_temp_sensor_index;
} tile_runtime_info_t;

typedef struct {
    uint32_t soc_pc3_residency_mS;
    uint32_t soc_pc4_residency_mS;
    uint16_t latest_rail_temperature_dC[MAX_NUM_OF_VR_RAILS];
    uint16_t latest_rail_voltage_mV[MAX_NUM_OF_VR_RAILS];
    uint16_t latest_rail_current_mA[MAX_NUM_OF_VR_RAILS];
    uint16_t latest_hnf_max_temp_dC[NUMBER_OF_HNF_CHANNELS_PER_DIE];
    uint16_t latest_soc_top_temp_dC[NUMBER_OF_SOC_TEMP_SENSORS];
    uint16_t latest_max_tile_temp_dC;
    uint16_t latest_max_soc_top_temp_dC;
    uint16_t latest_max_die_temp_dC; // max of latest_max_tile_temp_dC and latest_max_soc_top_temp_dC
} soc_runtime_info_t;

typedef struct {
    inst_soc_element_dimm_runtime_t  latest_dimm[NUMBER_OF_DIMMS_PER_DIE];
    uint32_t latest_dimm_total_pwr_mW;
    uint16_t latest_max_dimm_temp_dC;
} dimm_runtime_info_t;
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

typedef struct {
    uint64_t cstate_time_diff_uS;
    uint64_t pstate_time_diff_uS;
    uint64_t throttle_time_diff_uS;
    uint64_t rack_throttle_time_diff_uS;
    uint8_t overrun_count_change;

    bool valid_entry_pstate;
    bool pstate_change;
    bool valid_entry_cstate;
    bool cstate_change;
    bool throttling_state_change;
    bool rack_throttling_state_change;
    bool throttle_start;
    bool rack_priority_start;
} core_state_entry_data_t;

/*-- Declarations (Statics and globals) --*/

extern core_runtime_info_t core_rt[NUMBER_OF_CORES_PER_DIE];
extern tile_runtime_info_t tile_rt[NUMBER_OF_TILES_PER_DIE];
extern soc_runtime_info_t soc_rt;
extern dimm_runtime_info_t dimm_rt;

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
 * @brief Process the tile temperature sensor FIFO.
 *
 * @return true if the FIFO was processed successfully, false otherwise.
 */
bool data_smpl_process_tile_temperature_sensor_fifo(void);

/**
 * @brief Process the tile voltage sensor FIFO.
 *
 * @return none
 */
void data_smpl_process_tile_voltage_sensor_fifo(void);

/**
 * @brief Process the core current sensor FIFO.
 *
 * @return none
 */
void data_smpl_process_pstate_sensor_fifo(void);

/**
 * @brief Process the core current sensor FIFO.
 *
 * @return none
 */
void data_smpl_process_core_current_sensor_fifo(void);

/**
 * @brief Process the voltage regulator (VR) temperature sensor FIFO.
 *
 * @return none
 */
void data_smpl_process_vr_temp_sensor_fifo(void);

/**
 * @brief Process the voltage regulator (VR) current sensor FIFO.
 *
 * @return none
 */
void data_smpl_process_vr_current_sensor_fifo(void);

/**
 * @brief Process the SOC PVT temperature sensor FIFO.
 *
 * @return true if the FIFO was processed successfully, false otherwise.
 */
bool data_smpl_process_pvt_temperature_sensor_fifo(void);

/**
 * @brief Process the DIMM sensor FIFO.
 *
 * @return true if the FIFO was processed successfully, false otherwise.
 */
bool data_smpl_process_dimm_sensor_fifo(void);

/**
 * @brief   Update the maximum die temperature based on the latest tile and SOC top temperatures.
 *          This function compares the latest maximum tile temperature and the latest maximum SOC top temperature,
 *          and updates the maximum die temperature accordingly.
 *          The maximum die temperature is the greater of the two values.
 * @param none
 */
void data_smpl_update_max_die_temp(void);

/**
 * @brief Internal API to log tile/core/hnf temperature parameters
 *
 * @param[in] tile_temp_entry - SCF RAM formatted resource for temperature packets
 * @param[in] tile_index - index to the tile id being referenced by the entry
 *
 * @return true if valid
 */
bool data_smpl_parse_tile_temperature_entry(tile_temp_t* tile_temp_entry, uint8_t tile_index);

/**
 * @brief Internal API to log tile/core voltages
 *
 * @param[in] tile_voltage_entry - SCF RAM formatted resource for voltage packets
 * @param[in] tile_index - index to the tile id being referenced by the entry
 *
 * @return true if valid
 */
bool data_smpl_parse_tile_voltage_entry(tile_voltage_t* tile_voltage_entry, uint8_t tile_index);

/**
 * @brief Internal API to log core currents and power
 *
 * @param[in] core_current_entry - SCF RAM formatted resource for core current packets
 * @param[in] core_index - index to the core id being referenced by the entry
 * @param[out] time_diff_uS - time difference in microseconds since the last entry for the core
 *
 * @return bool   - true if a valid current entry
 */
bool data_smpl_parse_core_current_entry(core_current_t* core_current_entry, uint8_t core_index, uint32_t* time_diff_uS);

/**
 * @brief Internal API to log voltage regulator (VR) temperatures
 *
 * @param[in] vr_temp_entry - SCF RAM formatted resource for VR Temperatures
 *
 * @return None
 */
void data_smpl_parse_vr_temperature_entry(vr_temp_t* vr_temp_entry);

/**
 * @brief Internal API to log voltage regulator (VR) current and voltage
 *
 * @param[in] vr_current_entry - SCF RAM formatted resource for VR Currents (and voltages)
 *
 * @return None
 */
void data_smpl_parse_vr_current_entry(vr_current_t* vr_current_entry);

/**
 * @brief Internal API to log soc pvt temperature (SOC_TOP_TEMP)
 *
 * @param[in] pvt_temp_entry - SCF RAM formatted resource for soc_pvt_temp
 *
 * @return None
 */
void data_smpl_parse_pvt_temperature_entry(soc_pvt_temp_t* pvt_temp_entry);

/**
 * @brief Internal API to log soc dimm information (DIMM)
 *
 * @param[in] dimm_info_entry - SCF RAM formatted resource for dimm
 *
 * @return true if valid
 */
bool data_smpl_parse_dimm_entry(sensor_ram_dimm_info_t* dimm_info_entry);

/**
 * @brief Internal API to log pstate telemetry packets, for pstate, cstate and throttling info
 *
 * @param[in] pstate_entry - SCF RAM formatted resource for pstate packets
 *        NOTE: The Pstate packet contains the core id reference internally.
 * @param[in] timestamp_uS - latest timestamp
 * @param[out] entry_data - update the core state entry data
 *
 * @return none
 */
void data_smpl_parse_pstate_no_throttling(pstate_telem_t* pstate_entry, uint64_t timestamp_uS, core_state_entry_data_t* entry_data);

/**
 * @brief Internal API to log cstate telemetry.
 * @param[in] cstate_telemetry - SCF RAM formatted resource for pstate packets
 *        (IMPORTANT : pstate telemetry packet provide both p state/c state))
 *        NOTE: The Pstate packet contains the core id reference internally.
 * @param[in] timestamp_uS - latest timestamp
 * @param[out] entry_data - update the cstate entry data
 * @return none
 */
void  data_smpl_parse_cstate(pstate_telem_t* cstate_telemetry, uint64_t timestamp_uS, core_state_entry_data_t* entry_data);

/**
 * @brief update rack throttling
 *
 * @param[in] pstate_entry  provide pstate tlm pkt.
 * @param[in] timestamp_uS - latest timestamp
 * @param[out] entry_data  update the core state entry data
 * @return  none
 */
void data_smpl_parse_rack_throttling(pstate_telem_t* pstate_entry, uint64_t timestamp_uS, core_state_entry_data_t* entry_data);

/**
 * @brief Internal API to log states-pstate,cstate and also log core throttling telemetry.
 * @param[in] pstate_entry - SCF RAM formatted resource for pstate packets
 *        (IMPORTANT : pstate telemetry packet provide both p state/c state and throttling status))
 * @param[in] entry_data - update the core state entry data
 * @return none
 */
void data_smpl_parse_core_states_entry(pstate_telem_t* pstate_entry,  core_state_entry_data_t* entry_data);

/**
 * @brief  calculate throttling index based on throttle status in telemetry pkt.
 *
 * @param[in] status  1-12 if success or -1 in case of a error.
 * @return int return index for throttle info array.
 */
int8_t data_smpl_parse_throttling_state_change_get_index_from_status(pstate_throttle_status_t status);

/**
 * @brief log the throttling states, based on pstate pkt and start/end event
 * from pkt.
 *
 * @param[in] pstate_entry - incoming packet
 * @param[in] timestamp_uS - latest timestamp
 * @param[out] entry_data - update the core state entry data
 * This function will update the core state entry data with the latest throttling state change.
 * @return none
 */
void data_smpl_parse_throttling_state_change(pstate_telem_t* pstate_entry, uint64_t timestamp_uS, core_state_entry_data_t* entry_data);

/**
 * @brief This api clear the throttling tracker.
 *
 * @param[in] core_id
 * @param[in] timestamp_uS
 */
void data_smpl_parse_throttling_state_change_exit_transition(uint8_t core_id, uint64_t timestamp_uS);

/**
 * @brief This API updates the average pstate for the polling period.
 *
 * @param  none
 */
void data_smpl_update_soc_avg_pstate(void);

/**
 * @brief When a power package is about to be generated, this function is called to finalize
 *  any residencies to align with the timestamp of the new power package.
 *
 * @param  none
 */
void data_smpl_finalize_pwr_pkg_metrics(void);

/**
 * @brief This API resets the residency timestamps for all cores.
 * It is called when telemetry package publishing is enabled to ensure that the timestamps are
 * aligned with the new power package. Existing timestamps are valid but no updates may have occurred for some time,
 * yet, latest state is active so residency should accumulate from this point.
 * @param
 */
void data_smpl_reset_residency_timestamps(void);

/**
 * @brief This API update throttling for single core when there is no pstate packet,
 * @param[in] core_id  current core
 * @param[in] timestamp_uS system timestamp
 */
void data_smpl_update_metrics_for_single_core_during_throttling(uint8_t core_id, uint64_t time_stamp_uS);

/**
 * @brief  This api is Rack throttling specific , when we dont get a pstate packet and we are in rack
 *          throttling already so this api update the compute metrics
 * @param core_id
 * @param time_stamp_uS  this is latest system timestamp.
 */
void data_smpl_update_metrics_for_single_core_during_rack_throttling(uint8_t core_id, uint64_t time_stamp_uS);

/**
 * @brief get currently active throttling for a core.
 *
 * @param[in] core_id
 * @return   currently active throttling for a core
 */
uint8_t  data_smpl_get_active_throttling_for_single_core(uint8_t core_id);

