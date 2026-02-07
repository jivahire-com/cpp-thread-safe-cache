//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_runconfig.h
 * Header containing definitions for power service runtime configuration structures (static, knobs, fuse, etc)
 */

#pragma once

/*----------- Nested includes ------------*/
// clang-format off
#include <kng_soc_constants.h>
// clang-format on
#include <corebits.h>
#include <dvfs_struct.h>
#include <fpfw_cfg_mgr.h>
#include <pvt_struct.h>
#include <scp_avs_driver.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/
/* static config macros*/

#define BITS_PER_BYTE  CHAR_BIT


#define NUM_SOC_VM  18
#define NUM_TILE_VM 4

/* plimit macros */
#define MAX_PLIMIT (NUM_PSTATES - 1) // MAX_PLIMIT is lowest perf level
#define MIN_PLIMIT (0)               // MIN_PLIMIT is highest perf level

/* TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491015/
   Confirm power fuse counts */

/* number of VCPU leakage expected fuse points */
#define VCPU_LEAKAGE_COUNT 7
/* number of VCPU dynamic ldo expected fuse points */
#define VCPU_LDO_DYNAMIC_COUNT 2
/* number of core Cdyn expected fuse points */
#define CORE_CDYN_COUNT 3

/* number of VFT curvesets */
#define VFT_CURVE_COUNT_PER_CURVESET 4
#define VFT_CURVESET_COUNT           7

/* number of power samples for averaging of soc power output */
#define SOC_POWER_AVG_COUNT 5


#define VM_PRI_COUNT 16
// number of throttling priorities (expanded to support 0-15 range per Kingsgate HAS)
#define VM_THROT_COUNT 16
// number of boost priorities (expanded to support 0-15 range per Kingsgate HAS)
#define VM_BOOST_COUNT 16


/* number of plimit update options */
#define PLIMIT_UPDATE_MAX 8

// Helpers for DTS Coefficient values taken from fuses:
// temp as these should go in pvt_struct.h; use only if not already defined
#define DTS_K_COEFF_FUSED_TEMP(fused_k) (-1.0F * (float)fused_k)
#define DTS_Y_COEFF_FUSED_TEMP(fused_y) ((float)fused_y)

/**
 * @brief To obtain the temperature value, the following equation should be applied to the dout output of the DTS.
 * Temperature = ( 𝒅𝒐𝒖𝒕 /16384 ) ∗ 𝒀+𝑲 [℃] 
 * Reference : Synopsys Cores Sensors  Distributed Thermal Sensor (Series 2)  section 6.2
 */

#ifndef TEMP2DOUT_FUSED
    #define TEMP2DOUT_FUSED(t, fused_k, fused_y) \
        (uint32_t)(16384.0F * (t - DTS_K_COEFF_FUSED_TEMP(fused_k)) / DTS_Y_COEFF_FUSED_TEMP(fused_y))
#endif
#ifndef DOUT2TEMP_FUSED
    #define DOUT2TEMP_FUSED(dout, fused_k, fused_y) \
        (dout / 16384.0F * DTS_Y_COEFF_FUSED_TEMP(fused_y) + DTS_K_COEFF_FUSED_TEMP(fused_k))
#endif

#define MAX_VR_PER_DIE 8
/* Only Die0 has VSYS */

/*-------------- Typedefs ----------------*/
/**
 * @brief Struct for holding dts coefficient data from fuses
 */
typedef struct _dts_coeff_t
{
    uint16_t k_val;
    uint16_t y_val;
} dts_coeff_t;

/**
 * @brief Struct for holding adclk telemetry counts
 */
typedef struct _power_adclk_tel_t {
    uint64_t droop_count[NUM_AP_CORES_PER_DIE];
} power_adclk_tel_t;

/**
 * @brief Struct for holding vcpu ldo_dac -> leakage/dynamic current values from fuses
 */
typedef struct _power_fuse_vcpu_current_t
{
    uint16_t ldo_dac;
    uint32_t current_ma;
    uint16_t vcpu_mv;
    uint8_t temp_offset; // -128 to get temperature value used
} power_fuse_vcpu_current_t;

