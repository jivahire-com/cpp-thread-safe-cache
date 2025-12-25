//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_power_config.c
 * Source file to implement power config CLI commands.
 */

/*------------- Includes -----------------*/
#include <FpFwCli.h>
#include <FpFwUtils.h>
#include <cli_power_common.h>
#include <cli_power_config.h>
#include <cli_power_interface.h>
#include <corebits.h>
#include <dvfs.h>
#include <power_dfwk.h>
#include <power_runconfig.h>
#include <power_runconfig_i.h>
#include <silibs_common.h>
#include <string.h>
#include <tx_api.h>
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/
#define DFVS_FUSED_COREMEMASST_COUNT    5
#define INFO_ROWS                       3
#define POWER_KNOB_FORCE_PSTATE_DISABLE 32

/*-- Declarations (Statics and globals) --*/
static const char* s_true_str = "true";
static const char* s_false_str = "false";

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
static void print_power_config_fuse(power_fuse_data_t* fuses);
static void print_power_config_dts(power_fuse_data_t* fuses);
static void print_power_config_memasst(power_fuse_data_t* fuses);
static void print_power_config_interval(power_knobs_t* knobs);
static void print_power_config_limits(power_knobs_t* knobs);
static void print_power_config_pidcfg(power_knobs_t* knobs);
static void print_power_config_thresholds(power_knobs_t* knobs);
static void print_power_config_loopcfg(power_knobs_t* knobs);
static void print_power_config_survivability_mode(power_knobs_t* knobs);
static void print_power_config_static_rails(power_knobs_t* knobs);
static void print_power_config_tel(power_knobs_t* knobs);
static void print_power_config_throttling(power_knobs_t* knobs);
static void print_power_config_fgpll(power_knobs_t* knobs);
static void print_power_config_knobs(power_knobs_t* knobs);
static void print_power_config_max_allowed_plimit(power_knobs_t* knobs);
static void print_power_config_min_allowed_plimit(power_knobs_t* knobs);
static void print_power_config_vf(power_fuse_data_t* fuses);
static void print_power_config_vft(power_runconfig_t* p_runconfig);
static void print_power_config_vftpre(power_vft_curveset_precalc_t* precalculated_current);
static void print_power_config_vcpucalc(power_knobs_t* knobs);
static void print_power_tdp_fuse(power_fuse_data_t* fuses);
static void print_ldodac_to_volt_slope_fuse(power_fuse_data_t* fuses);
static void print_power_fuse_vcpu_interp_data(const power_fuse_data_t* fuses);
static void print_power_avs_ds_cfg(power_knobs_t* knobs);
static void print_power_leakage_temp_scaler(power_knobs_t* knobs);
static void print_power_force_vrs(power_knobs_t* knobs);
static void print_power_adclk_offset_cfg(power_knobs_t* knobs);
static void print_power_config_vsys_override(power_knobs_t* knobs);
static void print_power_config_vsys_vid(power_fuse_data_t* fuses);
static void print_power_config_cppc_lowest_nonlin_perf(power_knobs_t* knobs);

/*-- Declarations (Statics and globals) --*/

// clang-format off
/* Structure storing the dictionary of sub-command string against its corresponding print function */
const power_cli_sub_command_dictionary_element_t power_cli_config_sub_command_dictionary[] = {
    {"fuse",                (print_function)print_power_config_fuse,                POWER_IF_CMD_GET_RUNCONFIG_FUSES},
    {"tdp",                 (print_function)print_power_tdp_fuse,                   POWER_IF_CMD_GET_RUNCONFIG_FUSES},
    {"ldo2volt",            (print_function)print_ldodac_to_volt_slope_fuse,        POWER_IF_CMD_GET_RUNCONFIG_FUSES},
    {"vcpuinterp",          (print_function)print_power_fuse_vcpu_interp_data,      POWER_IF_CMD_GET_RUNCONFIG_FUSES},
    {"dts",                 (print_function)print_power_config_dts,                 POWER_IF_CMD_GET_RUNCONFIG_FUSES},
    {"memasst",             (print_function)print_power_config_memasst,             POWER_IF_CMD_GET_RUNCONFIG_FUSES},
    {"intervals",           (print_function)print_power_config_interval,            POWER_IF_CMD_GET_RUNCONFIG_KNOBS},
    {"limits",              (print_function)print_power_config_limits,              POWER_IF_CMD_GET_RUNCONFIG_KNOBS},
    {"pid",                 (print_function)print_power_config_pidcfg,              POWER_IF_CMD_GET_RUNCONFIG_KNOBS},
    {"pvt",                 (print_function)print_power_config_thresholds,          POWER_IF_CMD_GET_RUNCONFIG_KNOBS},
    {"ctrlloop",            (print_function)print_power_config_loopcfg,             POWER_IF_CMD_GET_RUNCONFIG_KNOBS},
    {"srvmode",             (print_function)print_power_config_survivability_mode,  POWER_IF_CMD_GET_RUNCONFIG_KNOBS},
    {"tel",                 (print_function)print_power_config_tel,                 POWER_IF_CMD_GET_RUNCONFIG_KNOBS},
    {"throttle",            (print_function)print_power_config_throttling,          POWER_IF_CMD_GET_RUNCONFIG_KNOBS},
    {"static",              (print_function)print_power_config_static_rails,        POWER_IF_CMD_GET_RUNCONFIG_KNOBS},
    {"allowed_min_plimit",  (print_function)print_power_config_min_allowed_plimit,  POWER_IF_CMD_GET_RUNCONFIG_KNOBS},
    {"allowed_max_plimit",  (print_function)print_power_config_max_allowed_plimit,  POWER_IF_CMD_GET_RUNCONFIG_KNOBS},
    {"fgpll",               (print_function)print_power_config_fgpll,               POWER_IF_CMD_GET_RUNCONFIG_KNOBS},
    {"avs_ds",              (print_function)print_power_avs_ds_cfg,                 POWER_IF_CMD_GET_RUNCONFIG_KNOBS},
    {"lkg_temp_scaler",     (print_function)print_power_leakage_temp_scaler,        POWER_IF_CMD_GET_RUNCONFIG_KNOBS},
    {"force_vrs",           (print_function)print_power_force_vrs,                  POWER_IF_CMD_GET_RUNCONFIG_KNOBS},
    {"adclk_offset",        (print_function)print_power_adclk_offset_cfg,           POWER_IF_CMD_GET_RUNCONFIG_KNOBS},
    {"knobs",               (print_function)print_power_config_knobs,               POWER_IF_CMD_GET_RUNCONFIG_KNOBS},
    {"vf",                  (print_function)print_power_config_vf,                  POWER_IF_CMD_GET_RUNCONFIG_FUSES},
    {"vft",                 (print_function)print_power_config_vft,                 POWER_IF_CMD_GET_RUNCONFIG},
    {"vftpre",              (print_function)print_power_config_vftpre,              POWER_IF_CMD_GET_RUNCONFIG_VFTPRE},
    {"vcpucalc",            (print_function)print_power_config_vcpucalc,            POWER_IF_CMD_GET_RUNCONFIG_KNOBS},
    {"vsys_override",       (print_function)print_power_config_vsys_override,       POWER_IF_CMD_GET_RUNCONFIG_KNOBS},
    {"vsys_vid",            (print_function)print_power_config_vsys_vid,            POWER_IF_CMD_GET_RUNCONFIG_FUSES},
    {"cppc_lowest_nonlin",  (print_function)print_power_config_cppc_lowest_nonlin_perf,  POWER_IF_CMD_GET_RUNCONFIG_KNOBS},
};

