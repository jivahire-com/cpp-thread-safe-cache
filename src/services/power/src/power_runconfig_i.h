//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_runconfig_i.h
 * Header containing internal definitions for power service runtime configuration structures (static, knobs, fuse, etc)
 */

#pragma once

/*----------- Nested includes ------------*/

#include <power_runconfig.h>
#include <assert.h>

/*-- Symbolic Constant Macros (defines) --*/

/* define bounds for nominal pstate */
#define NOMINAL_PSTATE_MIN (1)
#define NOMINAL_PSTATE_MAX (MAX_PLIMIT)

/*-------------- Typedefs ----------------*/
/**
 * @brief Struct for runtime configuration data
 */
typedef struct _power_runconfig_t
{
    power_knobs_t knobs;
    power_fuse_data_t fuses;
    power_derived_config_t derived;
    power_dvfs_vf_curveset_t dvfs_vft;
    power_vft_curveset_precalc_t precalculated_current;
    const power_service_config_t* p_sconfig;
    void* original_warmstart; // used to store a copy of calculated warmstart data before a warmstart recovery
} power_runconfig_t, *ppower_runconfig_t;

/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

/* General runtime config APIs */
power_runconfig_t* power_runconfig_get();
void power_runconfig_init(const power_service_config_t* p_config);

/* Knob specific APIs */
void power_knobs_read(power_knobs_t* p_knobs);
void power_knobs_ws_update(power_knobs_t* p_knobs);

/* Fuse specific APIs */
void power_fuses_read(power_fuse_data_t* p_fuses);
int32_t power_fuses_get_curve_assignment(uint32_t core, uint32_t* curve_assignment);

#ifdef __cplusplus
}
#endif