/**
 * @brief Struct for holding core ldodac -> cdyn values from fuses
 */
typedef struct _power_fuse_core_cdyn_t
{
    uint16_t ldo_dac;
    uint32_t cdyn_pf; // only need a uint16, but using uint32 to reuse interpolation code
} power_fuse_core_cdyn_t;

/**
 * @brief Struct for holding core ldodac-based values from fuses
 */
typedef struct _power_fuse_interp_common_t
{
    uint16_t ldo_dac;
    uint32_t field32;
} power_fuse_interp_common_t;

/**
 * @brief Struct to enable making vcpu interpolation code common
 */
typedef union _power_vcpu_interp_t
{
    power_fuse_interp_common_t common;
    power_fuse_vcpu_current_t current;
    power_fuse_core_cdyn_t cdyn;
} power_vcpu_interp_t;

/**
 * @brief Enum for process type from fuses
 */
typedef enum _power_fuse_process_id_t
{
    PROCESS_UNKNOWN_0 = 0,
    PROCESS_SS = 1,
    PROCESS_TT = 2,
    PROCESS_FF = 3,
    PROCESS_UNKNOWN = 0xF,
} power_fuse_process_id_t;

typedef struct _power_fuse_tdp_t
{
    uint16_t freq_MHz; // frequency used in mfg
    uint16_t power_A;  // power used in mfg
    uint8_t num_cores; // count of cores used in TDP measurement, cdyn, etc
} power_fuse_tdp_t;

/* TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491054/
   Add temperature detail to these curve/curveset structures as needed for ITD
 */

/**
 * @brief Struct for fused VF curves
 */
typedef struct _power_fuse_core_vf_t
{
    dvfs_vf_fused_pairs_t curve[VFT_CURVE_COUNT_PER_CURVESET]; // vf points for VF table
} power_fuse_core_vf_t;

/**
 * @brief Struct for fused VF curveset
 */
typedef struct _power_fuse_vf_curveset_t
{
    power_fuse_core_vf_t curveset[VFT_CURVESET_COUNT]; // curveset for VF table
} power_fuse_vf_curveset_t;


/**
 * @brief Struct for DVFS interpolated VF curveset
 */
typedef struct _power_dvfs_vf_curveset_t
{
    dvfs_vft_t curveset[VFT_CURVESET_COUNT]; // curveset for VF table
} power_dvfs_vf_curveset_t;

/**
 * @brief Struct for holding data read from fuses
 */
typedef struct _power_fuse_data_t
{
    corebits_t valid_cores;
    power_fuse_vf_curveset_t vf;
    dvfs_core_memasst_entries_t memasst;
    dvfs_vf_slope_t ldodac_to_volt;
    dts_coeff_t dts_coeff_tile[NUM_CPU_TILES];
    dts_coeff_t dts_coeff_soctop[SOC_PVT_TOTAL_CHANNELS_DTS];
    power_vcpu_interp_t vcpu_leakage[VCPU_LEAKAGE_COUNT];
    power_vcpu_interp_t vcpu_ldo_dyn[VCPU_LDO_DYNAMIC_COUNT];
    power_vcpu_interp_t core_cdyn[CORE_CDYN_COUNT];
    power_fuse_process_id_t process_id;
    power_fuse_tdp_t tdp_config; // tdp config used by mfg
    uint8_t v_ldo_dropout_mv;    // voltage in mV to add for ldo dropout
    uint8_t vcpu_guardband_mv;   // voltage in mV to add for Vin guardband
    int8_t  curve_max_temp[NUM_DVFS_ITD_TEMPERATURE_LOOKUP_COLUMNS - 1];
    uint16_t vsys_vid_mv;        // VSYS VID value from fuses
} power_fuse_data_t;

/**
 * @brief Flags for PVT VM
 */
typedef enum _power_vm_flags_t
{
    VM_FLAGS_NONE = 0,
    VM_FLAGS_DIV2 = 1,
} power_vm_flags_t;

