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
#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/
/* static config macros*/
#define MAX_BYTES_PER_FUSE (8)
#define BITS_PER_BYTE      (8)
#define MAX_BITS_PER_FUSE  (MAX_BYTES_PER_FUSE * BITS_PER_BYTE)

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
#define VFT_PER_SET                  4

/* number of power samples for averaging of soc power output */
#define SOC_POWER_AVG_COUNT 5


#define VM_PRI_COUNT 8
// number of throttling priorities 
#define VM_THROT_COUNT 8
// number of throttling priorities 
#define VM_BOOST_COUNT 8


/* number of plimit update options */
#define PLIMIT_UPDATE_MAX 8

/*-------------- Typedefs ----------------*/

/* =============================================================*/
/* Below was acquired from previously auto-generated config
   structures; expect these to be replaced when config is available */
/* =========================== BEGIN COPY FROM GENERATED CONFIG */
// TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491012/
// replace with actual config when available


typedef enum
{
    power_loops_disable_t_NONE = 0,             // All power module loops enabled
    power_loops_disable_t_CTRL_LOOP = 1,        // Power control loop disabled
    power_loops_disable_t_VR_TELEM_LOOP = 2,    // Power VR telemetry loop disabled
    power_loops_disable_t_PVT_TELEM_LOOP = 4,   // Power PVT telemetry loop disabled
    power_loops_disable_t_CTRL_VR_LOOP = 3,     // Power control and VR telemetry loops disabled
    power_loops_disable_t_CTRL_PVT_LOOP = 5,    // Power control and PVT telemetry loops disabled
    power_loops_disable_t_VR_PVT_LOOP = 6,      // Power VR and PVT telemetry loop disabled
    power_loops_disable_t_ALL = 7,              // Power control, VR and PVT telemetry loops disabled
    _power_loops_disable_t_PADDING = 0xffffffff // Force packing to int size
} power_loops_disable_t;

typedef enum
{
    power_loops_minimum_plimit_t_NONE = 0,             // No required plimit requests per loop iteration
    power_loops_minimum_plimit_t_MIN_1 = 1,            // Require 1 plimit request
    power_loops_minimum_plimit_t_MIN_2 = 2,            // Require 2 plimit requests
    power_loops_minimum_plimit_t_MIN_4 = 3,            // Require 4 plimit requests
    power_loops_minimum_plimit_t_MIN_8 = 4,            // Require 8 plimit requests
    power_loops_minimum_plimit_t_MIN_16 = 5,           // Require 16 plimit requests
    power_loops_minimum_plimit_t_MIN_32 = 6,           // Require 32 plimit requests
    power_loops_minimum_plimit_t_MIN_64 = 7,           // Require 64 plimit requests
    power_loops_minimum_plimit_t_MIN_128 = 8,           // Require max 128 plimit requests
    _power_loops_minimum_plimit_t_PADDING = 0xffffffff // Force packing to int size
} power_loops_minimum_plimit_t;

typedef struct __attribute__((packed))
{
    // proportional coefficient *1000
    uint32_t kpt;
    // integral coefficient *1000
    uint32_t kit;
    // derivative coefficient *1000
    uint32_t kdt;
    // offset in mW
    uint32_t setpoint_offset;
} power_pid_config_t;

typedef struct __attribute__((packed))
{
    // Enable MPMM
    bool enable;
    // MPMM Gear
    uint8_t gear;
} power_mpmm_config_t;

typedef struct __attribute__((packed))
{
    // value in mV
    uint16_t hyst_threshold;
    // value in mV
    uint16_t alarm_threshold;
} power_pvt_threshold_config_vm_t;

typedef struct __attribute__((packed))
{
    // value in 0.1C
    uint16_t hyst_threshold;
    // value in 0.1C
    uint16_t alarm_threshold;
} power_pvt_threshold_config_dts_t;

typedef struct __attribute__((packed))
{
    // overvolt thresholds
    power_pvt_threshold_config_vm_t overvolt;
    // undervolt thresholds
    power_pvt_threshold_config_vm_t undervolt;
} power_pvt_vm_cfg_t;

typedef struct __attribute__((packed))
{
    // SOC VM thresholds (vddq from DDR10, Vsoc in MSCP, Va0p85 in PCIe0, Vsoc northwest PCM, Vsys northwest PCM, Vddq from DDR1, Va0p85 in PCIe2, Vsys southeast PCM)
    power_pvt_vm_cfg_t thresholds[18];
} power_pvt_soc_vm_cfg_t;

