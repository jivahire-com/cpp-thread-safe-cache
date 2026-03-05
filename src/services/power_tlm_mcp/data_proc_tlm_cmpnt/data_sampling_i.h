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
#include <die_2_die_exchange_i.h>
#include <fpfw_status.h>
#include <pvt_struct.h>
#include <sensor_fifo_service.h> // for sensor ram data structures
#include <stdint.h>
#include <telemetry_package_defs.h>

/*-- Symbolic Constant Macros (defines) --*/

#define MAX_BUFFER_ENTRIES 		 8
#define MAX_NUMBER_POWER_SAMPLE  100


#define DOUT2MILLIVOLTS(dout) ((uint16_t)(DOUT2VOLTS((dout)) * 1000.0f))
#define MILLIVOLTS2DOUT(mv) ((uint16_t)(VOLTS2DOUT(((mv) / 1000.0f))))

#define MICROSECONDS_TO_MILLISECONDS(time_diff_uS) ((time_diff_uS) / 1000)


// The current conversion factor is set by default as 32.2 per bit.
#ifndef CORE_CURRENT_CONVERSION_FACTOR
    #define CORE_CURRENT_CONVERSION_FACTOR 32.2F
#endif
// Power management HAS V1.04 section 20.1.4.1.2, the power conversion factor is set by default as 32mW per bit.
// The power conversion factor is set by default as 32mW per bit.
#ifndef CORE_POWER_MW_PER_BIT
    #define CORE_POWER_MW_PER_BIT 32
#endif

#define ROUND_USEC_TO_MSEC(usec) ((usec + 500) / 1000)
#define ROUND_NSEC_TO_USEC(nsec) ((nsec + 500) / 1000)//nano to micro second rounding

#define CSTATE_C0 (0)
#define CSTATE_C1 (1)
#define CSTATE_C2 (2)
#define CSTATE_C3 (3)
#define CSTATE_C4 (4)

#define DIE_MESH_FREQ_HZ (2000000000ULL) // 2 GHz
#define MESH_PWR_TLM_INTERVAL(seconds) ((seconds) * DIE_MESH_FREQ_HZ)
//Mesh telemetry interval is based on a 2 GHz PLL reference clock (Reference KNG power management HAS v1.04 Section 11.5)
// Recommended interval is  count =  1 s × 2 GHz = 2,000,000,000 
#define PER_DIE_MESH_PWR_TLM_INTERVAL MESH_PWR_TLM_INTERVAL(1ULL) //Enable mesh telemetry with 1 s interval count

// Convert counter value to milliseconds based on reference clock frequency
#define MESH_COUNTER_TO_MS(counter) \
    ( ((double)(counter) / (double)(DIE_MESH_FREQ_HZ)) * 1000.0 )

// Convert D2DSS counter value to milliseconds, D2DSS sync with reference clock of DIE MESH
#define D2DSS_COUNTER_TO_MS(counter) \
    ( ((double)(counter) / (double)(DIE_MESH_FREQ_HZ)) * 1000.0 )

//Every Flit count is 64 bytes for kingsgate
#define D2DSS_FLIT_COUNT_TO_BYTES(count) ((count) * 64)
 
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
 * @brief D2D Link ID enumeration
 * 
 * Defines the link state identifiers for D2DSS interfaces.
 * Each D2DSS interface supports 3 link states (L0, L0s, L1).
 */
typedef enum
{
    D2D_LINK_L0 = 0,    // L0 - Active state with full bandwidth
    D2D_LINK_L0S,       // L0s - Low power state with reduced bandwidth
    D2D_LINK_L1,        // L1 - Sleep state with no bandwidth
    D2D_LINK_MAX
} d2d_link_t;