/**
 * @brief Power PVT VM detail
 */
typedef struct _power_vm_detail
{
    power_vm_flags_t flags;
} power_vm_detail_t;

/**
 *  @brief Enums for voltage rails
 */
typedef enum _power_control_loop_vr_names_t
{
    NO_VSYS = (-1),     // VSYS is on Die0 only
    MPCL_VR_VCPU = 0,   //Die0
    MPCL_VR_VPCIE0P75,
    MPCL_VR_VSYS,
    MPCL_VR_VDDQ1P1,
    MPCL_VR_VSOC,
    MPCL_VR_VD2D0P55,
    MPCL_VR_VPLL1P2,
    MPCL_VR_VGPIO1P2,
    MPCL_VR_VCPU1,      //Die1
    MPCL_VR_VD2D0P875,
    MPCL_VR_COUNT,
} power_control_loop_vr_names_t;
/**
 *  @brief Structure AVS bus, rail lookup
 */
typedef struct _power_avs_bus_rail_t {
    uint8_t bus_id;   // index of bus for this rail
    uint8_t rail_id;  // index of rail on this bus
} power_avs_bus_rail_t, *ppower_avs_bus_rail_t;

typedef struct _power_vr_idx_t {
    power_control_loop_vr_names_t vcpu_idx;  // index for the AVS VCPU
    power_control_loop_vr_names_t vsys_idx;  // index for the AVS VR VSYS (VSYS is on Die0 only)
    power_control_loop_vr_names_t flattened_vr_start_idx; // flattened VR start. Die0 = 4 AVSBuses 2 rails (4*2 = 8) 0-7. Die1 = 1 AVSBus 2 rails (1*2 = 2) 8-9, so starts at 8
} power_vr_idx_t;

/**
 * @brief Power service config
 */
typedef struct _power_service_config_t
{
    uintptr_t soc_pvt_base;                  // soc pvt base address
    uintptr_t scf_mhu_base;                  // scf mhu base address
    uintptr_t scp_exp_csr_base;              // scp exp csr base address - needed to set FORCE_PMIN
    uintptr_t cluster_pex_base;              // cluster pex base address
    uintptr_t scp_pwrctrl_base;              // scp pwrctrl base address
    uint32_t cluster_stride;                 // number of bytes between each cluster
    power_vm_detail_t soc_vm[NUM_SOC_VM];    // soc vm config detail
    power_vm_detail_t tile_vm[NUM_TILE_VM];  // tile vm config detail
    power_avs_bus_rail_t avs_details[MPCL_VR_COUNT];    // avs bus/rail details
    pscp_avs_interface_t scp_avs_insts[MAX_AVS_INST]; // array of pointers to AVS driver client interfaces
    const corebits_t* platform_cores_in_die; // platform cores
    unsigned int platform_die_core_count;    // platform core count
    uint8_t num_vr;                          // max number of VRs (8 VRs on Die0, 2 VRs on Die1)
    power_vr_idx_t vr_idx_info;              // VR index    
    void* icc_d2d_ctx;                       // icc d2d context for power control loop
    void* icc_d2d_cli_ctx;                   // icc d2d context for power cli
    void* icc_mscp_ctx;                       // icc mscp context for power control loop
    bool platform_soc_power_support;         // true if soc power supported on platform
    bool platform_core_power_support;        // true if tile/core power supported on platform
    bool platform_is_multi_die;              // true if platform is multi-die
    bool is_primary_die;
} power_service_config_t, *ppower_service_config_t;

/**
 * @brief Struct for VFT point
 */
typedef struct _power_core_vft_point_t
{
    uint16_t freq_Mhz;   // frequency in megahertz
    uint16_t voltage_mv; // worst-case voltage in millivolts
} power_core_vft_point_t;

/**
 * @brief Struct for VFT
 */
