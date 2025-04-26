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
    voltage_t voltage;
    current_t current;
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
    pwr_soc_element_dimm_t      dimm[NUMBER_OF_DIMM_MODULES];
} soc_runtime_info_t;

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

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize the dts coeff data structure.
 *
 * @return none
 */
void  tlm_logger_init_fuse_dts_coeff_data(void);

/**
 * @brief Initialize the static information, like cstate id, pstate, etc.. 
 * 
 * @return none
 */
void tlm_logger_init_constant_data();

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
 * @brief Internal API to log soc pvt temperature (SOC_TOP_TEMP) 
 *
 * @param soc_pvt_temp_t - SCF RAM formatted resource for soc_pvt_temp
 *
 * @return None
 */
void tlm_logger_log_soc_pvt_temp(soc_pvt_temp_t* pvt_temperature);
/**
 * @brief Internal API to log soc dimm information (DIMM) 
 *
 * @param sensor_ram_dimm_info_t - SCF RAM formatted resource for dimm
 *
 * @return fpfw_status_t
 */
fpfw_status_t telmain_log_dimm_info(sensor_ram_dimm_info_t* dimm_info);
/**
 * @brief Internal API to log pstate telemetry packets, for pstate, cstate and throttling info
 *
 * @param pstate_telemetry - SCF RAM formatted resource for pstate packets
 *        NOTE: The Pstate packet contains the core id reference internally.
 *
 * @return fpfw_status_t
 */
fpfw_status_t tlm_logger_log_core_pstate(pstate_telem_t* pstate_telemetry);
/**
 * @brief Internal API to log cstate telemetry.
 * @param cstate_telemetry - SCF RAM formatted resource for pstate packets
 *        (IMPORTANT : pstate telemetry packet provide both p state/c state))
 *        NOTE: The Pstate packet contains the core id reference internally.
 * @return fpfw_status_t
 */
fpfw_status_t  tlm_logger_log_core_cstate(pstate_telem_t* cstate_telemetry);
/**
 * @brief Internal API to log states-pstate,cstate and also log core throttling telemetry.
 * @param pstate_telemetry - SCF RAM formatted resource for pstate packets
 *        (IMPORTANT : pstate telemetry packet provide both p state/c state and throttling status))
 *        
 * @return fpfw_status_t
 */
fpfw_status_t  tlm_logger_log_core_states(pstate_telem_t* pstate_telemetry);
/**
 * @brief  calculate throttling index based on throttle status in telemetry pkt.
 * 
 * @param status  1-12 if success or -1 in case of a error.
 * @return int return index for throttle info array.
 */
int8_t tlm_logger_calculate_throttle_index(pstate_throttle_status_t status);
/**
 * @brief log the throttling states, based on pstate pkt and start/end event 
 * from pkt.
 * 
 * @param pstate_telemetry 
 * @return fpfw_status_t None
 */
fpfw_status_t tlm_logger_log_core_throttling(pstate_telem_t* pstate_telemetry);
/**
 * @brief This api clear the throttling tracker.
 * 
 * @param core_id 
 * @param timestamp_uS 
 */
void tlm_update_throttling_status_on_exit(uint8_t core_id, uint64_t timestamp_uS);
/**
 * @brief Power telemetry update management -update data after logging. 
 * @param   None 
 * @return  None 
 */
void data_proc_tlm_cmpnt_aggregate_update_mgr(void);
/**
 * @brief tlm_calculate_mma_res function calculates the minimum, maximum, and average values of a 
 *          given metric over a specified time period. It updates the provided pointers with the 
 *          calculated results based on the latest value and the time difference.
 * 
 * @param mma_min -Pointer to the variable that stores the minimum value of the metric.
 *                 This value will be updated based on the latest value and the time difference.
 * @param mma_max -Pointer to the variable that stores the maximum value of the metric.
 * @param mma_average -Pointer to the variable that stores the average value of the metric. 
 * @param mma_latest_value  -Pointer to the variable that stores the latest value of the metric
 * @param time_diff_uS - The time difference in microseconds between the current and previous measurements
 * @param residency_uS The total residency time in microseconds over which the average is calculated
 */