//clang-format on

const uint32_t length_dictionary =
    sizeof(power_cli_config_sub_command_dictionary) / sizeof(power_cli_sub_command_dictionary_element_t);

/*-------------- Functions ---------------*/
static PLACED_CODE void print_padding(uint32_t decimal, unsigned places)
{
    uint32_t value = 1;
    for (unsigned i = 0; i < places; ++i)
    {
        value *= 10;
        if (decimal < value)
        {
            FpFwCliPrint(" ");
        }
    }
}

static PLACED_CODE void print_power_tdp_fuse(power_fuse_data_t* fuses)
{
    FpFwCliPrint("\nTDP Fuse configs\n");
    FpFwCliPrint("-----------------\n");
    FpFwCliPrint("TDP cores  : %d\n", fuses->tdp_config.num_cores);
    FpFwCliPrint("TDP power  : %d W\n", fuses->tdp_config.power_A);
    FpFwCliPrint("TDP freq   : %d MHz\n", fuses->tdp_config.freq_MHz);
    FpFwCliPrint("Valid cores: " COREBITS_FMT_STR "\n", COREBITS_FMT_DATA(fuses->valid_cores));

    FpFwCliPrint("\n");
}

static PLACED_CODE void print_ldodac_to_volt_slope_fuse(power_fuse_data_t* fuses)
{
    FpFwCliPrint("\nLDODAC to mV Slope Fuse\n");
    FpFwCliPrint("------------------------\n");
    FpFwCliPrint("Slope (uV/LSB): %u\n", fuses->ldodac_to_volt.slope_uvolt);
    FpFwCliPrint("Offset (uV): %d\n", fuses->ldodac_to_volt.offset_uvolt);
    FpFwCliPrint("\n");
}

static PLACED_CODE void print_dts_coeffs(dts_coeff_t* coeff, unsigned count)
{
    for (unsigned int idx = 0; idx < count; ++idx)
    {
        FpFwCliPrint(" %02d y: %d k: %d\n", idx, coeff[idx].y_val, coeff[idx].k_val);
    }
}

static PLACED_CODE void print_power_config_dts(power_fuse_data_t* fuses)
{
    FpFwCliPrint("\nDTS Coefficients\n");
    FpFwCliPrint("----------------\n");
    FpFwCliPrint("\nTile\n");
    print_dts_coeffs(fuses->dts_coeff_tile, ARRAY_SIZE(fuses->dts_coeff_tile));
    FpFwCliPrint("\nSoCTOP\n");
    print_dts_coeffs(fuses->dts_coeff_soctop, ARRAY_SIZE(fuses->dts_coeff_soctop));
    FpFwCliPrint("\n");
}

static PLACED_CODE void print_power_config_memasst(power_fuse_data_t* fuses)
{
    FpFwCliPrint("\n");
    // Print table header
    FpFwCliPrint("+--------+---------+-------+-----------+----------+--------+--------+---------+\n");
    FpFwCliPrint("|  LDO   | hdrawlm | hdema | hshcrawlm | hshscema | tpemaa | tpemab | hd_emaw |\n");
    FpFwCliPrint("+--------+---------+-------+-----------+----------+--------+--------+---------+\n");
    for (unsigned r_idx = 0; r_idx < DFVS_FUSED_COREMEMASST_COUNT; ++r_idx)
    {
        if (!fuses->memasst.entry[r_idx].valid_boundary)
        {
            FpFwCliPrint("|   ---  |   ---   |  ---  |    ---    |   ---    |  ---   |  ---   |   ---   |\n");
            continue;
        }
        unsigned ldo = fuses->memasst.entry[r_idx].ldo_dac_in;
        FpFwCliPrint("| %6d | %7d | %5d | %9d | %8d | %6d | %6d | %7d |\n",
               ldo,
               fuses->memasst.entry[r_idx].hd_rawlm,
               fuses->memasst.entry[r_idx].hd_ema,
               fuses->memasst.entry[r_idx].hshc_rawlm,
               fuses->memasst.entry[r_idx].hshc_ema,
               fuses->memasst.entry[r_idx].tp_emaa,
               fuses->memasst.entry[r_idx].tp_emab,
               fuses->memasst.entry[r_idx].hd_emaw);
    }
    FpFwCliPrint("+--------+---------+-------+-----------+----------+--------+--------+---------+\n\n");
}