typedef struct _power_core_vft_t
{
    corebits_t assigned_cores;              // cores assigned to this VFT
    power_core_vft_point_t vf[NUM_PSTATES]; // vf points for VF table
    uint8_t min_plimit;                     // minimum possible plimit based on curve pairs
} power_core_vft_t;

/**
 * @brief Struct for VFT current precalculation
 */
typedef struct _power_vft_precalc_current_t
{
    float dynamic;
    float leakage;
    float dynamic_ldo; // intermediate value used in dynamic calculation
    float ref_leakage; // intermediate value used in leakage calculation (unscaled value)
    float cdyn_pf;     // intermediate value used in dynamic calculation
} power_vft_precalc_current_t;

/**
 * @brief Struct for VFT current precalculation
 */
typedef struct _power_vft_precalc_t
{
    power_vft_precalc_current_t vf[NUM_PSTATES]; // precalculated current values for vf points
} power_vft_precalc_t;

/**
 * @brief Struct for VFT curveset current precalculation
 */
typedef struct _power_vft_curveset_precalc_t
{
    power_vft_precalc_t curveset[VFT_CURVESET_COUNT]; // detail calculated using worst case voltage from curveset
} power_vft_curveset_precalc_t;

/**
 * @brief Struct for derived configuration
 */
typedef struct _power_derived_config_t
{
    power_core_vft_t vfts[VFT_CURVESET_COUNT];  // detail calculated using worst case voltage from curveset
    uint16_t soc_maximum_thermal_watts_limit;   // runtime limit, established from fuse + knob override
    uint8_t assigned_vft[NUM_AP_CORES_PER_DIE]; // index of assigned vft
    uint8_t pnominal;                           // nominal pstate
} power_derived_config_t;

/* List of different command IDs that can be sent to the the power service */
typedef enum
{
    //! All get commands
    POWER_IF_CMD_GET_RUNCONFIG_KNOBS,
    POWER_IF_CMD_GET_RUNCONFIG_FUSES,
    POWER_IF_CMD_GET_RUNCONFIG_VFTPRE,
    POWER_IF_CMD_GET_RUNCONFIG,

    //! All set commands
    POWER_IF_CMD_SET_CAP,
    POWER_IF_CMD_SET_DESIRED_PSTATE,
    POWER_IF_CMD_SET_PLIMIT, 
    POWER_IF_CMD_SET_LOOP_DISABLES,
    POWER_IF_CMD_SET_RACK_LIMIT,
    POWER_IF_CMD_SET_MINUPDATE,
    POWER_IF_CMD_SET_NOMINAL,
    POWER_IF_CMD_SET_FORCED,
    POWER_IF_CMD_SET_PSTATE_FREQ,
    POWER_IF_CMD_SET_ALARM_THRESHOLD,
    POWER_IF_CMD_SET_FORCE_PMIN,
    POWER_IF_CMD_SET_SOC_HOT,
    POWER_IF_CMD_SET_MEM_HOT,
    POWER_IF_CMD_SET_THERM_TRIP,
    POWER_IF_CMD_SET_CURR_THROTTLE,

    //! All status commands
    POWER_IF_CMD_STATUS_CL,
    POWER_IF_CMD_STATUS_VRTL,
    POWER_IF_CMD_STATUS_PVTTL,
    POWER_IF_CMD_STATUS_CAP,
    POWER_IF_CMD_STATUS_DROOPCOUNT,
    POWER_IF_CMD_STATUS_MAXTEMP,
    POWER_IF_CMD_STATUS_POWER,
    POWER_IF_CMD_STATUS_PLIMIT,
    POWER_IF_CMD_STATUS_PRIMARY_CORE,
    POWER_IF_CMD_STATUS_SELECTIONS,
    POWER_IF_CMD_STATUS_VCPU,
    POWER_IF_CMD_STATUS_WARMSTART,
    POWER_IF_CMD_STATUS_FORCE_PMIN,
    POWER_IF_CMD_STATUS_CORE,
    POWER_IF_CMD_STATUS_DVFS_FSM,
    POWER_IF_CMD_STATUS_DVFS_PLIMIT,
    POWER_IF_CMD_STATUS_DVFS_CPPC,
    POWER_IF_CMD_STATUS_CORE_PRIO,
    POWER_IF_CMD_STATUS_PRIO_CNT,
    POWER_IF_CMD_STATUS_VR_INST,
    POWER_IF_CMD_STATUS_PSTATE2CPPC,
    POWER_IF_CMD_STATUS_DVFS_THROT_SR,

    //! All log commands
    POWER_IF_CMD_LOG_DUMP,
    POWER_IF_CMD_LOG_DDR,
    POWER_IF_CMD_LOG_MASK,

    //! All accelerator commands
    POWER_IF_CMD_ACCEL_BW_REDUCE,

    //! Keep last
    POWER_IF_CMD_UNKNOWN
} power_if_cmd_t;

