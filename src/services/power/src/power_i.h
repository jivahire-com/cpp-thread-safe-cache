//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_i.h
 * Header containing internal definitions for power service
 */

#pragma once

/*----------- Nested includes ------------*/
#include "corebits.h"

#include <DfwkDriver.h>
#include <FpFwAssert.h>
#include <fpfw_status.h> // for FPFW_STATUS_SUCCESS, FPFW_STATUS_INVA...
#include <kng_error.h>
#include <power_runconfig.h>
#include <power_runconfig_i.h>
#include <stdbool.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MODULE_NAME "[PWRSVC] "
#define NEWLINE     "\n"

// set to 1 for more verbose logs
#define PWR_ENABLE_TRACE_LEVEL_LOG 0

#if PWR_ENABLE_TRACE_LEVEL_LOG
#define POWER_LOG_TRACE(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#else
#define POWER_LOG_TRACE(fmt, ...)
#endif
#define POWER_LOG_INFO(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define POWER_LOG_WARN(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define POWER_LOG_ERR(fmt, ...)  printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define POWER_LOG_CRIT(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)

#ifndef MIN
    #define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
    #define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif
#ifndef ARRAY_SIZE
    #define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

/* other helper macros */
#define FLOAT_TO_UNSIGNED(x) ((unsigned)(x + 0.5f))

/* Macros to convert AVS uint16_t to float */
#define AVS_TEMPERATURE_FLOAT(x) (((float)x) / 10)
#define AVS_CURRENT_FLOAT(x)     (((float)x) / 100)
#define AVS_VOLTAGE_FLOAT(x)     (((float)x) / 1000)

/* other helper macros */
#define DIMOF(x) (sizeof(x) / sizeof(x[0]))

/* min/max Vcpu limits */
#define VR_VCPU_MAX_VOLTAGE_MV 1320
#define VR_VCPU_MIN_VOLTAGE_MV 750

/* power cap */
#define NO_POWER_CAP (UINT16_MAX)

// Callback function pointer type for power cap
typedef void (*power_cap_completed_callback_t)(int result, uint16_t current_cap, uint16_t previous_cap_watts);

/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

// for unit test, only
// function to sequence AP core/SOC power initialization
void power_ap_soc_init();

/**
 * @brief Caps ldodac value to max value if it exceeds the limit
 *
 * @param[in] freq          - Frequency provided from vf_curves in fuse structure
 * @param[in] ldodac        - LDODAC value to be set as input
 * @param[in] curve_idx     - Index of vf curve to use to obtain max cap value
 *
 * @return pmm revision
 *
 */
void power_ap_soc_init_post_remote_sync();

/*
* Wait until both die pvt init is complete before setting sensor trigger and telemetry config
*/

int32_t ldo_cap_to_max(uint32_t freq, uint32_t ldodac, uint32_t curve_idx);

/**
 * @brief Gets the pmm revision by reading the corresponding fuse
 *
 * @return pmm revision
 *
 */
uint8_t power_fuses_get_pmm_rev();

/**
 * @brief Reads DTS k/y coefficients using offsets, widths passed
 *
 * @param[in] k_offset - bit offset for specific dts k_val fuse
 * @param[in] k_width - width for specific dts k_val fuse
 * @param[in] y_offset - bit offset for specific dts y_val fuse
 * @param[in] y_width - width for specific dts y_val fuse
 * @param[out] dts_coeff - Pointer to parameter variable
 *
 * @return FPFW_STATUS_SUCCESS or FPFW_STATUS_INVALID_ARGS
 *
 */
int32_t power_fuses_get_dts_coeff(uint32_t k_offset,
                                  uint32_t k_width,
                                  uint32_t y_offset,
                                  uint32_t y_width,
                                  uint32_t fuse_elements,
                                  uint32_t coeff_count,
                                  uint32_t coeff_spacing,
                                  dts_coeff_t* dts_coeff);

/**
 * @brief Reads entries of dvfs core memasst table fuses into structure
 *
 *
 * @param[out] memasst_entries - Pointer to a fused core memasst structure
 *
 * @return FPFW_STATUS_SUCCESS, FPFW_STATUS_FAIL,  FPFW_STATUS_INVALID_ARGS
 *
 */
int32_t power_fuses_read_memasst(dvfs_core_memasst_entries_t* memasst_entries);

