//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_knobs.c
 * Implementation of power config reads
 */

/*------------- Includes -----------------*/
#include "power_knobs_i.h"     // for POWER_KNOB_FORCE_PSTATE_DISABLE
#include "power_runconfig.h"   // for power_knobs_t, power_adclk_offset_cfg_t
#include "power_runconfig_i.h" // for power_knobs_read, power_knobs_ws_update

#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <fpfw_cfg_mgr.h>
#include <idsw_kng.h>
#include <kng_soc_constants.h> // for NUM_AP_CORES_PER_DIE
#include <silibs_common.h>     // for ARRAY_SIZE
#include <stdbool.h>           // for false, true
#include <stddef.h>            // for NULL

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
#define MILLIWATTS_UINT_TO_WATTS_FLOAT(val_mw) (((float)val_mw) / 1000.0f)

/*------------- Functions ----------------*/

void power_knobs_ws_update(power_knobs_t* p_knobs)
{
    /* define any knob updates that may depend on state stored in warm start data */
    FPFW_RUNTIME_ASSERT(p_knobs != NULL);

    if (p_knobs->mpmm.enable)
    {
        // FWK_LOG_INFO(MODULE_NAME "Disabled mpmm");
        p_knobs->activity_factor_dhry_adjustment = 80; // config_get_power_activity_factor_mpmm_enabled();
    }
    else
    {
        p_knobs->activity_factor_dhry_adjustment = 100; // config_get_power_activity_factor_mpmm_disabled();
    }
}