typedef void (*power_state_handler_t)(int, const void *);  // state handler function pointer type

#define POWER_LOOP_IDLE_STATE_ID 0
/**
 * @brief enum of control loop states
 */

typedef enum _power_ctrl_loop_state_t
{
    POWER_CONTROL_STATE_IDLE = POWER_LOOP_IDLE_STATE_ID,
    POWER_CONTROL_STATE_SYNC_DIES,        // synchronization point between dies before collecting inputs
    POWER_CONTROL_STATE_WARMSTART_ENTRY,
    POWER_CONTROL_STATE_COLLECT_INPUTS,
    POWER_CONTROL_STATE_EXCHANGE_INPUTS,
    POWER_CONTROL_STATE_DISTRIBUTE_AVAILABLE,
    POWER_CONTROL_STATE_SET_PLIMIT_BEFORE_VR,
    POWER_CONTROL_STATE_SET_VR_AFTER_PLIMIT,
    POWER_CONTROL_STATE_WAIT_VR_AFTER_PLIMIT,
    POWER_CONTROL_STATE_SET_VR_BEFORE_PLIMIT,
    POWER_CONTROL_STATE_WAIT_VR_BEFORE_PLIMIT,
    POWER_CONTROL_STATE_SET_PLIMIT_AFTER_VR,
    POWER_CONTROL_STATE_EXCHANGE_COMPLETION,
    POWER_CONTROL_STATE_ERROR,
    POWER_CONTROL_STATE_MAX,
} power_ctrl_loop_state_t;


/**
 * @brief enum for queue entry
 */

typedef enum _power_loops_queue_entry_type_t
{
    LOOP_QUEUE_ENTRY_TYPE_STATE_CHANGE,
    LOOP_QUEUE_ENTRY_TYPE_STATE_SIGNAL,
	LOOP_QUEUE_ENTRY_TYPE_NOP,
    LOOP_QUEUE_ENTRY_TYPE_EXEC_IN_IDLE,
} power_loops_queue_entry_type_t;


/**
 * @brief enum of loop state retry types
 */
typedef enum _power_loop_retries_t
{
    POWER_LOOP_RETRY_TYPE_INTERVAL,
    POWER_LOOP_RETRY_TYPE_STATE,
    POWER_LOOP_RETRY_TYPE_COUNT
} power_loop_retries_t;

/**
 * @brief enum of loop IDs
 */
typedef enum _power_loop_id_t
{
    LOOP_ID_CONTROL,
    LOOP_ID_VR_TELEM,
    LOOP_ID_PVT_TELEM,
    LOOP_ID_ADCLK_TELEM,
    LOOP_ID_COUNT
} power_loop_id_t;
/**
 *  @brief Structure for basic loop state
 */
typedef struct _power_loop_state_t {
    int current_state;
    int last_state;
    int last_event;
    bool interval_in_flight;
    uint16_t retries[POWER_LOOP_RETRY_TYPE_COUNT];
    uint64_t iteration; // loop iteration count
} power_loop_state_t;

/**
 *  @brief Structure for loop residency detail
 */