static PLACED_CODE void print_power_fuse_vcpu_interp_data(const power_fuse_data_t* fuses)
{
    FpFwCliPrint("\nVCPU Leakage Points\n");
    FpFwCliPrint("-------------------\n");
    FpFwCliPrint("Idx  LDO_DAC  Current(mA)  VCPU(mV)  Temp_Offset\n");
    for (unsigned i = 0; i < VCPU_LEAKAGE_COUNT; ++i) {
        const power_fuse_vcpu_current_t* v = &fuses->vcpu_leakage[i].current;
        FpFwCliPrint("%2u   %6u   %10u   %7u   %11u\n", i, v->ldo_dac, v->current_ma, v->vcpu_mv, v->temp_offset);
    }

    FpFwCliPrint("\nVCPU LDO Dynamic Points\n");
    FpFwCliPrint("-----------------------\n");
    FpFwCliPrint("Idx  LDO_DAC  Current(mA)  VCPU(mV)  Temp_Offset\n");
    for (unsigned i = 0; i < VCPU_LDO_DYNAMIC_COUNT; ++i) {
        const power_fuse_vcpu_current_t* v = &fuses->vcpu_ldo_dyn[i].current;
        FpFwCliPrint("%2u   %6u   %10u   %7u   %11u\n", i, v->ldo_dac, v->current_ma, v->vcpu_mv, v->temp_offset);
    }

    FpFwCliPrint("\nCore Cdyn Points\n");
    FpFwCliPrint("-----------------\n");
    FpFwCliPrint("Idx  LDO_DAC  Cdyn(pF)\n");
    for (unsigned i = 0; i < CORE_CDYN_COUNT; ++i) {
        const power_fuse_core_cdyn_t* c = &fuses->core_cdyn[i].cdyn;
        FpFwCliPrint("%2u   %6u   %9u\n", i, c->ldo_dac, c->cdyn_pf);
    }
    FpFwCliPrint("\n");
}

static PLACED_CODE void print_power_config_vsys_vid(power_fuse_data_t* fuses)
{
    FpFwCliPrint("\nVSYS VID Fuse\n");
    FpFwCliPrint("--------------\n");
    FpFwCliPrint("VSYS VID (mV): %u\n", fuses->vsys_vid_mv);
    FpFwCliPrint("\n");
}

static PLACED_CODE void print_power_config_fuse(power_fuse_data_t* fuses)
{
    FpFwCliPrint("\n---------------------------All Power Fuses---------------------------\n");
    FpFwCliPrint("LDO Dropout Voltage (mV): %u\n", fuses->v_ldo_dropout_mv);
    FpFwCliPrint("VCPU Guardband (mV): %u\n", fuses->vcpu_guardband_mv);
    FpFwCliPrint("Curve Max Temps (C):");
    for (unsigned i = 0; i < ARRAY_SIZE(fuses->curve_max_temp); ++i) {
        FpFwCliPrint(" %d", fuses->curve_max_temp[i]);
    }
    FpFwCliPrint("\n");
    static const char* process_names[] = { "Unknown0", "SS", "TT", "FF", "Unknown" };
    const char* process_str = "Unknown";
    if (fuses->process_id >= PROCESS_SS && fuses->process_id <= PROCESS_FF) {
        process_str = process_names[fuses->process_id];
    } else if (fuses->process_id == PROCESS_UNKNOWN_0) {
        process_str = process_names[0];
    } else if (fuses->process_id == PROCESS_UNKNOWN) {
        process_str = process_names[4];
    }
    FpFwCliPrint("Process ID: %s (%d)\n", process_str, fuses->process_id);
    print_power_tdp_fuse(fuses);
    print_power_config_vf(fuses);
    print_power_config_memasst(fuses);
    print_ldodac_to_volt_slope_fuse(fuses);
    print_power_config_dts(fuses);
    print_power_fuse_vcpu_interp_data(fuses);
}

static PLACED_CODE void print_power_config_interval(power_knobs_t* knobs)
{
    FpFwCliPrint("\nConfigured Loop Intervals\n");
    FpFwCliPrint("-------------------------\n");
    FpFwCliPrint("Cntrl. loop    Temp telem. divider    PVT loop\n");
    print_padding(knobs->control_loop_interval, 6);
    FpFwCliPrint("%ums    ", knobs->control_loop_interval);
    print_padding(knobs->temp_telemetry_divider, 6);
    FpFwCliPrint("%u ", knobs->temp_telemetry_divider);
    print_padding(knobs->temp_telemetry_divider * knobs->control_loop_interval, 6);
    FpFwCliPrint("(%ums)    ", knobs->temp_telemetry_divider * knobs->control_loop_interval);
    print_padding(knobs->pvt_loop_interval, 5);
    FpFwCliPrint("%ums\n", knobs->pvt_loop_interval);
    FpFwCliPrint("\n");
}

static PLACED_CODE void print_power_config_limits(power_knobs_t* knobs)
{
    FpFwCliPrint("\nConfigured Control Loop Limits\n");
    FpFwCliPrint("------------------------------\n");
    FpFwCliPrint("Thermal/watt    Vcpu0 current   Vcpu1 current\n");
    print_padding(knobs->soc_maximum_thermal_watts_limit, 6);
    FpFwCliPrint("%uW     ", knobs->soc_maximum_thermal_watts_limit);
    print_padding(knobs->soc_maximum_electrical_current_limit_vcpu0, 7);
    FpFwCliPrint("%uA    ", knobs->soc_maximum_electrical_current_limit_vcpu0);
    print_padding(knobs->soc_maximum_electrical_current_limit_vcpu1, 8);
    FpFwCliPrint("%uA", knobs->soc_maximum_electrical_current_limit_vcpu1);
    FpFwCliPrint("\n");
}