typedef struct __attribute__((packed))
{
    // Tile VM thresholds (Vcore0-disable, Vcore1-disable, Vcpu0, Vsys)
    power_pvt_vm_cfg_t thresholds[4];
} power_pvt_tile_vm_cfg_t;

typedef struct __attribute__((packed))
{
    // max 7
    uint8_t rolling_window_count;
    // 10-20 is valid
    uint8_t telemetry_epoch_count;
    // 0x0-0x1f, 512-16384 clocks
    uint8_t inc_ctr;
    // 0x0-0x1f, 512-16384 clocks
    uint8_t dec_ctr;
    // 0-3, 1-4 steps
    uint8_t inc_amt;
    // 0-3, 1-4 steps
    uint8_t dec_amt0;
    // 0-7, 1-8 steps
    uint8_t dec_amt1;
} power_currthrot_cfg_t;

typedef struct __attribute__((packed))
{
    // up to 100
    uint8_t iref_to_max_percent;
    uint8_t t1_percent;
    uint8_t t2_percent;
    uint8_t t3_percent;
} power_currthrot_threshold_cfg_t;

typedef struct __attribute__((packed))
{
    // hot thresholds
    power_pvt_threshold_config_dts_t hot;
    // thermtrip thresholds
    power_pvt_threshold_config_dts_t thermtrip;
    // 0x0-0xf, 262144-4194304 clocks
    uint8_t inc_ctr;
    // 0x0-0xf, 262144-4194304 clocks
    uint8_t dec_ctr;
    // 0-3, 1-4 steps
    uint8_t inc_amt;
    // 0-3, 1-4 steps
    uint8_t dec_amt;
} power_tile_tempthrot_cfg_t;

typedef struct __attribute__((packed))
{
    // enable adaptive clocking/throttling
    bool enable;
    // Telemetry interval in milliseconds, 0 disables
    uint16_t telemetry_interval;
    // 0x0-0x1f, 512-16384 clocks
    uint8_t inc_ctr;
    // 0x0-0x1f, 512-16384 clocks
    uint8_t dec_ctr;
    // 0-3, 1-4 steps
    uint8_t inc_amt;
    // 0-3, 1-4 steps
    uint8_t dec_amt;
    // Select 0: Synchronous, 1: Asynchronous response option (see ADCLK_CR1)
    uint8_t resp_option;
    // Number of PLL output cycles to wait before recovery step (see ADCLK_CR1)
    uint8_t recovery_step_wait;
    // Size of code jump on recovery step (see ADCLK_CR1)
    uint8_t recovery_step;
    // Number of PLL output cycles to wait before recovering back to nominal (see ADCLK_CR1)
    uint8_t resp_wait;
    // Code for synchronous droop response (see ADCLK_CR1)
    uint8_t resp_code;
    // Comparator configuration (see ADCLK_CR2)
    uint8_t adclk_comp_config;
    // Number of droop events seen within throttle window before throttle request is asserted (see ADCLK_CR2)
    uint8_t throttle_threshold;
    // Window for counting droop events before triggering throttle request (see ADCLK_CR2)
    uint8_t throttle_window_count;
} power_adclk_cfg_t;

typedef struct __attribute__((packed))
{
    // enable adaptive clocking offset writes
    bool enable;
    // 0-63, Per-core value added/subtracted to/from the ldoDacIn[8:0] value driven by DVFS
    uint8_t offset_value[68];
} power_adclk_offset_cfg_t;

typedef struct __attribute__((packed))
{
    // hot thresholds
    power_pvt_threshold_config_dts_t hot;
    // thermtrip thresholds
    power_pvt_threshold_config_dts_t thermtrip;
} power_soc_temp_cfg_t;

typedef struct __attribute__((packed))
{
    // 0 to disable, Voltage in mV to write to voltage regulator at boot
    uint16_t vr[10];
} power_force_vrs_t;

typedef struct __attribute__((packed))
{
    // x^3 coefficient
    float a;
    // x^2 coefficient
    float b;
    // x coefficient
    float c;
    // constant
    float d;
} power_leakage_poly_t;

typedef struct __attribute__((packed))
{
    // Polynomial for temperature scaling of leakage - indexed to process corner 0-SS, 1-TT, 2-FF, 3-unknown
    power_leakage_poly_t poly_coefficients[4];
} power_leakage_temp_scaler_t;