/**
 * @brief Clears fuse-disabled cores in a corebits structure
 *
 * \b Description:
 *      Bits set (previously) in a corebits structure are cleared if the
 * corresponding core disabled fuse bit is set
 *
 * @param[out] valid_bits - Pointer to a corebits structure
 *
 * @return FPFW_STATUS_SUCCESS or FPFW_STATUS_INVALID_ARGS
 *
 */
int32_t power_fuses_clear_core_valid_bits(corebits_t* valid_bits);

/**
 * @brief Reads LDODAC to mV equation parameters
 *
 * @param[out] slope_offset - Pointer to parameter variable
 *
 * @return FPFW_STATUS_SUCCESS or FPFW_STATUS_INVALID_ARGS
 *
 */
int32_t power_fuses_get_ldodac_to_voltage(dvfs_vf_slope_t* slope_offset);

/**
 * @brief Reads ldo headroom value (mV) from fuses
 *
 * @param[out] ldo_headroom - Pointer to parameter variable
 *
 * @return FPFW_STATUS_SUCCESS or FPFW_STATUS_INVALID_ARGS
 *
 */
int32_t power_fuses_get_ldo_headroom(uint8_t* ldo_headroom);

/**
 * @brief Reads Vcpu guardband value (mV) from fuses
 *
 * @param[out] vcpu_guardband - Pointer to parameter variable
 *
 * @return FPFW_STATUS_SUCCESS or FPFW_STATUS_INVALID_ARGS
 *
 */
int32_t power_fuses_get_vcpu_guardband(uint8_t* vcpu_guardband);

/**
 * @brief Reads Vcpu leakage table from fuses
 *
 * @param[out] vcpu_leakage - Pointer to parameter variable
 *
 * @return FPFW_STATUS_SUCCESS, FPFW_STATUS_FAIL, FPFW_STATUS_INVALID_ARGS
 *
 */
int32_t power_fuses_vcpu_leakage(power_vcpu_interp_t* vcpu_leakage, uint32_t count);

/**
 * @brief Reads Vcpu ldo dynamic table
 *
 * @param[out] vcpu_ldo_dyn - Pointer to parameter variable
 *
 * @return FPFW_STATUS_SUCCESS, FPFW_STATUS_FAIL,  FPFW_STATUS_INVALID_ARGS
 *
 */
int32_t power_fuses_ldo_dyn(power_vcpu_interp_t* vcpu_ldo_dyn, uint32_t count);

/**
 * @brief Reads core cdyn table from fuses
 *
 * @param[out] core_cdyn - Pointer to parameter variable
 *
 * @return FPFW_STATUS_SUCCESS, FPFW_STATUS_FAIL, FPFW_STATUS_INVALID_ARGS
 *
 */
int32_t power_fuses_core_cdyn(power_vcpu_interp_t* core_cdyn, uint32_t count);

/**
 * @brief Reads process corner identifier from fuses
 *
 * @param[out] process_id - Pointer to parameter variable
 *
 * @return FPFW_STATUS_SUCCESS or FPFW_STATUS_INVALID_ARGS
 *
 */
int32_t power_fuses_process_id(power_fuse_process_id_t* process_id);

/**
 * @brief Reads TDP config assumed/measured with from fuses
 *
 * @param[out] tdp_config - Pointer to parameter variable
 *
 * @return FPFW_STATUS_SUCCESS, FPFW_STATUS_FAIL, FPFW_STATUS_INVALID_ARGS
 *
 */
int32_t power_fuses_get_tdp_config(power_fuse_tdp_t* tdp_config);

/**
 * @brief Reads pairs of VF curve from fuses into structure
 *
 *
 * @param[out] vf_curves - Pointer to a fused vf structure
 * @param[in] ldo_offset - Offset for LDO values (can be used to raise/lower LDO output)
 *
 * @return FPFW_STATUS_SUCCESS or FPFW_STATUS_INVALID_ARGS
 *
 */
int32_t power_fuses_read_vf(power_fuse_vf_curveset_t* vf_curves, int8_t ldo_offset);

/**
 * @brief Get timestamp counter value
 *
 * \b Description:
 *      Use to get current 64bit timestamp value
 *
 * @return counter
 *
 */