typedef struct _power_loop_residency_t {
    uint64_t count;        // count of total state exits
    uint64_t ticks;        // number of clock ticks in state
    uint64_t last_entry;   // timestamp of last state entry
    uint64_t retries;      // total retries for this state
    uint16_t max_retries;  // max retries any transition
} power_loop_residency_t;

/**
 *  @brief Structure for per-loop iteration timing stats
 */
typedef struct _power_loop_timing_stats_t {
    uint64_t counter_start;      // timestamp when current iteration started
    uint32_t min_ticks;          // minimum iteration duration in ticks
    uint32_t max_ticks;          // maximum iteration duration in ticks
    uint32_t sample_count;       // internal: number of samples collected (for validity check)
} power_loop_timing_stats_t;

// Time-based emission interval (200 seconds in microseconds)
#define POWER_LOOP_TIMING_EMIT_INTERVAL_US  200000000ULL

/**
 *  @brief Structure for combined timing stats for all loops
 */
typedef struct _power_all_loops_timing_t {
    power_loop_timing_stats_t loops[LOOP_ID_COUNT];  // stats per loop
    uint64_t window_start;                            // timestamp when current window started
} power_all_loops_timing_t;

/**
 * @brief Struct for loop context, passed to common loop functions
 */

typedef struct _power_loop_context_t {
    unsigned int state_count;  // number of states in the loop
    power_loop_state_t status; // current loop status
    const power_state_handler_t *handlers; // state handler function pointer table
    power_loop_residency_t *residency; // state residency tracking
    power_loop_id_t id; // loop ID
    int error_state;  // loop's error state ID (-1 if no error state)
} power_loop_context_t;

/**
 * @brief Struct for per-core data
 */
typedef struct _power_core_t {
    uint16_t temperature_dC;           // current core temperature in 1/10 degrees C
    uint8_t current_pstate;            // from last pstate reg 
    uint8_t current_cstate;            // from last pstate reg
    uint8_t current_throt_status;      // from last pstate reg
    uint8_t current_throt_priority;    // current boost priority from update message
    uint8_t current_boost_priority;    // current boost priority from update message 
    uint8_t current_plimit;
    uint8_t selected_plimit;
    uint8_t desired_pstate;           // desired plimit; set by OS, delivered in success
    uint8_t current_base_pstate;      // this is the per-core nominal/base performance pstate
                                      // and fail messages
    uint8_t min_plimit;               // for resource distribution
    uint8_t desired_pstate_in_use;    // used with intervals_to_lower_plimit knob (this
                                      // is the desired request we're working with)
    uint8_t desired_pstate_for_count; // used with intervals_to_lower_plimit
                                      // knob (desired we're working towards)
    uint8_t desired_pstate_count;     // used with intervals_to_lower_plimit knob
                                      // (current count)
    uint8_t plimit_t1;                // current alarm lower threshold that sets DVFS CSR plimit.currthresh_1
                                      // this is the low current level that core current must be reduced to end current throttling
    uint8_t plimit_t2;                // current alarm upper threshold that sets DVFS CSR plimit.currthresh_2
                                      // this is the high current level to cause a reduction in P-State performance the throttle current on step at a time
    uint8_t plimit_t3;                // current alarm peak threshold that sets DVFS CSR plimit.currthresh_3
                                      // this is the high current level to cause a multi-step P-State performance reduction to more quickly lower core current consumption
} power_core_t;

/**
 * @brief Struct for all-core data
 */
typedef struct _power_cores_t
{
    power_core_t core[NUM_AP_CORES_PER_DIE];
} power_cores_t;

// structure for storing local/remote power calculations
typedef struct _power_latest_calcs
{
    float soc_power;           // most recent soc power measurement
    float vcpu_power;          // most recent vcpu power measurement
    uint16_t vcpu_avs_voltage; // most recent vcpu voltage
    uint16_t vcpu_avs_current; // most recent vcpu current
} power_latest_calcs_t;

/**
 * @brief Struct for plimit selection stats
 */
typedef struct _power_plimit_stats_t
{
    uint64_t acc[NUM_PSTATES];
} power_plimit_selection_stats_t;