typedef struct __attribute__((packed))
{
    // enable fatal error handling
    bool enable_error;
    // 0x0-0x3, Programmable count value for FLL frequency lock counter (dco1_lckcntsel)
    uint8_t lckcntsel;
    // Mask plllockraw errors
    bool mask_plllockraw;
    // Mask freqchangetimeout errors
    bool mask_freqchangetimeout;
    // Mask fllcaltimeout errors
    bool mask_fllcaltimeout;
    // Mask locktimeout errors
    bool mask_locktimeout;
    // Mask freq error
    bool mask_freqerror;
} power_plllock_cfg_t;

typedef struct __attribute__((packed))
{
    uint8_t pstate_min;
    uint8_t pstate_max;
} power_fllcal_pstate_bounds_t;

/* =========================== END COPY FROM GENERATED CONFIG */

/**
 * @brief Struct for holding dts coefficient data from fuses
 */
typedef struct _dts_coeff_t
{
    uint16_t k_val;
    uint16_t y_val;
} dts_coeff_t;

/**
 *  @brief Struct for power module configuration knobs
 */

// Temporary - equate existing typedef with config manager generated typedef
typedef POWER_CAPPING_MODE power_capping_mode_t;

// TODO: Move this struct to kng_mscp_config_knobs.xml
typedef struct _power_knobs_t
{
    power_loops_disable_t loops_disable; // disabled loops (control, vr telem, pvt telem)
    power_capping_mode_t capping_mode;
    power_soc_temp_cfg_t soc_temp; // configuration for soc thermal alarms that feed into soc_hot and thermtrip
    power_pvt_soc_vm_cfg_t soc_vm;                     // config for soc vm alarm
    power_pvt_tile_vm_cfg_t tile_vm;                   // config for tile vm alarm
    power_currthrot_cfg_t current_throt;               // configuration for odcm and dvfs current throttling
    power_currthrot_threshold_cfg_t current_threshold; // current threshold percentages for per-pstate generation
    power_tile_tempthrot_cfg_t tile_temp_throt; // configuration for tile temperature thresholds and related throttling
    power_adclk_cfg_t adclk_throt;              // configuration for adaptive clock throttling
    power_adclk_offset_cfg_t adclk_offset; // configuration for adaptive clock offset (one per die; pick the right one during knob init)
    power_pid_config_t pid;                          // pid configuration
    power_mpmm_config_t mpmm;                        // mpmm configuration
    power_force_vrs_t forced_vrs;                    // 0 or voltage_in mv for each VR
    power_leakage_temp_scaler_t leakage_temp_scaler; // polynomial coefficients for temperature scaling of leakage
    power_fllcal_pstate_bounds_t fllcal_pstate_bounds; // bounds to limit ftable frequency calibration
    power_loops_minimum_plimit_t minimum_plimit_updates; // minimum number of pstate updates per loop iteration - 0 -> 8 (0 is none, number of updates is 2^(x-1))
    power_plllock_cfg_t plllock_cfg; // core pll lock config
    float static_rail_power_watts;   // static/constant adder to account for other SoC power rails
    uint16_t control_loop_interval;  // Power control loop timer interval (ms)
    uint16_t pvt_loop_interval;      // PVT telemetry loop timer interval (ms)
    uint16_t temp_telemetry_divider; // Number of control loop intervals per 1
                                     // VR temperature telemetry
    uint16_t r_loadline_vcpu0_uohm; // Loadline resistance used in VR setpoint (and power adjustment) calculation (uOhm)
    uint16_t r_loadline_vcpu1_uohm; // Loadline resistance used in VR setpoint (and power adjustment) calculation (uOhm)
    uint16_t vsys_r_loadline_uohm; // Loadline resistance used in VR power adjustment calculation (uOhm)
    uint16_t soc_maximum_thermal_watts_limit;            // SOC maximum power limit,
                                                         // specific to thermal (W)
    uint16_t soc_maximum_electrical_current_limit_vcpu0; // Vcpu0 current maximum value (A)
                                                         // which is used to determine the
                                                         // maximum electrical power limit
    uint16_t soc_maximum_electrical_current_limit_vcpu1; // Vcpu1 current maximum value (A)
    int16_t vcpu_offset_mv;                              // offset in mV to apply to Vcpu calculation
    uint8_t force_pstate;                                // 32 to disable, 0-31 to force pstate

    uint8_t allowed_plimit_minimum; // limits control loop plimit selection - setting to nominal would disable turbo speeds
    uint8_t allowed_plimit_maximum;    // limits control loop plimit selection
    uint8_t max_plimit_step_size_up;   // limits control loop plimit selection - max number of steps up
    uint8_t max_plimit_step_size_down; // limits control loop plimit selection - max number of steps down
    uint8_t intervals_to_lower_plimit; // Control loop intervals over which to
                                       // sample last PSTATE (with no
                                       // throttling) before lowering plimit
                                       // below SCP calculated plimit -- TODO -
                                       // use desired perf from success message?
    uint8_t activity_factor_dhry_adjustment; // A default activity factor to use with peak CPU current calculation
    bool power_enable_velocity_boost;        // Enable velocity boost (prioritized turbo vs equal turbo)
    bool allow_plimit_below_nominal; // Outside of power capping, drop plimit to OS desired perf even if below nominal
    bool c4_cores_limit_to_nominal; // true to limit cores in c4 c-state to nominal perf
    bool c3_cores_limit_to_nominal; // true to limit cores in c3 c-state to nominal perf
    bool c2_cores_limit_to_nominal; // true to limit cores in c2 c-state to nominal perf
    bool enable_survivability_mode; // true to init DVFS in SW control mode and safe settings
    bool calsm_enable;              // true to enable fgpll calibration
    bool c1_tel_enable;             // should C1 telemetry be enabled

    bool soc_vm_overvolt_en;     // enable SOC VM overvolt alarm
    bool soc_vm_undervolt_en;    // enable SOC VM undervolt alarm
    bool tile_vm_overvolt_en;    // enable Tile VM overvolt alarm
    bool tile_vm_undervolt_en;   // enable Tile VM undervolt alarm
    bool soc_temp_hot_en;        // enable SOC temp hot alarm
    bool soc_temp_thermtrip_en;  // enable SOC temp thermtrip alarm
    bool tile_temp_hot_en;       // enable Tile temp hot alarm
    bool tile_temp_thermtrip_en; // enable Tile temp thermtrip alarm

    uint8_t survivability_mode_pstate; // Pstate to boot to under survivability mode
    uint8_t nominal_pstate;            // Pstate to use for nominal
    int8_t ldo_offset;                 // ldo dac code offset
} power_knobs_t;

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
 * @brief Struct for DVFS interpolated VF curves
 */