uint64_t power_timer_get_counter();
/**
 * @brief Get timestamp counter ticks for duration in microseconds
 *
 * \b Description:
 *      Use to get an equivalent duration in counter ticks for an input in
 * microseconds
 *
 * @param[in] time_in_us - time in microseconds to convert to ticks
 *
 * @return counter
 *
 */
uint64_t power_timer_get_counter_ticks_us(uint16_t time_in_us);

/**
 * @brief Convert counter ticks into microseconds
 *
 * \b Description:
 *      Use to get an equivalent duration in microseconds for an input in counter ticks
 *
 * @param[in] ticks - time in ticks to convert to microseconds
 *
 * @return time in us
 *
 */
uint64_t power_timer_get_us_from_counter(uint32_t ticks);

/**
 * @brief Start the loop timers
 *
 * \b Description:
 *      Use to start loop timers once all init is done
 *
 */
void power_timer_start_loop_timers();

/**
 * @brief Handler for async requests related to CLI
 *
 * @param[in] p_request - The power service async request
 * @param[in] p_context - Related context given to async handler function
 *
 */
void power_cli_requests_async_handler(PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_context);

/**
 * @brief Call kick off VR reads necessary for power calculations
 *
 * @return none
 *
 */
void power_vrs_initiate_vr_reads();

/**
 * @brief Returns recent power in mW
 *
 * @return power in mW
 *
 */
uint32_t power_vrs_get_recent_power_mw();

/**
 * @brief Store remote power values
 * 
 * @param p_remote_power 
 */
void store_remote_soc_power(power_latest_calcs_t* p_remote_power);

/**
 * @brief Get the power vrs context
 * 
 * @return power_vrs_context_t* 
 */
power_vrs_context_t* get_power_vrs_context();

/**
 * @brief Initiates send of vcpu voltage write, will lead to signal of voltage change pending/done
 *
 * @param[in] voltage_mv - Voltage in mV
 *
 * @return none
 *
 */
void power_vrs_write_vcpu_voltage(uint16_t voltage_mv);

/**
 * @brief Initiates read of vcpu voltage, will lead to signal of voltage change pending/done
 *
 * @return none
 *
 */
void power_vrs_read_vcpu_voltage();

/**
 * @brief Init function to initialize internal power_cap and related data.
 *
 * @return none
 *
 */
void power_cap_init();

/**
 * @brief Get VRcpu portion of power cap for use in control loop
 *
 * \b Description:
 *      Called to determine the current VRcpu portion of the power cap; this
 * function also determines which of the various caps are the acting
 * cap--maximum thermal limit in watts, maximum electrical limit (specific to
 * VRcpu rail), or the rack power cap
 *
 * @param[out] p_new_cap - Pointer to boolean, which if provided will be set to
 * true if a new rack power cap has been put into place
 * @param[in] p_local_power - local power inputs
 * @param[in] p_remote_power - remote power inputs
 *
 * @return VRcpu portion of power cap
 *
 */
float power_cap_get_vrcpu_cap(bool* p_new_cap, power_latest_calcs_t* p_local_power, power_latest_calcs_t* p_remote_power);

/**
 * @brief Indicate that previous power cap has been finalized
 *
 * \b Description:
 *      After a call to power_cap_get_vrcpu_cap, a call to this function
 * indicates that control loop is updated to achieve the new goal and initial HW
 * changes have been made.  The primary function of this API is to notify the
 * cap requestor that the cap is now in place.
 *
 * @return none
 *
 */
void power_cap_finalize();

/**
 * @brief Update the SOC power cap (rack limit)
 *
 * \b Description:
 *      This API should be called to pass in updates to the SOC power cap.
 *
 * @param[in] callback - Pointer to callback function which will be called once
 * cap is in place
 * @param[in] new_power_cap - the new SOC power cap in watts
 * @param[in] source_is_cli - should be true if source is CLI; used to ensure
 * CLI does not interfere with actual rack limit changes
 *
 * @return a pending or fail result (see _power_cap_update_result_t)
 *
 */
int power_cap_update(power_cap_completed_callback_t callback, uint16_t new_power_cap, bool source_is_cli);

/**
 * @brief Cancel an existing SOC power cap (rack limit)
 *
 * \b Description:
 *      This API should be called to cancel a previous power cap.
 *
 * @param[in] callback - Pointer to callback function which will be called once
 * cap is in place
 * @param[in] source_is_cli - should be true if source is CLI; used to ensure
 * CLI does not interfere with actual rack limit changes
 *
 * @return a pending or fail result (see _power_cap_update_result_t)
 *
 */