void tlm_calculate_mma_res(uint16_t* mma_min, uint16_t* mma_max, uint16_t* mma_average, uint16_t* mma_latest_value, uint32_t time_diff_uS, uint32_t residency_uS);
/**
 * @brief  function updates the minimum, maximum, and average current values for a specified core based on the provided 
 *         time difference and residency time
 * 
 * @param core_id  core for which the current values are being updated.
 * @param time_diff_uS  time_diff_uS: The time difference in microseconds between the current and previous measurements.
 * @param residency_uS  The total residency time in microseconds over which the average is calculated.
 */
void tlm_update_core_current(uint8_t core_id, uint32_t time_diff_uS, uint32_t residency_uS);
/**
 * @brief    function updates the minimum, maximum, and average voltage values for a specified core based 
 *           on the provided time difference and residency time
 * 
 * @param core_id  core for which the current values are being updated.
 * @param time_diff_uS  time_diff_uS: The time difference in microseconds between the current and previous measurements.
 * @param residency_uS  The total residency time in microseconds over which the average is calculated.
 */
void tlm_update_core_voltage(uint8_t core_id, uint32_t time_diff_uS, uint32_t residency_uS);
/**
 * @brief    function updates the minimum, maximum, and average temperature values for 
 *           a specified core based on the provided time difference and residency time
 * 
 * @param core_id  core for which the current values are being updated.
 * @param time_diff_uS  time_diff_uS: The time difference in microseconds between the current and previous measurements.
 * @param residency_uS  The total residency time in microseconds over which the average is calculated.
 */
void tlm_update_core_temperature(uint8_t core_id, uint32_t time_diff_uS, uint32_t residency_uS);
/**
 * @brief    function updates the minimum, maximum, and average voltage values for the 
 *           vCPU of a specified tile based on the provided time difference and residency time
 * 
 * @param core_id  core for which the current values are being updated.
 * @param time_diff_uS  time_diff_uS: The time difference in microseconds between the current and previous measurements.
 * @param residency_uS  The total residency time in microseconds over which the average is calculated.
 */
void tlm_update_tile_vcpu(uint8_t tile_id, uint32_t time_diff_uS, uint32_t residency_uS);
/**
 * @brief    function updates the minimum, maximum, and average voltage values for the 
 *           vSYS of a specified tile based on the provided time difference and residency time
 * 
 * @param core_id  core for which the current values are being updated.
 * @param time_diff_uS  time_diff_uS: The time difference in microseconds between the current and previous measurements.
 * @param residency_uS  The total residency time in microseconds over which the average is calculated.
 */
void tlm_update_tile_vsys(uint8_t tile_id, uint32_t time_diff_uS, uint32_t residency_uS);
/**
 * @brief function updates the minimum, maximum, and average voltage values for 
 *         a specified SOC VR (Voltage Regulator) rail based on the provided time difference and residency time
 * 
 * @param vr_index The identifier of the VR rail for which the voltage values are being updated.
 * @param time_diff_uS 
 * @param residency_uS 
 */
void tlm_update_soc_rails_voltage(uint8_t vr_index, uint32_t time_diff_uS, uint32_t residency_uS);
/**
 * @brief  function updates the minimum, maximum, and average current values for 
 *          a specified SOC VR (Voltage Regulator) rail based on the provided time difference and residency time
 * 
 * @param vr_index 
 * @param time_diff_uS 
 * @param residency_uS 
 */
void tlm_update_soc_rails_current(uint8_t vr_index, uint32_t time_diff_uS, uint32_t residency_uS);
/**
 * @brief  function updates the minimum, maximum, and average temperature values for 
 *         a specified SOC VR (Voltage Regulator) rail based on the provided time difference and residency time
 * 
 * @param vr_index - The identifier of the VR rail for which the voltage values are being updated.
 * @param time_diff_uS 
 * @param residency_uS 
 */
void tlm_update_soc_rails_temperature(uint8_t vr_index, uint32_t time_diff_uS, uint32_t residency_uS);
/**
 * @brief function updates the minimum, maximum, and average temperature values for 
 *        a specified SOC HNF  based on the provided time difference and residency time
 * 
 * @param hnf_index The identifier of the HNF for which the temperature values are being updated.
 * @param time_diff_uS  The time difference in microseconds between the current and previous measurements
 * @param residency_uS  The total residency time in microseconds over which the average is calculated.
 */
