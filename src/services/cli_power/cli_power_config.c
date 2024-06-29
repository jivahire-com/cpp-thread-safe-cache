//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_power_config.c
 * Source file to implement power config CLI commands.
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h>
#include <cli_power_common.h>
#include <cli_power_config.h>
#include <cli_power_interface.h>
#include <corebits.h>
#include <power_dfwk.h>
#include <power_runconfig.h>
#include <silibs_common.h>
#include <string.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define DFVS_FUSED_COREMEMASST_COUNT 5

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
static void print_power_config_mpmm(power_knobs_t* knobs);
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

/*-- Declarations (Statics and globals) --*/

// clang-format off
/* Structure storing the dictionary of sub-command string against its corresponding print function */
power_cli_sub_command_dictionary_element_t power_cli_config_sub_command_dictionary[] = {
    {"fuse",                (print_function)print_power_config_fuse,                POWER_RUNCONFIG_FUSES},
    {"dts",                 (print_function)print_power_config_dts,                 POWER_RUNCONFIG_FUSES},
    {"memasst",             (print_function)print_power_config_memasst,             POWER_RUNCONFIG_FUSES},
    {"intervals",           (print_function)print_power_config_interval,            POWER_RUNCONFIG_KNOBS},
    {"limits",              (print_function)print_power_config_limits,              POWER_RUNCONFIG_KNOBS},
    {"mpmm",                (print_function)print_power_config_mpmm,                POWER_RUNCONFIG_KNOBS},
    {"pid",                 (print_function)print_power_config_pidcfg,              POWER_RUNCONFIG_KNOBS},
    {"pvt",                 (print_function)print_power_config_thresholds,          POWER_RUNCONFIG_KNOBS},
    {"ctrlloop",            (print_function)print_power_config_loopcfg,             POWER_RUNCONFIG_KNOBS},
    {"srvmode",             (print_function)print_power_config_survivability_mode,  POWER_RUNCONFIG_KNOBS},
    {"tel",                 (print_function)print_power_config_tel,                 POWER_RUNCONFIG_KNOBS},
    {"throttle",            (print_function)print_power_config_throttling,          POWER_RUNCONFIG_KNOBS},
    {"static",              (print_function)print_power_config_static_rails,        POWER_RUNCONFIG_KNOBS},
    {"allowed_min_plimit",  (print_function)print_power_config_min_allowed_plimit,  POWER_RUNCONFIG_KNOBS},
    {"allowed_max_plimit",  (print_function)print_power_config_max_allowed_plimit,  POWER_RUNCONFIG_KNOBS},
    {"fgpll",               (print_function)print_power_config_fgpll,               POWER_RUNCONFIG_KNOBS},
    {"knobs",               (print_function)print_power_config_knobs,               POWER_RUNCONFIG_KNOBS},
    /* TODO: The below functions need to be added after the underlying print functions are ported. 
    These CLI functions are not implemented in this PR. */
    /*
    {"coremin",             (print_function)print_power_config_min_plimit,          POWER_RUNCONFIG_FUSES},
    {"lkgcalc",             (print_function)print_power_config_lkg,                 POWER_RUNCONFIG_FUSES},
    {"tel",                 (print_function)print_power_config_tel,                 POWER_RUNCONFIG_FUSES},
    {"vft",                 (print_function)print_power_config_vft,                 POWER_RUNCONFIG_FUSES},
    {"vftpre",              (print_function)print_power_config_vftpre,              POWER_RUNCONFIG_FUSES},
    {"vcpucalc",            (print_function)print_power_config_vcpucalc,            POWER_RUNCONFIG_KNOBS},
    */
};
//clang-format on

const uint32_t length_dictionary =
    sizeof(power_cli_config_sub_command_dictionary) / sizeof(power_cli_sub_command_dictionary_element_t);

/*-------------- Functions ---------------*/
/* -------------------------------------- */
static void print_padding(uint32_t decimal, unsigned places)
{
    uint32_t value = 1;
    for (unsigned i = 0; i < places; ++i)
    {
        value *= 10;
        if (decimal < value)
        {
            printf(" ");
        }
    }
}
/* -------------------------------------- */