/**
 *  @brief Struct for plimit send stats
 */
typedef struct _power_loop_plimit_stats_t
{
    power_plimit_selection_stats_t selections[VM_THROT_COUNT];
    uint64_t counter_start;
    uint64_t counter_last_send;
    uint32_t max_us;
    uint32_t min_us;
    uint32_t last_us;
} power_loop_plimit_stats_t;


/**
 *  @brief Struct for throttle/boost core count data used in resource distribution
 */
typedef struct _power_loop_pri_counts_t
{
    uint8_t throt_pri_count[VM_THROT_COUNT]; // how many cores have this throttle priority
    uint8_t required_nominal_for_throttle[NUM_PSTATES][VM_PRI_COUNT]; // how many cores have this base perf in this priority bucket (nominal and below) or would require boost in this perf (above nominal)
    uint8_t required_for_boost[NUM_PSTATES][VM_PRI_COUNT]; // how many cores have this base perf in this priority bucket (nominal and below) or would require boost in this perf (above nominal)
} power_loop_pri_counts_t;


/**
 *  @brief Struct for data of which there are local and remote versions
 */
typedef struct _power_remote_data_t
{
    power_latest_calcs_t power;
    power_loop_pri_counts_t pri_counts;
    uint16_t max_resources;
} power_remote_data_t;

/**
 *  @brief Struct for control loop state
 */
typedef struct _power_ctrl_loop_detail_t
{
    power_remote_data_t local;
    power_remote_data_t remote;
    uint16_t current_vcpu;
    uint16_t required_vcpu;
    uint16_t max_resources;
    uint16_t curr_resources;
    corebits_t plimits_pending;
    corebits_t plimits_successful;
    corebits_t throttle_priority[VM_THROT_COUNT];
    corebits_t boost_priority[VM_THROT_COUNT];
    power_cores_t cores;
    bool throttling;      // currently throttling
    bool throttle_query;  // same as throttling, but will remain true until queried
    bool rack_limit;      // cached value of last rack_limit flag observed
    bool loop_failure;    // is loop in a failure state
    bool loop_fail_query; // same as loop failure, but will remain true until queried
    power_loop_plimit_stats_t plimit;

} power_ctrl_loop_detail_t;

/**
 *  @brief Enum of VR telemetry states
 */

typedef enum _power_vr_telem_state_t
{
    POWER_VR_TELEM_STATE_IDLE = POWER_LOOP_IDLE_STATE_ID,
    POWER_VR_TELEM_STATE_CURRENT_TELEMETRY,
    POWER_VR_TELEM_STATE_TEMP_TELEMETRY,
    POWER_VR_TELEM_STATE_ERROR,
    POWER_VR_TELEM_STATE_MAX,
} power_vr_telem_state_t;


/**
 *  @brief Structure for PVT state
 */

typedef struct _power_telemetry_pvt_state_t {
    uint64_t valid_samples;    // count of valid samples
    uint16_t last_sample;      // last sample value (converted from raw)
    uint16_t last_sample_raw;  // last sample value
    uint16_t sample_hi;        // highest sample value (converted)
    uint16_t sample_lo;        // lowest sample value (converted)
} power_telemetry_pvt_state_t;


// structure for tracking latest v/c/t from AVS for each rail
typedef struct _power_vrs_avs_latest
{
    uint16_t voltage;     // Raw AVS value, 1LSB=1mV
    uint16_t current;     // Raw AVS value, 1LSB=10mA
    uint16_t temperature; // Raw AVS value, 1LSB=0.1 Celsius
} power_vrs_avs_latest_t;

// context structure for VRs
typedef struct _power_vrs_context
{
    // latest current/temp/voltage here
    power_vrs_avs_latest_t vr_inputs[MAX_VR_PER_DIE];
    power_latest_calcs_t latest_power;

    // store recent power calculations
    float soc_power[SOC_POWER_AVG_COUNT];        /* most recent x soc power measurements,
                                                    most recent in 0 index */
    float remote_soc_power[SOC_POWER_AVG_COUNT]; /* most recent x soc power measurements,
                                             most recent in 0 index */
    float vcpu_power_log[SOC_POWER_AVG_COUNT];
    float vcpu_current_log[SOC_POWER_AVG_COUNT];
} power_vrs_context_t;
/**
 *  @brief Struct for telemetry loops state
 */