static PLACED_CODE void print_power_config_pidcfg(power_knobs_t* knobs)
{
    FpFwCliPrint("\nCtrl loop PID config.\n");
    FpFwCliPrint("--------------------\n");
    FpFwCliPrint("Kp : (%u.%03u)\n", (unsigned int)knobs->pid.kpt / 1000, (unsigned int)knobs->pid.kpt % 1000);
    FpFwCliPrint("Ki : (%u.%03u)\n", (unsigned int)knobs->pid.kit / 1000, (unsigned int)knobs->pid.kit % 1000);
    FpFwCliPrint("Kd : (%u.%03u)\n", (unsigned int)knobs->pid.kdt / 1000, (unsigned int)knobs->pid.kdt % 1000);
    FpFwCliPrint("Setpoint offset : %umW\n", (unsigned int)knobs->pid.setpoint_offset);
    FpFwCliPrint("\n");
}

static PLACED_CODE void print_voltage(uint16_t voltage)
{
    print_padding(voltage, 4);
    FpFwCliPrint("%dmV", voltage);
}

static PLACED_CODE void print_temp(uint16_t temp)
{
    uint16_t tenths = temp % 10;
    temp = temp / 10;
    print_padding(temp, 3);
    FpFwCliPrint("%d.%dC", temp, tenths);
}

static PLACED_CODE void print_power_config_thresholds(power_knobs_t* knobs)
{
    FpFwCliPrint("\nAlarm Status\n");
    FpFwCliPrint("----------------\n");
    FpFwCliPrint("SOC VM Overvolt Alarm    : %s\n", knobs->soc_vm_overvolt_en ? "enabled" : "disabled");
    FpFwCliPrint("SOC VM Undervolt Alarm   : %s\n", knobs->soc_vm_undervolt_en ? "enabled" : "disabled");
    FpFwCliPrint("Tile VM Overvolt Alarm   : %s\n", knobs->tile_vm_overvolt_en ? "enabled" : "disabled");
    FpFwCliPrint("Tile VM Undervolt Alarm  : %s\n", knobs->tile_vm_undervolt_en ? "enabled" : "disabled");
    FpFwCliPrint("SOC Temp Hot Alarm       : %s\n", knobs->soc_temp_hot_en ? "enabled" : "disabled");
    FpFwCliPrint("SOC Temp Thermtrip Alarm : %s\n", knobs->soc_temp_thermtrip_en ? "enabled" : "disabled");
    FpFwCliPrint("Tile Temp Hot Alarm      : %s\n", knobs->tile_temp_hot_en ? "enabled" : "disabled");
    FpFwCliPrint("Tile Temp Thermtrip Alarm: %s\n", knobs->tile_temp_thermtrip_en ? "enabled" : "disabled");
    FpFwCliPrint("\n");

    FpFwCliPrint("\nPVT Thresholds\n");
    FpFwCliPrint("--------------\n");
    FpFwCliPrint("Tile\n");
    FpFwCliPrint("  Hot (hyst/alarm)      : ");
    print_temp(knobs->tile_temp_throt.hot.hyst_threshold);
    FpFwCliPrint(" / ");
    print_temp(knobs->tile_temp_throt.hot.alarm_threshold);
    FpFwCliPrint("\n");
    FpFwCliPrint("  Thermtrip (hyst/alarm): ");
    print_temp(knobs->tile_temp_throt.thermtrip.hyst_threshold);
    FpFwCliPrint(" / ");
    print_temp(knobs->tile_temp_throt.thermtrip.alarm_threshold);
    FpFwCliPrint("\n");

    for (int vm_idx = 0; vm_idx < TILE_PVT_NUM_CHANNELS_VM; ++vm_idx)
    {
        FpFwCliPrint("  VM%d OV    (hyst/alarm): ", vm_idx);
        print_voltage(knobs->tile_vm.thresholds[vm_idx].overvolt.hyst_threshold);
        FpFwCliPrint(" / ");
        print_voltage(knobs->tile_vm.thresholds[vm_idx].overvolt.alarm_threshold);
        FpFwCliPrint("\n");
        FpFwCliPrint("  VM%d UV    (hyst/alarm): ", vm_idx);
        print_voltage(knobs->tile_vm.thresholds[vm_idx].undervolt.hyst_threshold);
        FpFwCliPrint(" / ");
        print_voltage(knobs->tile_vm.thresholds[vm_idx].undervolt.alarm_threshold);
        FpFwCliPrint("\n");
    }

    FpFwCliPrint("SOC\n");
    FpFwCliPrint("  Hot (hyst/alarm)      : ");
    print_temp(knobs->soc_temp.hot.hyst_threshold);
    FpFwCliPrint(" / ");
    print_temp(knobs->soc_temp.hot.alarm_threshold);
    FpFwCliPrint("\n");
    FpFwCliPrint("  Thermtrip (hyst/alarm): ");
    print_temp(knobs->soc_temp.thermtrip.hyst_threshold);
    FpFwCliPrint(" / ");
    print_temp(knobs->soc_temp.thermtrip.alarm_threshold);
    FpFwCliPrint("\n");

    for (int vm_idx = 0; vm_idx < SOC_PVT_TOTAL_CHANNELS_VM; ++vm_idx)
    {
        FpFwCliPrint("  VM%d OV    (hyst/alarm): ", vm_idx);
        print_voltage(knobs->soc_vm.thresholds[vm_idx].overvolt.hyst_threshold);
        FpFwCliPrint(" / ");
        print_voltage(knobs->soc_vm.thresholds[vm_idx].overvolt.alarm_threshold);
        FpFwCliPrint("\n");
        FpFwCliPrint("  VM%d UV    (hyst/alarm): ", vm_idx);
        print_voltage(knobs->soc_vm.thresholds[vm_idx].undervolt.hyst_threshold);
        FpFwCliPrint(" / ");
        print_voltage(knobs->soc_vm.thresholds[vm_idx].undervolt.alarm_threshold);
        FpFwCliPrint("\n");
    }
    FpFwCliPrint("\n");
}