void tlm_update_soc_hnf_temperature(uint8_t hnf_index, uint32_t time_diff_uS, uint32_t residency_uS);
/**
 * @brief function updates the minimum, maximum, and average temperature values for 
 *        a specified SOC PVT  sensor based on 
 *        the provided time difference and residency time.
 * @param pvt_index  The identifier of the PVT sensor for which the temperature values are being updated.
 * @param time_diff_uS The time difference in microseconds between the current and previous measurements
 * @param residency_uS The total residency time in microseconds over which the average is calculated.
 */
void tlm_update_soc_pvt_temperature(uint8_t pvt_index, uint32_t time_diff_uS, uint32_t residency_uS);
/**
 * @brief  function updates the minimum, maximum, and average temperature values for a specified 
 * SOC DIMM (Dual In-line Memory Module) based on the provided time difference and residency time. 
 * It updates the temperature data for both S0 and S1 sensors of the DIMM module. It utilizes 
 * the tlm_calculate_mma_res function to perform the calculations.
 * 
 * @param dimm_module_index The identifier of the DIMM module for which the temperature values are being updated.
 * @param time_diff_uS -The time difference in microseconds between the current and previous measurements.
 * @param residency_uS  -The total residency time in microseconds over which the average is calculated.
 */
void tlm_update_soc_dimm_info(uint8_t dimm_module_index, uint32_t time_diff_uS, uint32_t residency_uS);
/**
 * @brief The tlm_update_pstate function updates the power state (PState) residency and power metrics 
 *          for a specified core based on the provided timestamp. It handles both throttling and 
 *          non-throttling scenarios and updates the minimum, maximum, and average power values 
 *          for the current PState.
 * 
 * @param core_id -The identifier of the core for which the PState is being updated.
 * @param time_stamp_uS -The current timestamp in microseconds.
 */
void tlm_update_pstate(uint8_t core_id, uint64_t time_stamp_uS);
/**
 * @brief  This API used during cores are throttling, MMA calculation is triggerd by this APIs
 * 
 * @param core_id 
 * @param throttle_index  Throttling type.
 * @param time_diff_uS    time diff between current timestamp and previous time stamp
 * @param residency_mS  - residency in mS
 */
void tlm_update_throttling_pstate(uint8_t core_id, int8_t throttle_index, uint32_t time_diff_uS, uint32_t residency_mS);
/**
 * @brief update rack throttling 
 * 
 * @param pstate_telemetry  provide pstate tlm pkt.
 * @param throttle_index - source of throttle , in this case rack.
 * @param core_id 
 */
void tlm_update_rack_throttling(pstate_telem_t* pstate_telemetry, int throttle_index, uint8_t core_id);
/**
 * @brief tlm_update_throttling from runtime update manager.
 * 
 * @param core_id 
 * @param time_stamp_uS -system time stamp
 */
void tlm_update_throttling(uint8_t core_id, uint64_t time_stamp_uS);
/**
 * @brief function is intended to update the histogram data for a specified core
 * 
 * @param core_id 
 */
void tlm_update_core_histogram(uint8_t core_id);
/**
 * @brief The tlm_soc_component_update function updates various SOC (System on Chip) components, 
 *         including VR (Voltage Regulator) rails, HNF temperatures, PVT (Process, Voltage, Temperature) sensors
 *         and DIMM (Dual In-line Memory Module) temperatures. It calculates the time difference 
 *         since the last update and uses this information to update the residency and metrics for each component.
 */
void tlm_soc_component_update(void);
/**
 * @brief function updates various metrics for all cores, including PState residency and power, 
 * CState residency, throttling status, core current, voltage, and temperature. It calculates the 
 * time difference since the last update and uses this information to update the residency and metrics for each core.
 */
void tlm_core_component_update(void);
/**
 * @brief function updates various metrics for all tiles, including the maximum tile temperature, 
 * vCPU voltage, and system voltage (Vsys). It calculates the time difference since the last update 
 * and uses this information to update the residency and metrics for each tile.
 */
void tlm_tile_component_update(void);
/**
 * @brief Reset the core throttle data for the specified core.
 *
 * @param[in] core_id - The core id to reset the throttle data to. 0 
 *
 * @return None
 */
void tlm_core_reset_throttle_data(uint16_t core_id);
#ifdef __cplusplus
}
#endif