/* -------------------------------------- */
static void print_power_config_fuse(power_fuse_data_t* fuses)
{
    printf("\nMisc Fuse configurations\n");
    printf("-----------------------\n");
    printf("TDP cores  : %d\n", fuses->tdp_config.num_cores);
    printf("TDP power  : %d A\n", fuses->tdp_config.power_A);
    printf("TDP freq   : %d MHz\n", fuses->tdp_config.freq_MHz);
    printf("Valid cores: " COREBITS_FMT_STR "\n", COREBITS_FMT_DATA(fuses->valid_cores));

    printf("\n");
}
/* -------------------------------------- */

/* -------------------------------------- */
static void print_dts_coeffs(dts_coeff_t* coeff, unsigned count)
{
    for (unsigned int idx = 0; idx < count; ++idx)
    {
        printf(" %02d y: %d k: %d\n", idx, coeff[idx].y_val, coeff[idx].k_val);
    }
}

static void print_power_config_dts(power_fuse_data_t* fuses)
{
    printf("\nDTS Coefficients\n");
    printf("----------------\n");
    printf("\nTile\n");
    print_dts_coeffs(fuses->dts_coeff_tile, ARRAY_SIZE(fuses->dts_coeff_tile));
    printf("\nSoCTOP\n");
    print_dts_coeffs(fuses->dts_coeff_soctop, ARRAY_SIZE(fuses->dts_coeff_soctop));
    printf("\n");
}
/* -------------------------------------- */

/* -------------------------------------- */
static void print_power_config_memasst(power_fuse_data_t* fuses)
{
    printf("\n");
    printf("  ldo   hdrawlm hdema hshcrawlm hshscema tpemaa tpemab hd_emaw\n");
    for (unsigned r_idx = 0; r_idx < DFVS_FUSED_COREMEMASST_COUNT; ++r_idx)
    {
        printf(" ");
        if (!fuses->memasst.entry[r_idx].valid_boundary)
        {
            printf("---\n");
            continue;
        }
        unsigned ldo = fuses->memasst.entry[r_idx].ldo_dac_in;
        print_padding(ldo, 2);
        printf("%d    %d    %d    %d    %d    %d    %d    %d\n",
               ldo,
               fuses->memasst.entry[r_idx].hd_rawlm,
               fuses->memasst.entry[r_idx].hd_ema,
               fuses->memasst.entry[r_idx].hshc_rawlm,
               fuses->memasst.entry[r_idx].hshc_ema,
               fuses->memasst.entry[r_idx].tp_emaa,
               fuses->memasst.entry[r_idx].tp_emab,
               fuses->memasst.entry[r_idx].hd_emaw);
    }
    printf("\n");
}
/* -------------------------------------- */

/* -------------------------------------- */
static void print_power_config_interval(power_knobs_t* knobs)
{
    printf("\nConfigured Loop Intervals\n");
    printf("-------------------------\n");
    printf("Control loop    Temp telemetry divider    PVT loop\n");
    print_padding(knobs->control_loop_interval, 9);
    printf("%ums    ", knobs->control_loop_interval);
    print_padding(knobs->temp_telemetry_divider, 9);
    printf("%u ", knobs->temp_telemetry_divider);
    print_padding(knobs->temp_telemetry_divider * knobs->control_loop_interval, 6);
    printf("(%ums)    ", knobs->temp_telemetry_divider * knobs->control_loop_interval);
    print_padding(knobs->pvt_loop_interval, 5);
    printf("%ums\n", knobs->pvt_loop_interval);
    printf("\n");
}
/* -------------------------------------- */