static PLACED_CODE void print_power_config_loopcfg(power_knobs_t* knobs)
{
    FpFwCliPrint("\nCtrl loop configs\n");
    FpFwCliPrint("-----------------\n");
    FpFwCliPrint("Pwr capping mode : %s\n", knobs->capping_mode == power_capping_mode_t_ALL ? "all" : "per VM");
    FpFwCliPrint("Power Loops Disable Status : %s\n", knobs->loops_disable == power_loops_disable_t_NONE ? "All power module loops enabled." : "Some power module loops disabled.");
    if (knobs->loops_disable & power_loops_disable_t_CTRL_LOOP)
    {
        FpFwCliPrint("Power control loop disabled.\n");
    }
    if (knobs->loops_disable & power_loops_disable_t_VR_TELEM_LOOP)
    {
        FpFwCliPrint("Power VR telemetry loop disabled.\n");
    }
    if (knobs->loops_disable & power_loops_disable_t_PVT_TELEM_LOOP)
    {
        FpFwCliPrint("Power PVT telemetry loop disabled.\n");
    }
    FpFwCliPrint("Nominal Pstate : %u Frequency : %d MHz\n", knobs->nominal_pstate, dvfs_get_freq_from_plimit(knobs->nominal_pstate));
    FpFwCliPrint("C2 limits to nominal : %s\n", knobs->c2_cores_limit_to_nominal ? s_true_str : s_false_str);
    FpFwCliPrint("C3 limits to nominal : %s\n", knobs->c3_cores_limit_to_nominal ? s_true_str : s_false_str);
    FpFwCliPrint("Allow plimit < nominal : %s\n", knobs->allow_plimit_below_nominal ? s_true_str : s_false_str);
    FpFwCliPrint("FLLCap Pstate Bounds : min=%u, max=%u\n", knobs->fllcal_pstate_bounds.pstate_min, knobs->fllcal_pstate_bounds.pstate_max);
    FpFwCliPrint("Velocity Boost status : %s\n", knobs->power_enable_velocity_boost ? "enabled" : "disabled");
    FpFwCliPrint("Intervals to lwr : %u\n", knobs->intervals_to_lower_plimit);
    FpFwCliPrint("Allwd plimit min : %u\n", knobs->allowed_plimit_minimum);
    FpFwCliPrint("Allwd plimit max : %u\n", knobs->allowed_plimit_maximum);
    FpFwCliPrint("Step size up max : %u\n", knobs->max_plimit_step_size_up);
    FpFwCliPrint("Step size dn max : %u\n", knobs->max_plimit_step_size_down);
    FpFwCliPrint("Min plimit upds/loop : %u\n", knobs->minimum_plimit_updates);
    FpFwCliPrint("Force pstate status : %s\n", knobs->force_pstate == POWER_KNOB_FORCE_PSTATE_DISABLE ? "disabled" : "enabled");
    FpFwCliPrint("\n");
}

static PLACED_CODE void print_power_config_survivability_mode(power_knobs_t* knobs)
{
    FpFwCliPrint("\nSurvivability Mode\n");
    FpFwCliPrint("------------------\n");
    FpFwCliPrint("Enabled : %s\n", knobs->enable_survivability_mode ? s_true_str : s_false_str);
    FpFwCliPrint("P-State : %d", knobs->survivability_mode_pstate);
    FpFwCliPrint(" / ");
    FpFwCliPrint("%d MHz", dvfs_get_freq_from_plimit(knobs->survivability_mode_pstate));
    FpFwCliPrint("\n");
}

static PLACED_CODE void print_power_config_static_rails(power_knobs_t* knobs)
{
    FpFwCliPrint("\nStatic rails\n");
    FpFwCliPrint("------------\n");
    char tempstr[20];
    snprintf(tempstr, sizeof(tempstr), "%f", knobs->static_rail_power_watts);
    FpFwCliPrint("Static rail pwr: %sW\n", tempstr);
    FpFwCliPrint("\n");
}

static PLACED_CODE void print_power_config_tel(power_knobs_t* knobs)
{
    FpFwCliPrint("\nTelemetry config\n");
    FpFwCliPrint("----------------\n");
    FpFwCliPrint("C1 telem. enabled : %s\n", knobs->c1_tel_enable ? s_true_str : s_false_str);
    FpFwCliPrint("\n");
}