int power_cap_cancel(power_cap_completed_callback_t callback, bool source_is_cli);

/**
 * @brief Get power capping capped state
 *
 * @return true if capped
 *
 */
bool power_cap_is_capped();

/**
 * @brief Get the current power cap
 * 
 * @return uint16_t 
 */
uint16_t get_current_soc_power_cap();

/**
 * @brief Get the inst max electrical limit calculated for the respective die
 * 
 * @return float 
 */
float get_inst_max_electrical_limit(uint8_t die_num);

/**
 * @brief Get the current vrcpu cap object
 * 
 * @return float 
 */
float get_current_vrcpu_cap();

/**
 * @brief Find maximum of all cores plimit voltage requirement
 *
 * \b Description:
 *      This API should be called to determine the core voltage requirement related to LDO input voltage.
 * Input is the global power context.
 * @param[in] p_runconfig - power runtime configuration
 * @param[in] power_cores_t - all core configuration
 * @return max voltage requirement (mV)
 *
 */
uint16_t power_vcpu_calc_max_core_voltage_mv(power_runconfig_t* p_runconfig, power_cores_t* p_cores);

/**
 * @brief Calculate core leakage scaler for temperature
 *
 * \b Description:
 *      This API will be called as part of peak current calculation; included here for test.
 *
 * @param[in] p_runconfig - power runtime configuration
 * @param[in] temp_C - Core temperature
 *
 * @return scaling factor
 *
 */
float power_vcpu_calc_core_leakage_scaler(power_runconfig_t* p_runconfig, unsigned temp_dC);

/**
 * @brief Calculate Vcpu leakage peak current
 *
 * \b Description:
 *      This API should be called to determine the Vcpu peak current requirement related to the selected
 * plimits for all cores, current SOC temp, etc.  Input is the global power context.
 * @param[in] p_runconfig - power runtime configuration
 * @param[in] power_ctrl_loop_detail_t - control loop runtime configuration
 * @return Current in amps
 *
 */
float power_vcpu_calc_peak_current_A(power_runconfig_t* p_runconfig, power_ctrl_loop_detail_t* loop_config);

/**
 * @brief Calculate per-pstate dynamic and leakage current
 *
 * \b Description:
 *      This API should be called after knob and fuse init to pre-calculate the dynamic and leakage current
 * associated with VF points in all the VF curves.
 * @param[in] p_runconfig - power runtime configuration
 * @param[in] dvfs_config_t - dvfs runtime configuration
 * @return none
 *
 */
void power_vcpu_precalculate_vf_currents(power_runconfig_t* p_runconfig, dvfs_config_t* p_dvfs_cfg);

/**
 * @brief Interpolate a value from given points
 *
 * \b Description:
 *      Included only for test
 *
 * @return Current in amps
 *
 */
uint32_t power_vcpu_interpolate_from_points(const power_vcpu_interp_t* points, unsigned points_count, uint16_t ldo_dac);

int32_t power_fuses_get_curve_temp(int8_t* core_max_temp, uint32_t count);

/**
 * @brief AVS write callback function.
 *
 * \b Description:
 *      Callback function for AVS power write request
 * 
 * @param[in] Request
 * @param[in] CompletionContext
 * @return none
 *
 */
void AVSPwrWriteRequestCompletion(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext);

/**
 * @brief AVS read callback function.
 *
 * \b Description:
 *      Callback function for AVS power read request
 * 
 * @param[in] Request
 * @param[in] CompletionContext
 * @return none
 *
 */

void AVSPwrReadRequestCompletion(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext);

/**
 * @brief AVS write VR knob callback function.
 *
 * \b Description:
 *      Callback function for AVS power init write knob request.  
 *      If a knob is non zero, then the value is written to the voltage regulator.
 * 
 * @param[in] Request
 * @param[in] CompletionContext
 * @return none
 *
 */
void AVSPwrKnobWriteRequestCompletion(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext);

/**
 * @brief Checks if HW has AVS devices.
 *
 * @param[in] None
 * @return true or false
 *
 */
bool power_hw_has_avs_devices(void);

void power_service_dispatch_async(PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_context);

#ifdef __cplusplus
}
#endif