/* -------------------------------------- */
static void print_power_config_limits(power_knobs_t* knobs)
{
    printf("\nConfigured Control Loop Limits\n");
    printf("---------------------------------------------\n");
    printf("Thermal/watt    Vcpu0 current   Vcpu1 current\n");
    print_padding(knobs->soc_maximum_thermal_watts_limit, 10);
    printf("%uW     ", knobs->soc_maximum_thermal_watts_limit);
    print_padding(knobs->soc_maximum_electrical_current_limit_vcpu0, 10);
    printf("%uA    ", knobs->soc_maximum_electrical_current_limit_vcpu0);
    print_padding(knobs->soc_maximum_electrical_current_limit_vcpu1, 10);
    printf("%uA", knobs->soc_maximum_electrical_current_limit_vcpu1);
    printf("\n");
}
/* -------------------------------------- */

/* -------------------------------------- */
static void print_power_config_mpmm(power_knobs_t* knobs)
{
    printf("\nMPMM configuration\n");
    printf("------------------\n");
    printf("Enabled : %s\n", knobs->mpmm.enable ? s_true_str : s_false_str);
    printf("Gear    : %u\n", knobs->mpmm.gear);
    printf("\n");
}
/* -------------------------------------- */

/* -------------------------------------- */
static void print_power_config_pidcfg(power_knobs_t* knobs)
{
    printf("\nCtrl loop PID configuration\n");
    printf("-----------------------\n");
    printf("Kp             : (%u.%03u)\n", (unsigned int)knobs->pid.kpt / 1000, (unsigned int)knobs->pid.kpt % 1000);
    printf("Ki             : (%u.%03u)\n", (unsigned int)knobs->pid.kit / 1000, (unsigned int)knobs->pid.kit % 1000);
    printf("Kd             : (%u.%03u)\n", (unsigned int)knobs->pid.kdt / 1000, (unsigned int)knobs->pid.kdt % 1000);
    printf("Setpoint offset: %umW\n", (unsigned int)knobs->pid.setpoint_offset);
    /* TODO: These values don't exist in the struct. Check if these values will exist in Kingsgate and enable them when they are supported ADO: 1491013 */
    // printf("Resource max   : %u\n", knobs->pid.resource_max);
    printf("\n");
}
/* -------------------------------------- */

/* -------------------------------------- */

static void print_voltage(uint16_t voltage)
{
    print_padding(voltage, 4);
    printf("%dmV", voltage);
}

static void print_temp(uint16_t temp)
{
    uint16_t tenths = temp % 10;
    temp = temp / 10;
    print_padding(temp, 3);
    printf("%d.%dC", temp, tenths);
}

static void print_power_config_thresholds(power_knobs_t* knobs)
{
    printf("\nPVT Thresholds\n");
    printf("--------------\n");
    printf("Tile\n");
    printf("  Hot (hyst/alarm)      : ");
    print_temp(knobs->tile_temp_throt.hot.hyst_threshold);
    printf(" / ");
    print_temp(knobs->tile_temp_throt.hot.alarm_threshold);
    printf("\n");
    printf("  Thermtrip (hyst/alarm): ");
    print_temp(knobs->tile_temp_throt.thermtrip.hyst_threshold);
    printf(" / ");
    print_temp(knobs->tile_temp_throt.thermtrip.alarm_threshold);
    printf("\n");

    for (int vm_idx = 0; vm_idx < TILE_PVT_NUM_CHANNELS_VM; ++vm_idx)
    {
        printf("  VM%d OV    (hyst/alarm): ", vm_idx);
        print_voltage(knobs->tile_vm.thresholds[vm_idx].overvolt.hyst_threshold);
        printf(" / ");
        print_voltage(knobs->tile_vm.thresholds[vm_idx].overvolt.alarm_threshold);
        printf("\n");
        printf("  VM%d UV    (hyst/alarm): ", vm_idx);
        print_voltage(knobs->tile_vm.thresholds[vm_idx].undervolt.hyst_threshold);
        printf(" / ");
        print_voltage(knobs->tile_vm.thresholds[vm_idx].undervolt.alarm_threshold);
        printf("\n");
    }

    printf("SOC\n");
    printf("  Hot (hyst/alarm)      : ");
    print_temp(knobs->soc_temp.hot.hyst_threshold);
    printf(" / ");
    print_temp(knobs->soc_temp.hot.alarm_threshold);
    printf("\n");
    printf("  Thermtrip (hyst/alarm): ");
    print_temp(knobs->soc_temp.thermtrip.hyst_threshold);
    printf(" / ");
    print_temp(knobs->soc_temp.thermtrip.alarm_threshold);
    printf("\n");

    for (int vm_idx = 0; vm_idx < SOC_PVT_TOTAL_CHANNELS_VM; ++vm_idx)
    {
        printf("  VM%d OV    (hyst/alarm): ", vm_idx);
        print_voltage(knobs->soc_vm.thresholds[vm_idx].overvolt.hyst_threshold);
        printf(" / ");
        print_voltage(knobs->soc_vm.thresholds[vm_idx].overvolt.alarm_threshold);
        printf("\n");
        printf("  VM%d UV    (hyst/alarm): ", vm_idx);
        print_voltage(knobs->soc_vm.thresholds[vm_idx].undervolt.hyst_threshold);
        printf(" / ");
        print_voltage(knobs->soc_vm.thresholds[vm_idx].undervolt.alarm_threshold);
        printf("\n");
    }
    printf("\n");
}
/* -------------------------------------- */