typedef struct _power_telem_loop_detail_t {
    power_telemetry_pvt_state_t soc_top_temp_data[SOC_PVT_TOTAL_CHANNELS_DTS];
    power_telemetry_pvt_state_t soc_top_voltage_data[SOC_PVT_TOTAL_CHANNELS_VM];
    uint16_t soc_max_temp_dC;    // soc max temp in 0.1C
    power_vrs_avs_latest_t* vr_data;
} power_telem_loop_detail_t;

enum _power_cap_update_result_t
{
    MP_POWER_CAP_SUCCESS = 0,
    MP_POWER_CAP_PENDING,
    MP_POWER_CAP_FAIL_PREVIOUS_UPDATED, /* Returned (via callback) when a
                                           previous request fails due to new
                                           request */
    MP_POWER_CAP_FAIL_CLI_NOT_ALLOWED,  /* CLI not allowed if non-CLI in progress
                                         */
    MP_POWER_CAP_FAIL_INVALID_REQUEST,   /* Invalid request received */
};


/**
 *  @brief Enum of PVT telemetry states
 *
 *  PVT loop is simpler; no events other than interval -- read_pvt does not
 * require a signal/event to return to idle, so no chance for entry into an
 * error state. Keeping the same basic structure to allow for timestamping, etc.
 */
typedef enum _power_pvt_telem_state_t
{
    POWER_PVT_TELEM_STATE_IDLE = POWER_LOOP_IDLE_STATE_ID,
    POWER_PVT_TELEM_STATE_READ_PVT,
    POWER_PVT_TELEM_STATE_MAX,
} power_pvt_telem_state_t;

/**
 *  @brief Enum of ADCLK telemetry states
 *
 * No events other than interval, it does not
 * require a signal/event to return to idle, so no chance for entry into an
 * error state. Keeping the same basic structure to allow for timestamping, etc.
 */
typedef enum _power_adclk_telem_state_t
{
    POWER_ADCLK_TELEM_STATE_IDLE = POWER_LOOP_IDLE_STATE_ID,
    POWER_ADCLK_TELEM_STATE_READ_ADCLK,
    POWER_ADCLK_TELEM_STATE_MAX,
} power_adclk_telem_state_t;

typedef struct _pwr_intparams {
    power_ctrl_loop_detail_t* p_pwrstatus_s_ctrl_loop;
    power_loop_context_t* p_pwrstatus_s_ctrl_loop_context;
    power_telem_loop_detail_t* p_pwrstatus_s_telem_loop;
    power_loop_context_t* p_pwrstatus_s_vr_telem_loop_context;        
    power_loop_context_t* p_pwrstatus_s_pvt_telem_loop_context; 

    power_knobs_t* p_knobs;
    power_fuse_data_t* p_fuses;    
} pwr_intparams_t;   


/**
 * @brief Checks if HW has GPIO connected meaningfully
 *
 * @return true or false
 *
 */
bool power_hw_gpio_connected();

/**
 * @brief Get the adclk droop count telemetry.
 *
 * \b Description:
 *      Retrieves the adaptive clocking droop count for all cores within a die from the power module and copies the telemetry data to the caller-provided address.
 * 
 * @param[in] adclk_tel address of the destination telemetry, provided by the caller as a pointer.
 *
 */
void power_get_adclk_telem(power_adclk_tel_t* adclk_tel);

/**
 * @brief Reset the adclk droop count telemetry.
 *
 * \b Description:
 *      Used to reset the adaptive clocking droop count to 0 for all cores within a die. Should be called once the telemetry service received the telemetry.
 *
 */
void power_reset_adclk_telem();

/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