//Die-to-Die Subsystem  MAS v1p0 Section 5.8, Performance monitoring Unit (PMU) -counters
//IP Name : Debug wrapper Sub-IP 
//Ref: https://microsoft.sharepoint.com/:w:/r/teams/EchoFalls/_layouts/15/Doc.aspx?sourcedoc=%7B179CCE4D-5CCE-4087-9E7F-AB4514BDFE03%7D&file=Kingsgate%20D2D%20MAS%20v1p0.docx&wdOrigin=TEAMS-MAGLEV.undefined_ns.rwc&action=default&mobileredirect=true
typedef enum
{
    D2DSS_SOURCE_CNTR_RX_PHY_L0S_RESIDENCY = 0, 
    D2DSS_SOURCE_CNTR_RX_PHY_L1_RESIDENCY,//on die
    D2DSS_SOURCE_CNTR_TX_PHY_L0S_RESIDENCY,
    D2DSS_SOURCE_CNTR_TX_PHY_L1_RESIDENCY,//off die
    D2DSS_SOURCE_CNTR_TX_PHY_L0_RESIDENCY,
    D2DSS_SOURCE_CNTR_TX_PHY_FLIT_COUNT,
    D2DSS_SOURCE_CNTR_RX_PHY_L0_RESIDENCY,
    D2DSS_SOURCE_CNTR_RX_PHY_FLIT_COUNT
} d2d_source_counter_t;

/**
 *  @brief Core related runtime resources
 */
typedef struct {
    uint8_t pkt_pstate_is_valid : 1; // true if pstate is valid in the current packet
    uint8_t pkt_pstate_is_pending_invalid : 1; // set on the transition of pstate from valid to invalid, wrap up metrics
    uint8_t pkt_cstate_is_valid : 1; // true if cstate is valid in the current packet
    uint8_t throttle_is_active : 1; // true if 1 or more throttling types are active, including rack throttling
    uint8_t rack_throttle_is_active : 1; // only true if rack throttling is active
    uint8_t reserved : 3;
} core_status_flags_t;

typedef struct {
    uint64_t cstate_res_timestamp_uS; // timestamp of last cstate residency metrics update
    uint64_t pstate_res_timestamp_uS; // timestamp of last pstate residency metrics update
    uint64_t throttle_res_timestamp_uS[NUMBER_OF_THROTTLE_SOURCES];
    uint64_t rack_pri_res_timestamp_uS[NUMBER_OF_RACK_THROTTLE_PRIORITIES];
    uint64_t latest_cstate_entry_latency_uS;
    uint64_t latest_cstate_exit_latency_uS;
    uint64_t latest_cstate_exit_timestamp_uS;
    uint16_t latest_voltage_mV;
    uint16_t latest_vcpu_voltage_mV; // for droop count record
    uint16_t latest_vmin_mV; // Vmin value for this core from SCP
    uint16_t latest_current_mA;
    uint16_t latest_power_mW;
    uint16_t latest_max_value_dC;
    uint8_t latest_mpam_id;
    uint8_t latest_cstate;  //cstate from pstate packet from sensor fifo
    uint8_t latest_pstate; //either pstate_from_pstate_pkt or pstate_from_current_pkt
    uint8_t nominal_pstate; // used to determine throttling extent
    uint8_t latest_rack_throttle_priority;
    uint8_t latest_plimit;
    uint8_t pstate_from_pstate_pkt; //pstate from pstate packet
    uint8_t pstate_from_current_pkt; //pstate from current packet, during throttling
    bool throttle_source_tracker[NUMBER_OF_THROTTLE_SOURCES];
    core_status_flags_t status_flags;
} core_runtime_info_t;

typedef struct {
    uint16_t latest_max_temp_dC;
    uint8_t latest_max_temp_sensor_index;
} tile_runtime_info_t;