/* -------------------------------------- */
static void print_power_config_loopcfg(power_knobs_t* knobs)
{
    printf("\nCtrl loop configuration\n");
    printf("-----------------------\n");
    printf("Power capping mode  : %s\n", knobs->capping_mode == power_capping_mode_t_ALL ? "all" : "per VM");
    printf("C2 limits to nominal: %s\n", knobs->c2_cores_limit_to_nominal ? s_true_str : s_false_str);
    printf("C3 limits to nominal: %s\n", knobs->c3_cores_limit_to_nominal ? s_true_str : s_false_str);
    printf("Allow plimit<nominal: %s\n", knobs->allow_plimit_below_nominal ? s_true_str : s_false_str);
    /* TODO: These values don't exist in the struct. Check if these values will exist in Kingsgate and enable them when they are supported ADO: 1491013 */
    // printf("Minimal turbo pri   : %s\n", knobs->allow_minimal_turbo_prioritization ? s_true_str : s_false_str);
    printf("Intervals to lower  : %u\n", knobs->intervals_to_lower_plimit);
    printf("Allwd plimit minimum: %u\n", knobs->allowed_plimit_minimum);
    printf("Allwd plimit maximum: %u\n", knobs->allowed_plimit_maximum);
    printf("Step size up maximum: %u\n", knobs->max_plimit_step_size_up);
    printf("Step size dn maximum: %u\n", knobs->max_plimit_step_size_down);
    printf("Min plimit upds/loop: ");
    
    /* TODO: These values don't exist in the struct. Check if these values will exist in Kingsgate and enable them when they are supported ADO: 1491013 */
    // print_minupdate(knobs->minimum_plimit_updates);
    // printf("\n");
    // printf("Nominal pstate      : ");
    // if (knobs->nominal_pstate == 0) {
    //     printf("fused default (P%d)\n", power_context.runconfig.pnominal);
    // } else {
    //     printf("P%d\n", knobs->nominal_pstate);
    // }
    // printf("\nForced pstates\n");
    // for (int count = 0; count < ST_COUNT; ++count)
    // {
    //     printf("Core:Pxx ");
    // }
    // printf("\n");
    // for (int count = 0; count < ST_COUNT; ++count)
    // {
    //     printf("=========");
    // }
    // printf("\n");
    // for (unsigned core_idx = 0; core_idx < power_context.core_count; ++core_idx) {
    //     if (knobs->force_pstate[core_idx] == POWER_KNOB_FORCE_PSTATE_DISABLE) {
    //         printf("%04d:--- ", core_idx);
    //     } else {
    //         printf("%04d:P%02d ", core_idx, knobs->force_pstate[core_idx]);
    //     }
    //
    //     if (core_idx % ST_COUNT == ST_COUNT - 1) {
    //         printf("\n");
    //     }
    // }
    //
    printf("\n");
}
/* -------------------------------------- */

