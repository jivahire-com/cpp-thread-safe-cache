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
#include "power_runconfig_i.h" // for power_knobs_read

#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <fpfw_cfg_mgr.h>
#include <idsw_kng.h>
#include <kng_soc_constants.h> // for NUM_AP_CORES_PER_DIE
#include <power_i.h>           // for POWER_LOG
#include <silibs_common.h>     // for ARRAY_SIZE
#include <stdbool.h>           // for false, true
#include <stddef.h>            // for NULL

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
#define MILLIWATTS_UINT_TO_WATTS_FLOAT(val_mw) (((float)(val_mw)) / 1000.0f)

/*------------- Functions ----------------*/
void power_knobs_read(power_knobs_t* p_knobs)
{
    FPFW_RUNTIME_ASSERT(p_knobs != NULL);

    p_knobs->pid = config_get_power_pid();
    p_knobs->soc_maximum_thermal_watts_limit =
        config_get_power_soc_maximum_thermal_watts_limit(); // Default to 0 once we have knob + fuse support

    p_knobs->control_loop_interval = config_get_power_control_loop_interval();
    p_knobs->pvt_loop_interval = config_get_power_pvt_loop_interval();
    p_knobs->temp_telemetry_divider = config_get_power_temp_telemetry_divider();

    p_knobs->soc_maximum_electrical_current_limit_vcpu0 = config_get_power_soc_maximum_electrical_current_limit_vcpu0();
    p_knobs->soc_maximum_electrical_current_limit_vcpu1 = config_get_power_soc_maximum_electrical_current_limit_vcpu1();
    p_knobs->r_loadline_vcpu0_uohm = config_get_power_r_loadline_vcpu0();
    p_knobs->r_loadline_vcpu1_uohm = config_get_power_r_loadline_vcpu1();
    p_knobs->vsys_r_loadline_uohm = config_get_power_vsys_r_loadline();
    p_knobs->ldo_offset = config_get_power_ldo_offset();
    p_knobs->activity_factor_dhry_adjustment = config_get_power_activity_factor_dhry_adjustment();

    p_knobs->capping_mode = config_get_power_capping_mode();
    p_knobs->power_enable_velocity_boost = config_get_power_enable_velocity_boost();
    p_knobs->c4_cores_limit_to_nominal = config_get_power_c4_cores_limit_to_nominal();
    p_knobs->c3_cores_limit_to_nominal = config_get_power_c3_cores_limit_to_nominal();
    p_knobs->c2_cores_limit_to_nominal = config_get_power_c2_cores_limit_to_nominal();
    p_knobs->allow_plimit_below_nominal = config_get_power_allow_plimit_below_nominal();
    p_knobs->intervals_to_lower_plimit = config_get_power_intervals_to_lower_plimit();
    p_knobs->allowed_plimit_minimum = config_get_power_allowed_plimit_minimum();
    p_knobs->allowed_plimit_maximum = config_get_power_allowed_plimit_maximum();

    p_knobs->max_plimit_step_size_up = config_get_power_max_plimit_step_size_up();
    p_knobs->max_plimit_step_size_down = config_get_power_max_plimit_step_size_down();

    p_knobs->force_pstate = config_get_power_force_pstate();
    FPFW_RUNTIME_ASSERT(p_knobs->force_pstate <= POWER_KNOB_FORCE_PSTATE_DISABLE);

    p_knobs->current_throt = config_get_power_current_throt();
    p_knobs->current_threshold = config_get_power_plimit_current_thresholds();

    p_knobs->adclk_throt = config_get_power_adclk_throt();

    // we'll index into offset_value, ensure config set it up correctly
    FPFW_RUNTIME_ASSERT(ARRAY_SIZE(p_knobs->adclk_offset.offset_value) >= NUM_AP_CORES_PER_DIE);
    p_knobs->adclk_offset = config_get_power_adclk_offset_cfg();

    p_knobs->tile_temp_throt = config_get_power_tile_temp_throt();
    p_knobs->tile_vm = config_get_power_tile_vms();

    p_knobs->soc_temp = config_get_power_soc_temp();
    p_knobs->soc_vm = config_get_power_soc_vms();

    p_knobs->soc_vm_overvolt_en = config_get_power_soc_vm_overvolt_en();
    p_knobs->soc_vm_undervolt_en = config_get_power_soc_vm_undervolt_en();
    p_knobs->tile_vm_overvolt_en = config_get_power_tile_vm_overvolt_en();
    p_knobs->tile_vm_undervolt_en = config_get_power_tile_vm_undervolt_en();
    p_knobs->soc_temp_hot_en = config_get_power_soc_hot_en();
    p_knobs->soc_temp_thermtrip_en = config_get_power_soc_thermtrip_en();
    p_knobs->tile_temp_hot_en = config_get_power_tile_hot_en();
    p_knobs->tile_temp_thermtrip_en = config_get_power_tile_thermtrip_en();

    if (idsw_get_die_id() == DIE_0)
    {
        p_knobs->vcpu_offset_mv = config_get_power_vcpu0_offset();
    }
    else
    {
        p_knobs->vcpu_offset_mv = config_get_power_vcpu1_offset();
    }

    p_knobs->enable_survivability_mode = config_get_power_enable_survivability_mode();
    p_knobs->survivability_mode_pstate = config_get_power_survivability_mode_pstate();

    p_knobs->calsm_enable = config_get_power_enable_fgpll_calsm();
    p_knobs->fllcal_pstate_bounds = config_get_power_fllcal_pstate_bounds();

    p_knobs->forced_vrs = config_get_power_force_vrs();
    p_knobs->loops_disable = config_get_power_disable_loops();

    if (p_knobs->enable_survivability_mode)
    {
        // ensure maximum VCPU not exceeded in survivability mode (LDO bypass requirement)
        if (p_knobs->forced_vrs.vr[MPCL_VR_VCPU] > POWER_KNOB_VR_FORCED_VCPU_SURVIVABILITY)
        {
            POWER_LOG_WARN(MODULE_NAME "Survivability mode, VCPU0 limited to %d (original %d)",
                           POWER_KNOB_VR_FORCED_VCPU_SURVIVABILITY,
                           p_knobs->forced_vrs.vr[MPCL_VR_VCPU]);
            p_knobs->forced_vrs.vr[MPCL_VR_VCPU] = POWER_KNOB_VR_FORCED_VCPU_SURVIVABILITY;
        }
        if (p_knobs->forced_vrs.vr[MPCL_VR_VCPU1] > POWER_KNOB_VR_FORCED_VCPU_SURVIVABILITY)
        {
            POWER_LOG_WARN(MODULE_NAME "Survivability mode, VCPU1 limited to %d (original %d)",
                           POWER_KNOB_VR_FORCED_VCPU_SURVIVABILITY,
                           p_knobs->forced_vrs.vr[MPCL_VR_VCPU1]);
            p_knobs->forced_vrs.vr[MPCL_VR_VCPU1] = POWER_KNOB_VR_FORCED_VCPU_SURVIVABILITY;
        }
        // If survivabilty mode is enabled, we should disable the control loop
        if (((p_knobs->forced_vrs.vr[MPCL_VR_VCPU] == 0) || (p_knobs->forced_vrs.vr[MPCL_VR_VCPU1] == 0)) &&
            ((p_knobs->loops_disable & power_loops_disable_t_CTRL_LOOP) != power_loops_disable_t_CTRL_LOOP))
        {
            // disable control loop
            POWER_LOG_WARN(MODULE_NAME
                           "Survivability mode, consider forcing value of Vcpu. Disabling control loop.");
            p_knobs->loops_disable |= power_loops_disable_t_CTRL_LOOP;
        }
    }

    p_knobs->minimum_plimit_updates = config_get_power_minimum_plimit_updates();

    p_knobs->leakage_temp_scaler = config_get_power_leakage_temperature_polynomials();

    p_knobs->static_rail_power_watts = MILLIWATTS_UINT_TO_WATTS_FLOAT(config_get_power_soc_static_rails());

    p_knobs->plllock_cfg = config_get_power_plllock_cfg();

    p_knobs->nominal_pstate = config_get_power_nominal_pstate();

    p_knobs->c1_tel_enable = config_get_power_c1_telemetry_enable();

    p_knobs->itd_cfg = config_get_power_itd_cfg();

    p_knobs->es1_fuse_overrides = config_get_power_es1_fuse_overrides();

    p_knobs->rc0_override_for_rc3 = config_get_power_rc0_override_for_rc3();

    p_knobs->avs_ds = config_get_power_avs_ds();

    p_knobs->enable_vsys_vboot_override = config_get_power_enable_vsys_vboot_override();

    p_knobs->cppc_lowest_nonlin_perf = config_get_power_cppc_lowest_nonlin_perf();
}