typedef struct {
    uint32_t soc_pc3_residency_mS;
    uint32_t soc_pc4_residency_mS;
    pkg_mon_data_t die1_pkg_mon; // used only for Die 1 package monitor data for die 0 to read
    uint32_t latest_rail_current_mA[MAX_NUM_OF_VR_RAILS];
    uint16_t latest_rail_temperature_dC[MAX_NUM_OF_VR_RAILS];
    uint16_t latest_rail_voltage_mV[MAX_NUM_OF_VR_RAILS];
    uint16_t latest_hnf_max_temp_dC[NUMBER_OF_HNF_CHANNELS_PER_DIE];
    uint16_t latest_soc_top_temp_dC[NUMBER_OF_SOC_TEMP_SENSORS];
    uint16_t latest_max_tile_temp_dC;
    uint16_t latest_max_soc_top_temp_dC;
    uint16_t latest_max_die_temp_dC; // max of latest_max_tile_temp_dC and latest_max_soc_top_temp_dC
    uint8_t latest_pkg_cstate;
} soc_runtime_info_t;

typedef struct {
    inst_soc_element_dimm_runtime_t  latest_dimm[NUMBER_OF_DIMMS_PER_DIE];
    uint32_t latest_dimm_total_pwr_mW;
    uint16_t latest_max_dimm_temp_dC;
    dimm_throttle_source_t latest_throttle_source;
} dimm_runtime_info_t;

typedef struct {
    uint8_t active : 1; // true if the mpam is currently active
    uint8_t throttling : 1; // set if the mpam is actively throttling
    uint8_t reserved : 6;
} mpam_status_flags_t;

typedef struct {
    uint64_t residency_timestamp_uS; // timestamp of last cstate residency metrics update
    uint8_t nominal_pstate;
    mpam_status_flags_t status_flags;
} mpam_runtime_info_t;

typedef struct {
    uint64_t latest_tx_res_counter[NUMBER_OF_D2D_LINKS_STATE];
    uint64_t latest_rx_res_counter[NUMBER_OF_D2D_LINKS_STATE];
    uint64_t latest_bw_tx_flit_counter[NUMBER_OF_D2D_LINKS_STATE];
    uint64_t latest_bw_rx_flit_counter[NUMBER_OF_D2D_LINKS_STATE];
} d2dss_runtime_info_t;

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

// NOTE: this is a temporary structure used to process pstate sensor fifo entries,
// the fifo entry contains a great deal of information and multiple functions parse
// specific parts of the entry. This structure is used to pass the parsed data
// to the functions that process the pstate/cstate/throttle input data..
// It is not used to store the data in the core_rt[] structure.
// The data is used to determine which metrics to update and the data required to update them.
typedef struct {
    uint64_t cstate_time_diff_uS;
    uint64_t pstate_time_diff_uS;
    uint64_t throttle_time_diff_uS;
    uint64_t rack_throttle_time_diff_uS;
    uint64_t packet_timestamp_uS;
    uint8_t overrun_count_change;
    throttle_source_t throttle_source;

    bool valid_entry_pstate;
    bool pstate_change;
    bool valid_entry_cstate;
    bool cstate_change;
    bool throttling_state_change;
    bool rack_throttling_state_change;
    bool throttle_start;
    bool rack_priority_start;
} core_state_entry_data_t;
//ref :https://dev.azure.com/ms-tsd/Kingsgate/_git/silibs?path=/soc_fw_shared/include/mcp_telemetry_shared.h&version=GBdev_a0&line=22&lineEnd=22&lineStartColumn=5&lineEndColumn=102&lineStyle=plain&_a=contents
typedef enum
{
    CSTATE_ENTER_PSCI = 0,   // First thing done when entering Sleep (C2/C3) - ENTER Cstate (1)
    CSTATE_EXIT_PSCI,        // Last thing done from Wake up (C2/C3)         - EXIT Cstate  (2)
    CSTATE_ENTER_HW_LOW_PWR, // Last thing done when entering Sleep          - ENTER Cstate (4)
    CSTATE_EXIT_HW_LOW_PWR,  // First thing done when exiting sleep (HW wakes core, FW starts) - EXIT State (1)
    CSTATE_ENTER_CFLUSH, // Only when entering Cstate, in between ENTER_PSCI & ENTER_LOW_PWR  - ENTER Cstate (2)
    CSTATE_EXIT_CFLUSH,  // Only when exiting Cstate,  in between ENTER_PSCI & ENTER_LOW_PWR  - EXIT Cstate  (3)

    CSTATE_MAX_ID
} cstate_tfa_timestamp_id_t;