/* -------------------------------------- */
static void print_power_config_survivability_mode(power_knobs_t* knobs)
{
    printf("\nSurvivability Mode\n");
    printf("--------------\n");
    printf("Enabled : %s\n", knobs->enable_survivability_mode ? s_true_str : s_false_str);
    printf("P-State : %d", knobs->survivability_mode_pstate);
    printf(" / ");
    /* TODO: These values don't exist in the struct. Check if these values will exist in Kingsgate and enable them when they are supported ADO: 1491013 */
    // printf("%d MHz", dvfs_get_freq_from_plimit(knobs->survivability_mode_pstate));
    printf("\n");
}
/* -------------------------------------- */

/* -------------------------------------- */
static void print_power_config_static_rails(power_knobs_t* knobs)
{
    printf("\nStatic rails\n");
    printf("--------------\n");
    char tempstr[20];
    snprintf(tempstr, sizeof(tempstr), "%f", knobs->static_rail_power_watts);
    printf("Static rail power: %sW\n", tempstr);
    printf("\n");
}
/* -------------------------------------- */

/* -------------------------------------- */
static void print_power_config_tel(power_knobs_t* knobs)
{
    printf("\nTelemetry configuration\n");
    printf("-----------------------\n");
    printf("C1 telemetry enabled : %s\n", knobs->c1_tel_enable ? s_true_str : s_false_str);
    printf("\n");
}
/* -------------------------------------- */

/* -------------------------------------- */
static void print_power_config_throttling(power_knobs_t* knobs)
{
    printf("\nThrottle configuration\n");
    printf("----------------------\n");
    printf("Current throttling\n");
    /* TODO: These values don't exist in the struct. Check if these values will exist in Kingsgate and enable them when they are supported ADO: 1491013 */
    // printf("  Lower threshold   : %u\n", knobs->current_throt.lower_threshold_percent);
    // printf("  Upper threshold   : %u\n", knobs->current_throt.upper_threshold_percent);
    // printf("  Peak threshold    : %u\n", knobs->current_throt.peak_threshold_percent);
    printf("  Rolling window cnt: %u\n", knobs->current_throt.rolling_window_count);
    printf("  Telem epoch count : %u\n", knobs->current_throt.telemetry_epoch_count);
    printf("  Inc Counter       : %u\n", knobs->current_throt.inc_ctr);
    printf("  Dec Counter       : %u\n", knobs->current_throt.dec_ctr);
    printf("  Inc Amount        : %u\n", knobs->current_throt.inc_amt);
    printf("  Dec Amount0       : %u\n", knobs->current_throt.dec_amt0);
    printf("  Dec Amount1       : %u\n", knobs->current_throt.dec_amt1);
    printf("Tile temperature throttling\n");
    printf("  Inc Counter       : %u\n", knobs->tile_temp_throt.inc_ctr);
    printf("  Dec Counter       : %u\n", knobs->tile_temp_throt.dec_ctr);
    printf("  Inc Amount        : %u\n", knobs->tile_temp_throt.inc_amt);
    printf("  Dec Amount        : %u\n", knobs->tile_temp_throt.dec_amt);
    printf("Adaptive clocking throttling\n");
    printf("  Enabled           : %s\n", knobs->adclk_throt.enable ? s_true_str : s_false_str);
    printf("  Inc Counter       : %u\n", knobs->adclk_throt.inc_ctr);
    printf("  Dec Counter       : %u\n", knobs->adclk_throt.dec_ctr);
    printf("  Inc Amount        : %u\n", knobs->adclk_throt.inc_amt);
    printf("  Dec Amount        : %u\n", knobs->adclk_throt.dec_amt);
    printf("  Response Option   : %u\n", knobs->adclk_throt.resp_option);
    printf("  Recovery Step Wait: %u\n", knobs->adclk_throt.recovery_step_wait);
    printf("  Recover Step      : %u\n", knobs->adclk_throt.recovery_step);
    printf("  Response Wait     : %u\n", knobs->adclk_throt.resp_wait);
    printf("  Response Code     : %u\n", knobs->adclk_throt.resp_code);
    printf("  Comparator Config : %u\n", knobs->adclk_throt.adclk_comp_config);
    printf("  Throttle Threshold: %u\n", knobs->adclk_throt.throttle_threshold);
    printf("  Thrttl Windw Count: %u\n", knobs->adclk_throt.throttle_window_count);
    printf("\n");
}
/* -------------------------------------- */