static PLACED_CODE void print_power_config_throttling(power_knobs_t* knobs)
{
    FpFwCliPrint("\nThrottle config.\n");
    FpFwCliPrint("----------------\n");
    const power_currthrot_threshold_cfg_t* currthrot_cfg = &knobs->current_threshold;
    FpFwCliPrint("\nCurrent Threshold % Per Pstate Generation\n");
    FpFwCliPrint("-----------------------------------\n");
    FpFwCliPrint("Current Throttling Enabled : %s\n", currthrot_cfg->power_current_throt_en ? s_true_str : s_false_str);
    FpFwCliPrint("Iref to Max Percent        : %u\n", currthrot_cfg->iref_to_max_percent);
    FpFwCliPrint("T1 Percent                 : %u\n", currthrot_cfg->t1_percent);
    FpFwCliPrint("T2 Percent                 : %u\n", currthrot_cfg->t2_percent);
    FpFwCliPrint("T3 Percent                 : %u\n", currthrot_cfg->t3_percent);
    FpFwCliPrint("\nCurrent throttling (odcm & dvfs)\n");
    FpFwCliPrint(" Rolling windw cnt: %u\n", knobs->current_throt.rolling_window_count);
    FpFwCliPrint(" Telem epoch count: %u\n", knobs->current_throt.telemetry_epoch_count);
    FpFwCliPrint(" Inc Counter  : %u\n", knobs->current_throt.inc_ctr);
    FpFwCliPrint(" Dec Counter  : %u\n", knobs->current_throt.dec_ctr);
    FpFwCliPrint(" Inc Amount   : %u\n", knobs->current_throt.inc_amt);
    FpFwCliPrint(" Dec Amount0  : %u\n", knobs->current_throt.dec_amt0);
    FpFwCliPrint(" Dec Amount1  : %u\n", knobs->current_throt.dec_amt1);
    FpFwCliPrint("\nTile temp throttling\n");
    FpFwCliPrint(" Inc Counter  : %u\n", knobs->tile_temp_throt.inc_ctr);
    FpFwCliPrint(" Dec Counter  : %u\n", knobs->tile_temp_throt.dec_ctr);
    FpFwCliPrint(" Inc Amount   : %u\n", knobs->tile_temp_throt.inc_amt);
    FpFwCliPrint(" Dec Amount   : %u\n", knobs->tile_temp_throt.dec_amt);
    FpFwCliPrint("\nAdaptive clocking throttling\n");
    FpFwCliPrint(" Enabled      : %s\n", knobs->adclk_throt.enable ? s_true_str : s_false_str);
    FpFwCliPrint(" Inc Counter  : %u\n", knobs->adclk_throt.inc_ctr);
    FpFwCliPrint(" Dec Counter  : %u\n", knobs->adclk_throt.dec_ctr);
    FpFwCliPrint(" Inc Amount   : %u\n", knobs->adclk_throt.inc_amt);
    FpFwCliPrint(" Dec Amount   : %u\n", knobs->adclk_throt.dec_amt);
    FpFwCliPrint(" Response Option : %u\n", knobs->adclk_throt.resp_option);
    FpFwCliPrint(" Recovery Step Wait: %u\n", knobs->adclk_throt.recovery_step_wait);
    FpFwCliPrint(" Recover Step : %u\n", knobs->adclk_throt.recovery_step);
    FpFwCliPrint(" Response Wait: %u\n", knobs->adclk_throt.resp_wait);
    FpFwCliPrint(" Response Code: %u\n", knobs->adclk_throt.resp_code);
    FpFwCliPrint(" Comparator Config: %u\n", knobs->adclk_throt.adclk_comp_config);
    FpFwCliPrint(" Throttle Threshold: %u\n", knobs->adclk_throt.throttle_threshold);
    FpFwCliPrint(" Thrttl Windw Count: %u\n", knobs->adclk_throt.throttle_window_count);
    FpFwCliPrint(" Input Sense Enable: %s\n", knobs->adclk_throt.input_sense_enable ? "LDO input" : "LDO output");
    FpFwCliPrint("\n");
}

static PLACED_CODE void print_power_config_fgpll(power_knobs_t* knobs)
{
    FpFwCliPrint("\nFGPLL Config\n");
    FpFwCliPrint("------------\n");
    FpFwCliPrint("Cal Enabled : %s\n", knobs->calsm_enable ? s_true_str : s_false_str);
    FpFwCliPrint("ErrDetect : %s\n", knobs->plllock_cfg.enable_error ? s_true_str : s_false_str);
    FpFwCliPrint("LckCntSel : %d\n", knobs->plllock_cfg.lckcntsel);
    FpFwCliPrint("Mask PLL Lock Raw      : %s\n", knobs->plllock_cfg.mask_plllockraw ? s_true_str : s_false_str);
    FpFwCliPrint("Mask Freq Change Timeout: %s\n", knobs->plllock_cfg.mask_freqchangetimeout ? s_true_str : s_false_str);
    FpFwCliPrint("Mask FLL Cal Timeout   : %s\n", knobs->plllock_cfg.mask_fllcaltimeout ? s_true_str : s_false_str);
    FpFwCliPrint("Mask Lock Timeout      : %s\n", knobs->plllock_cfg.mask_locktimeout ? s_true_str : s_false_str);
    FpFwCliPrint("Mask Freq Error        : %s\n", knobs->plllock_cfg.mask_freqerror ? s_true_str : s_false_str);
    FpFwCliPrint("\n");
}

static PLACED_CODE void print_power_config_itd_cfg(power_knobs_t* knobs)
{
    FpFwCliPrint("\nITD Config\n");
    FpFwCliPrint("--------------\n");
    FpFwCliPrint("ITD Enabled : %d\n", knobs->itd_cfg);
}

static PLACED_CODE void print_power_avs_ds_cfg(power_knobs_t* knobs)
{
    avs_ds_cfg_array_t* avs_ds_cfg = &knobs->avs_ds;
    FpFwCliPrint("\nAVS Drive Strength Config\n");
    FpFwCliPrint("--------------------------\n");
    FpFwCliPrint("AVS Drive Strength Config Array:\n");
    for (unsigned i = 0; i < ARRAY_SIZE(avs_ds_cfg->array); ++i) {
        FpFwCliPrint("  AVS%d: clk=%d, mdata=%d\n", i, avs_ds_cfg->array[i].clk, avs_ds_cfg->array[i].mdata);
    }
}

static PLACED_CODE void print_power_leakage_temp_scaler(power_knobs_t* knobs)
{
    const power_leakage_temp_scaler_t* scaler = &knobs->leakage_temp_scaler;
    static const char* corner_names[] = { "SS", "TT", "FF", "Unknown" };
    FpFwCliPrint("\nLeakage Temperature Scaling Polynomials\n");
    FpFwCliPrint("--------------------------------------\n");
    for (unsigned i = 0; i < 4; ++i) {
        FpFwCliPrint("Corner %s:\n", corner_names[i]);
        FpFwCliPrint("  y = %f * x^3 + %f * x^2 + %f * x + %f\n",
            scaler->poly_coefficients[i].a,
            scaler->poly_coefficients[i].b,
            scaler->poly_coefficients[i].c,
            scaler->poly_coefficients[i].d);
    }
    FpFwCliPrint("\n");
}

static PLACED_CODE void print_power_force_vrs(power_knobs_t* knobs)
{
    const power_force_vrs_t* force_vrs = &knobs->forced_vrs;
    static const char* vr_names[10] = {
        "VCPU0",      // 0 Die0
        "VPCIE0P75", // 1 Die0
        "VSYS",      // 2 Die0
        "VDDQ1P1",   // 3 Die0
        "VSOC",      // 4 Die0
        "VD2D0P55",  // 5 Die0
        "VPLL1P2",   // 6 Die0
        "VGPIO1P2",  // 7 Die0
        "VCPU1",     // 8 Die1
        "VD2D0P875"  // 9 Die1
    };

    FpFwCliPrint("\nForced VR Values\n");
    FpFwCliPrint("----------------\n");
    for (int i = 0; i < 10; ++i) {
        const char* die = (i < 8) ? "Die 0" : "Die 1";
        FpFwCliPrint("VR[%d] (%s, %s): %u mV\n", i, die, vr_names[i], force_vrs->vr[i]);
    }
    FpFwCliPrint("\n");
}