/*-- Declarations (Statics and globals) --*/

extern core_runtime_info_t core_rt[NUMBER_OF_CORES_PER_DIE];
extern tile_runtime_info_t tile_rt[NUMBER_OF_TILES_PER_DIE];
extern soc_runtime_info_t soc_rt;
extern dimm_runtime_info_t dimm_rt;
extern mpam_runtime_info_t mpam_rt[NUMBER_OF_MPAMS];
extern mpam_data_t mpam_staging[NUMBER_OF_MPAMS];

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
 * @brief Process Aging counters data read from core cluster
 * @param[in] core_id
 * @param[in] this_pwr_pkg_timestamp_uS timestamp when we are creating package.
 * @return none
 */
void data_smpl_process_aging_data(uint8_t core_id, uint64_t this_pwr_pkg_timestamp_uS);

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
 * @param[in] core_id - index to the core id being referenced by the entry
 *
 * @return bool   - true if a valid current entry
 */
bool data_smpl_parse_core_current_entry(core_current_t* core_current_entry, uint8_t core_id);

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
 * @param[out] entry_data - update the core state entry data
 *
 * @return none
 */
void data_smpl_parse_pstate(pstate_telem_t* pstate_entry, core_state_entry_data_t* entry_data);

/**
 * @brief Internal API to log cstate telemetry.
 * @param[in] cstate_telemetry - SCF RAM formatted resource for pstate packets
 *        (IMPORTANT : pstate telemetry packet provide both p state/c state))
 *        NOTE: The Pstate packet contains the core id reference internally.
 * @param[out] entry_data - update the cstate entry data
 * @return none
 */
void  data_smpl_parse_cstate(pstate_telem_t* cstate_telemetry, core_state_entry_data_t* entry_data);

/**
 * @brief Internal API to log states-pstate,cstate and also log core throttling telemetry.
 * @param[in] pstate_entry - SCF RAM formatted resource for pstate packets
 *        (IMPORTANT : pstate telemetry packet provide both p state/c state and throttling status))
 * @param[in] entry_data - update the core state entry data
 * @return none
 */
void data_smpl_parse_core_states_entry(pstate_telem_t* pstate_entry,  core_state_entry_data_t* entry_data);

/**
 * @brief This api clear the throttling tracker.
 *
 * @param[in] core_id
 * @param[in] timestamp_uS
 */
void data_smpl_terminate_non_rack_throttle_sources(uint8_t core_id, uint64_t* timestamp_uS);

/**
 * @brief This API updates the average pstate for the polling period.
 *
 * @param  none
 */
void data_smpl_update_soc_avg_pstate(void);

/**
 * @brief This API updates the core histogram data.
 *
 * @param  none
 */
void data_smpl_update_core_histogram(void);

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
 * @brief This API completes the throttling metrics at the end of a packaging interval.
 * Updates residencies for active throttling sources
 * @param[in] core_id  current core
 * @param[in] this_pwr_pkg_timestamp_uS system timestamp of power package being generated to finalize package residency
 */
void data_smpl_finalize_pwr_pkg_throttling_metrics(uint8_t core_id, uint64_t* this_pwr_pkg_timestamp_uS);

/**
 * @brief get currently active throttling for a core.
 *
 * @param[in] core_id
 * @return   currently active throttling for a core
 */
uint8_t  data_smpl_get_active_throttle_sources(uint8_t core_id);

