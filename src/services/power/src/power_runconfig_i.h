//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_runconfig_i.h
 * Header containing internal definitions for power service runtime configuration structures (static, knobs, fuse, etc)
 */

#pragma once

/*----------- Nested includes ------------*/

#include <power_dfwk.h> // for (anonymous), ppower_service_cli_re...
#include <power_runconfig.h>
#include <assert.h>


/*-- Symbolic Constant Macros (defines) --*/

/* define bounds for nominal pstate */
#define NOMINAL_PSTATE_MIN (1)
#define NOMINAL_PSTATE_MAX (MAX_PLIMIT)

/*-------------- Typedefs ----------------*/
typedef void (*power_service_setval)(char* p_string, void* p_set_data);

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


/* Definition of the dictionary entry of parameters that can be queried from the power runconfig structure */
typedef struct {
    power_if_cmd_t id;
    void* p_runconfig_element;
} power_runconfig_read_dictionary_element_t;


/* Definition of the dictionary entry of parameters that can be set in the power runconfig from an external interface */
typedef struct {
    power_if_cmd_t id;
    power_service_setval p_function;
    char* p_string;
} power_runconfig_write_dictionary_element_t;


/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

/* General runtime config APIs */
power_runconfig_t* power_runconfig_get();
void* power_runconfig_get_element(power_if_cmd_t id);
void power_runconfig_set_element(power_if_cmd_t id, void* p_set_data);
void power_runconfig_init(const power_service_config_t* p_config);
dvfs_config_t* power_get_dvfs_config();

/* Knob specific APIs */
void power_knobs_read(power_knobs_t* p_knobs);
void power_knobs_ws_update(power_knobs_t* p_knobs);

/* Fuse specific APIs */
void power_fuses_read(power_fuse_data_t* p_fuses);
int32_t power_fuses_get_curve_assignment(uint32_t core, uint32_t* curve_assignment);

/* Adclk droop count telemetry APIs */
void power_adclk_tel_mutex_lock();
void power_adclk_tel_mutex_unlock();
power_adclk_tel_t* power_get_adclk_telem_ptr();

#ifdef __cplusplus
}
#endif