static PLACED_CODE void print_power_adclk_offset_cfg(power_knobs_t* knobs)
{
    const power_adclk_offset_cfg_t* adclk_offset_cfg = &knobs->adclk_offset;
    FpFwCliPrint("\nAdaptive Clocking Offset Config\n");
    FpFwCliPrint("--------------------------------\n");
    FpFwCliPrint("Adaptive Clocking Offset Enabled: %s\n", adclk_offset_cfg->enable ? s_true_str : s_false_str);
    FpFwCliPrint("Offset Values (per core):\n");
    for (unsigned i = 0; i < ARRAY_SIZE(adclk_offset_cfg->offset_value); ++i) {
        FpFwCliPrint("  Core %u: %u\n", i, adclk_offset_cfg->offset_value[i]);
    }
    FpFwCliPrint("\n");
}

static PLACED_CODE void print_power_config_vsys_override(power_knobs_t* knobs)
{
    const power_force_vrs_t* force_vrs = &knobs->forced_vrs;
    FpFwCliPrint("\nVSYS_vboot Override Config\n");
    FpFwCliPrint("------------------------------\n");
    FpFwCliPrint("VSYS Override Enabled : %s\n", knobs->enable_vsys_vboot_override ? s_true_str : s_false_str);
    FpFwCliPrint("VSYS Override Value   : %u mV\n", force_vrs->vr[MPCL_VR_VSYS]);
    FpFwCliPrint("\n");
}

static PLACED_CODE void print_power_config_cppc_lowest_nonlin_perf(power_knobs_t* knobs)
{
    FpFwCliPrint("\nCPPC Lowest Non-Linear Performance Config\n");
    FpFwCliPrint("-----------------------------------------\n");
    if (knobs->cppc_lowest_nonlin_perf == 32)
    {
        FpFwCliPrint("CPPC Lowest Non-Linear Perf : knob not set (32)\n");
    }
    else
    {
        FpFwCliPrint("CPPC Lowest Non-Linear Perf Pstate : %u\n", knobs->cppc_lowest_nonlin_perf);
    }
    FpFwCliPrint("\n");
}

static PLACED_CODE void print_power_config_knobs(power_knobs_t* knobs)
{
    FpFwCliPrint("\n---------------------------All Power Knobs---------------------------\n");
    print_power_config_interval(knobs);
    print_power_config_tel(knobs);
    print_power_config_limits(knobs);
    print_power_config_vcpucalc(knobs);
    print_power_config_pidcfg(knobs);
    print_power_config_loopcfg(knobs);
    print_power_config_throttling(knobs);
    print_power_config_thresholds(knobs);
    print_power_config_survivability_mode(knobs);
    print_power_config_fgpll(knobs);
    print_power_config_static_rails(knobs);
    print_power_config_itd_cfg(knobs);
    print_power_avs_ds_cfg(knobs);
    print_power_leakage_temp_scaler(knobs);
    print_power_force_vrs(knobs);
    print_power_adclk_offset_cfg(knobs);
    print_power_config_vsys_override(knobs);
    print_power_config_cppc_lowest_nonlin_perf(knobs);
}

static PLACED_CODE void print_power_config_max_allowed_plimit(power_knobs_t* knobs)
{
    FpFwCliPrint("Allowed plimit max: %u\n", (unsigned int)knobs->allowed_plimit_maximum);
}

static PLACED_CODE void print_power_config_min_allowed_plimit(power_knobs_t* knobs)
{
    FpFwCliPrint("Allowed plimit min: %u\n", (unsigned int)knobs->allowed_plimit_minimum);
}

PLACED_CODE power_if_cmd_t cli_power_config_get_cmd_id(char* sub_command)
{
    if (sub_command == NULL)
    {
        return POWER_IF_CMD_UNKNOWN;
    }

    /* Parse dictionary for subcommand and fetch corresponding power_ext_if_cmd_id */
    for (uint32_t index = 0; index < length_dictionary; index++)
    {
        if (strcmp(sub_command, power_cli_config_sub_command_dictionary[index].sub_command) == 0)
        {
            return power_cli_config_sub_command_dictionary[index].power_ext_if_cmd_id;
        }
    }

    /* If sub-command is not found in the dictionary */
    return POWER_IF_CMD_UNKNOWN;
}

PLACED_CODE void cli_power_config_async_print(PDFWK_ASYNC_REQUEST_HEADER p_request, void* completion_context)
{
    FPFW_UNUSED(completion_context);

    ppower_service_cli_request_t request = (ppower_service_cli_request_t)p_request;

    if (request->fetch_data.p_requested_data == NULL)
    {
        FpFwCliPrint("CLI Data Error\n");
        return;
    }

    for (uint32_t index = 0; index < length_dictionary; index++)
    {
        /* Compare the sub command string with the dictionary and call the appropriate print function */
        if (strcmp(request->sub_command, power_cli_config_sub_command_dictionary[index].sub_command) == 0)
        {
            power_cli_config_sub_command_dictionary[index].fn(request->fetch_data.p_requested_data);

            // Print completion message
            FpFwCliPrint("pwr_cli_comp\n");
            return;
        }
    }

    FpFwCliPrint("Invalid sub cmd.\n");
}

