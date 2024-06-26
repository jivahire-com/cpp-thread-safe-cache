//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_i.h
 * Header containing internal definitions for power service
 */

#pragma once

/*----------- Nested includes ------------*/
#include <power_runconfig_i.h>
#include <FpFwAssert.h>
#include <stdbool.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MODULE_NAME "[PWRSVC] "
#define NEWLINE     "\n"

#define POWER_LOG_INFO(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define POWER_LOG_WARN(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
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

// Helpers for DTS Coefficient values taken from fuses:
// temp as these should go in pvt_struct.h; use only if not already defined
#define DTS_K_COEFF_FUSED_TEMP(fused_k) (-1.0F * (float)fused_k)
#define DTS_Y_COEFF_FUSED_TEMP(fused_y) ((float)fused_y)
#ifndef TEMP2DOUT_FUSED
    #define TEMP2DOUT_FUSED(t, fused_k, fused_y) \
        (uint32_t)(16384.0F * (t - DTS_K_COEFF_FUSED_TEMP(fused_k)) / DTS_Y_COEFF_FUSED_TEMP(fused_y))
#endif
#ifndef DOUT2TEMP_FUSED
    #define DOUT2TEMP_FUSED(dout, fused_k, fused_y) \
        (dout / 16384.0F * DTS_Y_COEFF_FUSED_TEMP(fused_y) + DTS_K_COEFF_FUSED_TEMP(fused_k))
#endif

/* other helper macros */
#define DIMOF(x) (sizeof(x) / sizeof(x[0]))

/*-------------- Typedefs ----------------*/


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

#ifdef __cplusplus
}
#endif