/**
 * @brief Handle the start of a throttle event.
 *
 * @param[in] core_id - The ID of the core where the throttle event occurred.
 * @param[in] throttle_source - The source of the throttle event.
 * @param[out] entry_data - Update the core state entry data with the throttle start information.
 */
void data_smpl_handle_throttle_source_start(uint8_t core_id, throttle_source_t throttle_source, core_state_entry_data_t* entry_data);

/**
 * @brief Handle the end of a throttle event.
 *
 * @param[in] core_id - The ID of the core where the throttle event occurred.
 * @param[in] throttle_source - The source of the throttle event.
 * @param[out] entry_data - Update the core state entry data with the throttle end information.
 */
void data_smpl_handle_throttle_source_end(uint8_t core_id, throttle_source_t throttle_source, core_state_entry_data_t* entry_data);

/**
 * @brief Handle the start of a rack throttle event.
 *
 * @param[in] core_id - The ID of the core where the rack throttle event occurred.
 * @param[in] new_priority - The new priority for the rack throttle event.
 * @param[out] entry_data - Update the core state entry data with the rack throttle start information.
 */
void data_smpl_handle_rack_throttle_start(uint8_t core_id,
                                              uint8_t new_priority,
                                              core_state_entry_data_t* entry_data);

/**
 * @brief Handle the end of a rack throttle event.
 *
 * @param[in] core_id - The ID of the core where the rack throttle event occurred.
 * @param[out] entry_data - Update the core state entry data with the rack throttle end information.
 */
void data_smpl_handle_rack_throttle_end(uint8_t core_id, core_state_entry_data_t* entry_data);

/**
 * @brief update aging counter metrics for all cores.
 *
 * @return   none
 */
void data_smpl_update_metrics_for_cores_aging_counters(void);

/**
 * @brief Calculate MPAM power for all MPAMs based on the latest core power and MPAM ID.
 *
 * @return none
 */
void data_smpl_calculate_mpam_data_from_cores();

/**
 * @brief Get the TFA timestamp for a specific core and timestamp ID.
 *
 * @param[in] core_id - The ID of the core.
 * @param[in] timestamp_id - The ID of the timestamp to retrieve.
 * @return The TFA timestamp in microseconds.
 */
uint64_t data_smpl_get_cstate_tfa_timestamp(uint8_t core_id, cstate_tfa_timestamp_id_t timestamp_id);
/**
 * @brief Process the C-state entry latency for a specific core and C-state.
 *
 * @param[in] core_id - The ID of the core.
 * @param[in] packet_cstate - The C-state from the packet.
 * @param[in] packet_timestamp_uS - The timestamp from the packet in microseconds.
 */
void data_smpl_process_cstate_entry_latency(uint8_t core_id, uint8_t packet_cstate, uint64_t packet_timestamp_uS);

/**
 * @brief Process the C-state exit latency for a specific core.
 *
 * @param[in] core_id - The ID of the core.
 */
void data_smpl_process_cstate_exit_latency(uint8_t core_id);

/**
 * @brief Update the mesh telemetry metrics. This function is called based on the programmed interval.
 *
 * @return None
 */
void data_smpl_update_metrics_for_per_die_mesh_counters(void);

/**
 * @brief Initialize the die mesh telemetry.
 *
 * @return None
 */
void data_smpl_die_mesh_tlm_init(void);

/**
 * @brief Reset the die mesh telemetry.
 *
 * @return None
 */
void data_smpl_die_mesh_tlm_reset(void);

/**
 * @brief Initialize D2DSS PMU counters for all interfaces and required events.
 * Configures PMU counters for D2D link state telemetry collection.
 * 
 * @return none
 */
void data_smpl_init_d2dss_pmu_counters(void);

/**
 * @brief Read D2DSS PMU counter for a specific interface and event.
 * 
 * @param[in] interface_id - D2DSS interface ID (0-7)
 * @param[in] event_number - PMU event counter number (0-7)
 * @param[out] counter_value - Pointer to store the 64-bit counter value
 * @return true if the read was successful, false otherwise.
 */