static PLACED_CODE void print_power_config_vf(power_fuse_data_t* fuses)
{
    //! @brief Print the VF curveset from fuses
    power_fuse_vf_curveset_t* vf_curves = &fuses->vf;
    FpFwCliPrint("\nRaw VF Curveset From Fuses\n");
    FpFwCliPrint("%-12s %-10s %-8s %-16s %-14s\n", "CurveSet", "Curve", "Pair", "Frequency (MHz)", "LDO DAC Code");
    FpFwCliPrint("=======================================================================\n");

    int cs = 0; // Only print for curve set 0
    for (int curve = 0; curve < VFT_CURVE_COUNT_PER_CURVESET; ++curve) {
        for (int pair = 0; pair < DVFS_FUSED_PAIRS_COUNT; ++pair) {
            const dvfs_vf_pair_t* p = &vf_curves->curveset[cs].curve[curve].pair[pair];
            FpFwCliPrint("%-12d %-10d %-8d %-16u %-14u\n", cs, curve, pair, p->freq_Mhz, p->ldo_dac_in);
        }
        FpFwCliPrint("\n");
    }
    FpFwCliPrint("\n");
}

static PLACED_CODE void print_power_config_vft(power_runconfig_t* p_runconfig)
{
    FpFwCliPrint("\nMax Voltage for Every Curve\n");
    FpFwCliPrint("%-12s %-10s %-16s %-14s\n", "CurveSet", "PState", "Frequency (MHz)", "LDO_DAC To Voltage(mV)");
    FpFwCliPrint("=======================================================================\n");

    for (unsigned vf_idx = 0; vf_idx < VFT_CURVESET_COUNT; ++vf_idx){
        for (unsigned pstate_idx = p_runconfig->derived.vfts[vf_idx].min_plimit; pstate_idx < NUM_PSTATES; ++pstate_idx){
                 FpFwCliPrint("%-12d %-10d %-16u %-14u\n",vf_idx,  pstate_idx,p_runconfig->derived.vfts[vf_idx].vf[pstate_idx].freq_Mhz, p_runconfig->derived.vfts[vf_idx].vf[pstate_idx].voltage_mv);
        }
    }
}

static PLACED_CODE void print_power_config_vftpre(power_vft_curveset_precalc_t* precalculated_current)
{
    char tempstr[20];
    FpFwCliPrint("\n");
    for (unsigned vf_idx = 0; vf_idx <= VFT_CURVESET_COUNT; ++vf_idx) {
        for (unsigned pstate_idx = 0; pstate_idx < NUM_PSTATES + INFO_ROWS; ++pstate_idx) {
            switch (pstate_idx) {
                case 0:  // print curve number
                    FpFwCliPrint("Curve   %d     ", vf_idx);
                    FpFwCliPrint("\n");
                    break;
                case 1:  // print header
                    FpFwCliPrint("  lkg (A)    reflkg (A) dynamic (A) dyn_ldo (A)  cdyn (pF)    PState ");
                    FpFwCliPrint("\n");
                    break;
                case 2:  // print header
                    FpFwCliPrint("=========== =========== =========== =========== =========== ===========");
                    FpFwCliPrint("\n");
                    break;
                default:
                    print_padding((unsigned)precalculated_current->curveset[vf_idx].vf[pstate_idx - INFO_ROWS].leakage, 3);
                    snprintf(tempstr, sizeof(tempstr), "%f", precalculated_current->curveset[vf_idx].vf[pstate_idx - INFO_ROWS].leakage);
                    FpFwCliPrint("%s ", tempstr);
                    print_padding((unsigned)precalculated_current->curveset[vf_idx].vf[pstate_idx-INFO_ROWS].ref_leakage, 3);
                    snprintf(tempstr, sizeof(tempstr), "%f", precalculated_current->curveset[vf_idx].vf[pstate_idx-INFO_ROWS].ref_leakage);
                    FpFwCliPrint("%s ", tempstr);
                    print_padding((unsigned)precalculated_current->curveset[vf_idx].vf[pstate_idx-INFO_ROWS].dynamic, 3);
                    snprintf(tempstr, sizeof(tempstr), "%f", precalculated_current->curveset[vf_idx].vf[pstate_idx-INFO_ROWS].dynamic);
                    FpFwCliPrint("%s ", tempstr);
                    print_padding((unsigned)precalculated_current->curveset[vf_idx].vf[pstate_idx-INFO_ROWS].dynamic_ldo, 3);
                    snprintf(tempstr, sizeof(tempstr), "%f", precalculated_current->curveset[vf_idx].vf[pstate_idx-INFO_ROWS].dynamic_ldo);
                    FpFwCliPrint("%s ", tempstr);
                    print_padding((unsigned)precalculated_current->curveset[vf_idx].vf[pstate_idx-INFO_ROWS].cdyn_pf, 3);
                    snprintf(tempstr, sizeof(tempstr), "%f", precalculated_current->curveset[vf_idx].vf[pstate_idx-INFO_ROWS].cdyn_pf);
                    FpFwCliPrint("%s ", tempstr);
                    print_padding((unsigned)(pstate_idx-INFO_ROWS), 3);
                    FpFwCliPrint("%d",(pstate_idx-INFO_ROWS));
                    FpFwCliPrint("\n");
                    break;
            }
        }
        FpFwCliPrint("\n");
    }
}

static PLACED_CODE void print_power_config_vcpucalc(power_knobs_t* knobs)
{
    FpFwCliPrint("\nConfigured Vcpu Calc Inputs\n");
    FpFwCliPrint("---------------------------\n");
    FpFwCliPrint("R_loadline VCPU0 uohm : %uuOhm\n", knobs->r_loadline_vcpu0_uohm);
    FpFwCliPrint("R_loadline VCPU1 uohm : %uuOhm\n", knobs->r_loadline_vcpu1_uohm);
    FpFwCliPrint( "ActvtyFact : %u\n", knobs->activity_factor_dhry_adjustment);
    FpFwCliPrint("vcpu Offset mv    : %imV\n", knobs->vcpu_offset_mv);
    FpFwCliPrint("Vldo offset: %i(LDO DAC offset)\n", knobs->ldo_offset);
    FpFwCliPrint("forced Pstate(32 to disable, 0-31 to force pstate): %i\n", knobs->force_pstate);
    FpFwCliPrint("\n");
}