typedef struct _power_dvfs_vf_curve_t
{
    dvfs_vft_t curve[VFT_CURVE_COUNT_PER_CURVESET]; // vf points for VF table
} power_dvfs_vf_curve_t;

/**
 * @brief Struct for DVFS interpolated VF curveset
 */
typedef struct _power_dvfs_vf_curveset_t
{
    power_dvfs_vf_curve_t curveset[VFT_CURVESET_COUNT]; // curveset for VF table
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
    const corebits_t* platform_cores_in_die; // platform cores
    unsigned int platform_die_core_count;    // platform core count
    void* icc_d2d_ctx;                       // icc d2d context
    bool platform_soc_power_support;         // true if soc power supported on platform
    bool platform_core_power_support;        // true if tile/core power supported on platform
    bool platform_is_multi_die;              // true if platform is multi-die
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
    POWER_IF_CMD_GET_RUNCONFIG_KNOBS,
    POWER_IF_CMD_GET_RUNCONFIG_FUSES,
    POWER_IF_CMD_SET_CAP,
    POWER_IF_CMD_SET_DESIRED_PSTATE,
    POWER_IF_CMD_SET_PLIMIT, 
    POWER_IF_CMD_SET_LOOP_DISABLES,
    POWER_IF_CMD_SET_RACK_LIMIT,
    POWER_IF_CMD_SET_MINUPDATE,
    POWER_IF_CMD_SET_NOMINAL,
    POWER_IF_CMD_STATUS_CL,
    POWER_IF_CMD_STATUS_VRTL,
    POWER_IF_CMD_SET_PVTTL,

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
 * @brief Struct for loop context, passed to common loop functions
 */

typedef struct _power_loop_context_t {
    unsigned int state_count;  // number of states in the loop
    power_loop_state_t status; // current loop status
    const power_state_handler_t *handlers; // state handler function pointer table
    power_loop_residency_t *residency; // state residency tracking
    power_loop_id_t id; // loop ID
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


/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