/* -------------------------------------- */
static void print_power_config_fgpll(power_knobs_t* knobs)
{
    printf("\nFGPLL Config\n");
    printf("--------------\n");
    printf("Cal Enabled : %s\n", knobs->calsm_enable ? s_true_str : s_false_str);
    /* TODO: These values don't exist in the struct. Check if these values will exist in Kingsgate and enable them when they are supported ADO: 1491013 */
    // printf("P-State Cap : %d", knobs->ftable_pstate_cap);
    // printf(" / ");
    // printf("%d MHz", dvfs_get_freq_from_plimit(knobs->ftable_pstate_cap));
    printf("\n");
    printf("ErrDetect   : %s\n", knobs->plllock_cfg.enable_error ? s_true_str : s_false_str);
    printf("LckCntSel   : %d\n", knobs->plllock_cfg.lckcntsel);
}
/* -------------------------------------- */

/* -------------------------------------- */
static void print_power_config_knobs(power_knobs_t* knobs)
{
    print_power_config_interval(knobs);
    print_power_config_tel(knobs);
    print_power_config_limits(knobs);
    // print_power_config_vcpucalc(knobs);
    print_power_config_mpmm(knobs);
    print_power_config_pidcfg(knobs);
    print_power_config_loopcfg(knobs);
    print_power_config_throttling(knobs);
    print_power_config_thresholds(knobs);
    print_power_config_survivability_mode(knobs);
    print_power_config_fgpll(knobs);
    print_power_config_static_rails(knobs);
}
/* -------------------------------------- */

/* -------------------------------------- */
static void print_power_config_max_allowed_plimit(power_knobs_t* knobs)
{
    printf("Allowed plimit maximum: %u\n", (unsigned int)knobs->allowed_plimit_maximum);
}
/* -------------------------------------- */

/* -------------------------------------- */
static void print_power_config_min_allowed_plimit(power_knobs_t* knobs)
{
    printf("Allowed plimit minimum: %u\n", (unsigned int)knobs->allowed_plimit_minimum);
}
/* -------------------------------------- */

/* -------------------------------------- */
power_runconfig_element_t cli_power_config_get_runconfig_element_id(char* sub_command)
{
    if (sub_command == NULL)
    {
        return POWER_RUNCONFIG_UNKNOWN;
    }

    /* Parse dictionary for subcommand and fetch corresponding power_runconfig_element_id */
    for (uint32_t index = 0; index < length_dictionary; index++)
    {
        if (strcmp(sub_command, power_cli_config_sub_command_dictionary[index].sub_command) == 0)
        {
            return power_cli_config_sub_command_dictionary[index].power_runconfig_element_id;
        }
    }

    /* If sub-command is not found in the dictionary */
    return POWER_RUNCONFIG_UNKNOWN;
}

void cli_power_config_async_print(PDFWK_ASYNC_REQUEST_HEADER p_request, void* completion_context)
{
    FPFW_UNUSED(completion_context);

    ppower_service_cli_request_t request = (ppower_service_cli_request_t)p_request;

    if (request->p_requested_data == NULL)
    {
        printf("CLI Data Error\n");
        return;
    }

    for (uint32_t index = 0; index < length_dictionary; index++)
    {
        /* Compare the sub command string with the dictionary and call the appropriate print function */
        if (strcmp(request->sub_command, power_cli_config_sub_command_dictionary[index].sub_command) == 0)
        {
            power_cli_config_sub_command_dictionary[index].fn(request->p_requested_data);
            return;
        }
    }

    printf("Invalid sub command\n");
}
/* -------------------------------------- */