void power_knobs_read(power_knobs_t* p_knobs)
{
    // TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491012/
    // this is temp implementation for knob reads
    FPFW_RUNTIME_ASSERT(p_knobs != NULL);

    p_knobs->pid = (power_pid_config_t){.kpt = 15000UL, .kit = 2000000UL, .kdt = 0UL, .setpoint_offset = 5000UL}; // config_get_power_pid();
    p_knobs->soc_maximum_thermal_watts_limit =
        350; // config_get_power_soc_maximum_thermal_watts_limit();  (should default to 0 once we have knob + fuse support)

    p_knobs->control_loop_interval = 10;  // config_get_power_control_loop_interval();
    p_knobs->pvt_loop_interval = 100;     // config_get_power_pvt_loop_interval();
    p_knobs->temp_telemetry_divider = 10; // config_get_power_temp_telemetry_divider();

    p_knobs->soc_maximum_electrical_current_limit_vcpu0 =
        500; // config_get_power_soc_maximum_electrical_current_limit_vcpu0();  // Vcpu max current in amps
    p_knobs->soc_maximum_electrical_current_limit_vcpu1 =
        500; // config_get_power_soc_maximum_electrical_current_limit_vcpu1();  // Vcpu max current in amps
    p_knobs->r_loadline_vcpu0_uohm = 400; // config_get_power_r_loadline_vcpu0(); // (uOhm)
    p_knobs->r_loadline_vcpu1_uohm = 400; // config_get_power_r_loadline_vcpu1(); // (uOhm)
    p_knobs->vsys_r_loadline_uohm = 300; // config_get_power_vsys_r_loadline();                       // (uOhm)
    p_knobs->ldo_offset = 0;             // config_get_power_ldo_offset();
    p_knobs->activity_factor_dhry_adjustment = 100; // activity factor with mpmm disabled (default)

    p_knobs->mpmm = (power_mpmm_config_t){.enable = false, .gear = 2}; // config_get_power_mpmm();

    p_knobs->capping_mode = config_get_power_capping_mode();
    p_knobs->power_enable_velocity_boost = true; // config_get_power_enable_velocity_boost();
    p_knobs->c4_cores_limit_to_nominal = true;   // config_get_power_c3_cores_limit_to_nominal();
    p_knobs->c3_cores_limit_to_nominal = true;   // config_get_power_c3_cores_limit_to_nominal();
    p_knobs->c2_cores_limit_to_nominal = true;   // config_get_power_c2_cores_limit_to_nominal();
    p_knobs->allow_plimit_below_nominal = false; // config_get_power_allow_plimit_below_nominal();
    p_knobs->intervals_to_lower_plimit = 0;      // config_get_power_intervals_to_lower_plimit();
    p_knobs->allowed_plimit_minimum = 0;         // config_get_power_allowed_plimit_minimum();
    p_knobs->allowed_plimit_maximum = 31;        // config_get_power_allowed_plimit_maximum();

    p_knobs->max_plimit_step_size_up = 31;   // config_get_power_max_plimit_step_size_up();
    p_knobs->max_plimit_step_size_down = 31; // config_get_power_max_plimit_step_size_down();

    p_knobs->force_pstate = 32;
    FPFW_RUNTIME_ASSERT(p_knobs->force_pstate <= POWER_KNOB_FORCE_PSTATE_DISABLE);

    p_knobs->current_throt = (power_currthrot_cfg_t){.rolling_window_count = 5,
                                                     .telemetry_epoch_count = 10,
                                                     .inc_ctr = 31,
                                                     .dec_ctr = 31,
                                                     .inc_amt = 0,
                                                     .dec_amt0 = 1,
                                                     .dec_amt1 = 2}; // config_get_power_current_throt();
    p_knobs->current_threshold =
        (power_currthrot_threshold_cfg_t){.iref_to_max_percent = 80, .t1_percent = 90, .t2_percent = 95, .t3_percent = 100}; // config_get_power_plimit_current_thresholds();

    p_knobs->adclk_throt = (power_adclk_cfg_t){.enable = false,
                                               .telemetry_interval = 0,
                                               .inc_ctr = 31,
                                               .dec_ctr = 31,
                                               .inc_amt = 0,
                                               .dec_amt = 1,
                                               .resp_option = 0,
                                               .recovery_step_wait = 0,
                                               .recovery_step = 2,
                                               .resp_wait = 16,
                                               .resp_code = 16,
                                               .adclk_comp_config = 0,
                                               .throttle_threshold = 10,
                                               .throttle_window_count = 50}; // config_get_power_adclk_throt();
    // we'll index into offset_value, ensure config set it up correctly
    FPFW_RUNTIME_ASSERT(ARRAY_SIZE(p_knobs->adclk_offset.offset_value) >= NUM_AP_CORES_PER_DIE);
    p_knobs->adclk_offset = (power_adclk_offset_cfg_t){
        .enable = false,
        .offset_value = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}; // config_get_power_adclk_offset_cfg();

    p_knobs->tile_temp_throt =
        (power_tile_tempthrot_cfg_t){.hot = {.hyst_threshold = 930, .alarm_threshold = 950},
                                     .thermtrip = {.hyst_threshold = 1130, .alarm_threshold = 1150},
                                     .inc_ctr = 15,
                                     .dec_ctr = 15,
                                     .inc_amt = 0,
                                     .dec_amt = 1}; // config_get_power_tile_temp_throt();
    p_knobs->tile_vm = (power_pvt_tile_vm_cfg_t){
        .thresholds = {{.overvolt = {.hyst_threshold = 1050, .alarm_threshold = 1100},
                        .undervolt = {.hyst_threshold = 0, .alarm_threshold = 0}},
                       {.overvolt = {.hyst_threshold = 1050, .alarm_threshold = 1100},
                        .undervolt = {.hyst_threshold = 0, .alarm_threshold = 0}},
                       {.overvolt = {.hyst_threshold = 1383, .alarm_threshold = 1433},
                        .undervolt = {.hyst_threshold = 680, .alarm_threshold = 630}},
                       {.overvolt = {.hyst_threshold = 1006, .alarm_threshold = 1056},
                        .undervolt = {.hyst_threshold = 775, .alarm_threshold = 725}}}}; // config_get_power_tile_vms();

    p_knobs->soc_temp = (power_soc_temp_cfg_t){.hot = {.hyst_threshold = 930, .alarm_threshold = 950},
                                               .thermtrip = {.hyst_threshold = 1130, .alarm_threshold = 1150}}; // config_get_power_soc_temp();
    p_knobs->soc_vm = (power_pvt_soc_vm_cfg_t){
        .thresholds = {{.overvolt = {.hyst_threshold = 1281, .alarm_threshold = 1331},
                        .undervolt = {.hyst_threshold = 941, .alarm_threshold = 891}},
                       {.overvolt = {.hyst_threshold = 858, .alarm_threshold = 908},
                        .undervolt = {.hyst_threshold = 689, .alarm_threshold = 639}},
                       {.overvolt = {.hyst_threshold = 933, .alarm_threshold = 983},
                        .undervolt = {.hyst_threshold = 777, .alarm_threshold = 727}},
                       {.overvolt = {.hyst_threshold = 858, .alarm_threshold = 908},
                        .undervolt = {.hyst_threshold = 689, .alarm_threshold = 639}},
                       {.overvolt = {.hyst_threshold = 1006, .alarm_threshold = 1056},
                        .undervolt = {.hyst_threshold = 775, .alarm_threshold = 725}},
                       {.overvolt = {.hyst_threshold = 1281, .alarm_threshold = 1331},
                        .undervolt = {.hyst_threshold = 941, .alarm_threshold = 891}},
                       {.overvolt = {.hyst_threshold = 933, .alarm_threshold = 983},
                        .undervolt = {.hyst_threshold = 777, .alarm_threshold = 727}},
                       {.overvolt = {.hyst_threshold = 1006, .alarm_threshold = 1056},
                        .undervolt = {.hyst_threshold = 775, .alarm_threshold = 725}}}}; // config_get_power_soc_vms();

    // TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491012/
    // replace with platform override when we get knob support
    if IS_PLATFORM_FPGA ()
    {
        // FPGA has random values for temp/vm pvts, so disable the related alarms
        p_knobs->soc_vm_overvolt_en = false;     // config_get_power_soc_vm_overvolt_en();
        p_knobs->soc_vm_undervolt_en = false;    // config_get_power_soc_vm_undervolt_en();
        p_knobs->tile_vm_overvolt_en = false;    // config_get_power_tile_vm_overvolt_en();
        p_knobs->tile_vm_undervolt_en = false;   // config_get_power_tile_vm_undervolt_en();
        p_knobs->soc_temp_hot_en = false;        // config_get_power_soc_hot_en();
        p_knobs->soc_temp_thermtrip_en = false;  // config_get_power_soc_thermtrip_en();
        p_knobs->tile_temp_hot_en = false;       // config_get_power_tile_hot_en();
        p_knobs->tile_temp_thermtrip_en = false; // config_get_power_tile_thermtrip_en();
    }
    else
    {
        p_knobs->soc_vm_overvolt_en = true;     // config_get_power_soc_vm_overvolt_en();
        p_knobs->soc_vm_undervolt_en = true;    // config_get_power_soc_vm_undervolt_en();
        p_knobs->tile_vm_overvolt_en = true;    // config_get_power_tile_vm_overvolt_en();
        p_knobs->tile_vm_undervolt_en = true;   // config_get_power_tile_vm_undervolt_en();
        p_knobs->soc_temp_hot_en = true;        // config_get_power_soc_hot_en();
        p_knobs->soc_temp_thermtrip_en = true;  // config_get_power_soc_thermtrip_en();
        p_knobs->tile_temp_hot_en = true;       // config_get_power_tile_hot_en();
        p_knobs->tile_temp_thermtrip_en = true; // config_get_power_tile_thermtrip_en();
    }

    p_knobs->vcpu_offset_mv = 0; // config_get_power_vcpu_offset();

    p_knobs->enable_survivability_mode = false; // config_get_power_enable_survivability_mode();
    p_knobs->survivability_mode_pstate = 31;    // config_get_power_survivability_mode_pstate();

    p_knobs->calsm_enable = true; // config_get_power_enable_fgpll_calsm();
    p_knobs->fllcal_pstate_bounds = (power_fllcal_pstate_bounds_t){0, 31}; // config_get_power_ftable_pstate_cap();

    p_knobs->forced_vrs = (power_force_vrs_t){.vr = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}; // config_get_power_force_vrs();
    p_knobs->loops_disable = power_loops_disable_t_NONE; // config_get_power_disable_loops();

    if (p_knobs->enable_survivability_mode)
    {
        // TODO: complete survivability mode: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1809001
        // ensure maximum VCPU not exceeded in survivability mode (LDO bypass requirement)

        // temporarily, just disable control loop
        p_knobs->loops_disable |= power_loops_disable_t_CTRL_LOOP;
    }

    p_knobs->minimum_plimit_updates = power_loops_minimum_plimit_t_MIN_64; // config_get_power_minimum_plimit_updates();

    p_knobs->leakage_temp_scaler = (power_leakage_temp_scaler_t){
        .poly_coefficients = {{.a = 3e-06F, .b = -0.0003F, .c = 0.0141F, .d = -0.1239F},
                              {.a = 3e-06F, .b = -0.0003F, .c = 0.0141F, .d = -0.1239F},
                              {.a = 3e-06F, .b = -0.0003F, .c = 0.0141F, .d = -0.1239F},
                              {.a = 3e-06F, .b = -0.0003F, .c = 0.0141F, .d = -0.1239F}}}; // config_get_power_leakage_temperature_polynomials();

    p_knobs->static_rail_power_watts = MILLIWATTS_UINT_TO_WATTS_FLOAT(0 /*config_get_power_soc_static_rails()*/);

    p_knobs->plllock_cfg = (power_plllock_cfg_t){.enable_error = true,
                                                 .lckcntsel = 2,
                                                 .mask_plllockraw = true,
                                                 .mask_freqchangetimeout = false,
                                                 .mask_fllcaltimeout = false,
                                                 .mask_locktimeout = false}; // config_get_power_plllock_cfg();

    p_knobs->nominal_pstate = 16; // config_get_power_nominal_pstate(); (should default to 0 once we have knob + fuse support)

    p_knobs->c1_tel_enable = false; // config_get_power_c1_telemetry_enable();

    // perform the knob updates for p_knobs which can also be updated after warmstart restoration
    power_knobs_ws_update(p_knobs);
}