bool data_smpl_read_d2dss_pmu_counter(uint8_t interface_id, uint8_t event_number, uint64_t* counter_value);

/**
 * @brief Read D2DSS L0 link state status for a specific interface.
 * This function reads L0 residency and bandwidth counters to determine L0 state status.
 * 
 * @param[in] interface_id - D2DSS interface ID (0-7)
 * @param[out] tx_res_diff - Pointer to receive TX residency counter difference
 * @param[out] rx_res_diff - Pointer to receive RX residency counter difference
 * @param[out] bw_tx_diff - Pointer to receive TX bandwidth flit counter difference
 * @param[out] bw_rx_diff - Pointer to receive RX bandwidth flit counter difference
 * @return true if L0 state is active/detected, false otherwise
 */
bool data_smpl_read_d2dss_l0_state(uint8_t interface_id, uint64_t* tx_res_diff, uint64_t* rx_res_diff, uint64_t* bw_tx_diff, uint64_t* bw_rx_diff);

/**
 * @brief Read D2DSS L0s link state status for a specific interface.
 * This function reads L0s residency counters to determine L0s state status.
 * 
 * @param[in] interface_id - D2DSS interface ID (0-7)
 * @param[out] tx_res_diff - Pointer to receive TX residency counter difference
 * @param[out] rx_res_diff - Pointer to receive RX residency counter difference
 * @return true if L0s state is active/detected, false otherwise
 */
bool data_smpl_read_d2dss_l0s_state(uint8_t interface_id, uint64_t* tx_res_diff, uint64_t* rx_res_diff);

/**
 * @brief Read D2DSS L1 link state status for a specific interface.
 * This function reads L1 residency counters to determine L1 state status.
 * 
 * @param[in] interface_id - D2DSS interface ID (0-7)
 * @param[out] tx_res_diff - Pointer to receive TX residency counter difference
 * @param[out] rx_res_diff - Pointer to receive RX residency counter difference
 * @return true if L1 state is active/detected, false otherwise
 */
bool data_smpl_read_d2dss_l1_state(uint8_t interface_id, uint64_t* tx_res_diff, uint64_t* rx_res_diff);

/**
 * @brief Process D2DSS link telemetry data for all interfaces
 *
 * This function processes telemetry data for all 8 D2DSS interfaces (0-7)
 * and aggregates the data using compute metrics.
 */
void data_smpl_update_metrics_for_d2d_link_telemetry(void);

/**
 * @brief Reset D2DSS link telemetry counters for all interfaces.
 * This function resets all PMU counters and telemetry data for D2DSS interfaces.
 * 
 * @return none
 */
void data_smpl_d2dss_link_tlm_reset(void);

/**
 * @brief Process PC3 and PC4 states.
 *
 * This function processes the telemetry data related to PC3 and PC4 states.
 */
void data_smpl_process_package_cstates(void);

/* * 
 * @brief Update SOC package C-state residency metrics.
 *
 * This function updates the SOC package C-state residency metrics based on the latest telemetry data.
 */
void data_smpl_update_soc_package_cstate(void);

/**
 * @brief Get the secondary die package C-state.
 *
 */
void data_smpl_get_secondary_die_package_cstate(void);

/**
 * @brief Update MPAM memory power metrics.
 *
 */
void data_smpl_update_mpam_mem_power(void);

/**
 * @brief Read Vmin value from SCP and populate the core runtime info structure.
 *
 * This function reads the Vmin value for each core , written by SCP via core exchange, 
 * and updates the corresponding field in the core runtime info structure.
 */
void data_smpl_read_and_populate_core_vmin(void);

/**
 * @brief Calculate memory power based on total memory power in mW and update MPAM data.
 *
 * @param[in] total_memory_power_mW - The total memory power in milliwatts.
 */
void data_sample_calculate_vm_memory_power(uint32_t total_memory_power_mW);
