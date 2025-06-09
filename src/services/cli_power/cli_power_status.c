//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_power_status.c
 * Source file to implement power status CLI commands.
 */

/*------------- Includes -----------------*/

#include "DfwkCommon.h" // for _DFWK_ASYNC_REQUEST_HEADER
#include "power_dfwk.h" // for (anonymous), ppower_service_cli_req...
#include "power_hw_int_i.h"
#include "power_i.h" // for power_latest_calcs_t, power_runcon...

#include <FpFwUtils.h>           // for FPFW_UNUSED
#include <assert.h>              // for static_assert
#include <cli_power_common.h>    // for ST_COUNT
#include <cli_power_interface.h> // for power_cli_sub_command_dictionary_el...
#include <cli_power_status.h>
#include <core_mapping.h>
#include <corebits.h>
#include <inttypes.h>
#include <numa_config_variable.h>
#include <power_runconfig.h>
#include <stdint.h> // for uint32_t
#include <stdio.h>  // for printf, NULL
#include <string.h> // for strcmp

/*-- Symbolic Constant Macros (defines) --*/
#define CLI_SUCCESS 0

#define PWR_MW 1000

#define VR_CPU_IDX 0

/* helper for count of entries */
#define dimof(x) (sizeof(x) / sizeof(x[0]))

// state names for control loop
static const char* ctrl_loop_names[POWER_CONTROL_STATE_MAX] =
    {"Idle   ", "WsEntry", "CollInp", "ExchInp", "DistAvl", "PlmtBef", "VrAfter", "VrWtAft", "VrBefor", "VrWtBef", "PlmtAft", "ExchCmp", "Error  "};

// state names for vr telemetry
static const char* vrtelem_loop_names[POWER_VR_TELEM_STATE_MAX] = {"Idle   ", "CurrTel", "TempTel", "Error  "};

// state names for pvt telemetry
static const char* pvttelem_loop_names[POWER_PVT_TELEM_STATE_MAX] = {
    "Idle   ",
    "ReadPvt",
};
/*-- Declarations (Statics and globals) --*/
extern NUMA_CFG numa_cfg;

/*--------- Function Prototypes ----------*/

static void print_power_status_clinfo(ppower_service_cli_request_t p_cli_request);
static void print_power_status_vrtlinfo(ppower_service_cli_request_t p_cli_request);
static void print_power_status_pvttlinfo(ppower_service_cli_request_t p_cli_request);
static void print_power_status_capinfo(ppower_service_cli_request_t p_cli_request);
static void print_power_status_droopcount_info(ppower_service_cli_request_t p_cli_request);
static void print_power_status_maxtemp_info(ppower_service_cli_request_t p_cli_request);
static void print_power_status_power_info(ppower_service_cli_request_t p_cli_request);
static void print_power_status_plimit_info(ppower_service_cli_request_t p_cli_request);
static void print_power_status_primary_core_info(ppower_service_cli_request_t p_cli_request);
static void print_power_status_selections_info(ppower_service_cli_request_t p_cli_request);
static void print_power_status_vcpu_info(ppower_service_cli_request_t p_cli_request);
static void print_power_status_warmstart_info(ppower_service_cli_request_t p_cli_request);

static void print_cl_detail(power_ctrl_loop_detail_t* s_ctrl_loop, power_knobs_t* p_knobs);
static int32_t power_loops_info(power_loop_residency_t* residency,
                                unsigned max_state,
                                unsigned error_state,
                                bool loop_disabled,
                                const char** loop_state_names);
static int32_t power_status_vcpu(power_ctrl_loop_detail_t* s_ctrl_loop, power_knobs_t* p_knobs);

static void print_pvt_detail(ppower_service_cli_request_t p_cli_request);
static void print_pvt_entry(unsigned index, power_telemetry_pvt_state_t* state_data, bool voltage);
static void print_padding(uint32_t decimal, unsigned places);
static void print_voltage(uint16_t voltage);
static void print_temp(uint16_t temp);

// clang-format off
const power_cli_sub_command_dictionary_element_t power_cli_status_sub_command_dictionary[] = {
    {"cl",                NULL,            POWER_IF_CMD_STATUS_CL},
    {"vrtl",              NULL,            POWER_IF_CMD_STATUS_VRTL},
    {"pvttl",             NULL,            POWER_IF_CMD_STATUS_PVTTL},
    {"cap",               NULL,            POWER_IF_CMD_STATUS_CAP},
    {"droopcount",        NULL,            POWER_IF_CMD_STATUS_DROOPCOUNT},
    {"maxtemp",           NULL,            POWER_IF_CMD_STATUS_MAXTEMP},
    {"power",             NULL,            POWER_IF_CMD_STATUS_POWER},
    {"plimit",            NULL,            POWER_IF_CMD_STATUS_PLIMIT},
    {"primarycore",       NULL,            POWER_IF_CMD_STATUS_PRIMARY_CORE},
    {"selections",        NULL,            POWER_IF_CMD_STATUS_SELECTIONS},
    {"vcpu",              NULL,            POWER_IF_CMD_STATUS_VCPU},
    {"warmstart",         NULL,            POWER_IF_CMD_STATUS_WARMSTART},
};

//clang-format on



const uint32_t length_status_commands_dictionary =
    sizeof(power_cli_status_sub_command_dictionary) / sizeof(power_cli_sub_command_dictionary_element_t);

/*-------------- Functions ---------------*/

power_if_cmd_t cli_power_status_get_cmd_id(const char* sub_command)
{
    if (sub_command == NULL)
    {
        return POWER_IF_CMD_UNKNOWN;
    }

    /* Parse dictionary for subcommand and fetch corresponding power_ext_if_cmd_id */
    for (uint32_t index = 0; index < length_status_commands_dictionary; index++)
    {
        if (strcmp(sub_command, power_cli_status_sub_command_dictionary[index].sub_command) == 0)
        {
            return power_cli_status_sub_command_dictionary[index].power_ext_if_cmd_id;
        }
    }

    /* If sub-command is not found in the dictionary */
    return POWER_IF_CMD_UNKNOWN;
}

void cli_power_status_async_print(PDFWK_ASYNC_REQUEST_HEADER p_request, void* completion_context)
{

    FPFW_UNUSED(completion_context);

    ppower_service_cli_request_t p_cli_request = (ppower_service_cli_request_t)p_request;
    p_cli_request->header = *p_request;

    switch (p_cli_request->power_ext_if_cmd_id)
    {
        case POWER_IF_CMD_STATUS_CL:
            print_power_status_clinfo(p_cli_request);        
            break;
            
        case POWER_IF_CMD_STATUS_VRTL: 
            print_power_status_vrtlinfo(p_cli_request);                        
            break;
            
        case POWER_IF_CMD_STATUS_PVTTL:
            print_power_status_pvttlinfo(p_cli_request);                            
            break;
        case POWER_IF_CMD_STATUS_CAP:
            print_power_status_capinfo(p_cli_request);
            break;
        case POWER_IF_CMD_STATUS_DROOPCOUNT:
            print_power_status_droopcount_info(p_cli_request);
            break;
        case POWER_IF_CMD_STATUS_MAXTEMP:
            print_power_status_maxtemp_info(p_cli_request);
            break;
        case POWER_IF_CMD_STATUS_POWER:
            print_power_status_power_info(p_cli_request);
            break;
        case POWER_IF_CMD_STATUS_PLIMIT: 
            print_power_status_plimit_info(p_cli_request);
            break;
        case POWER_IF_CMD_STATUS_PRIMARY_CORE:  
            print_power_status_primary_core_info(p_cli_request);
            break;
        case POWER_IF_CMD_STATUS_SELECTIONS:    
            print_power_status_selections_info(p_cli_request);
            break;
        case POWER_IF_CMD_STATUS_VCPU:  
            print_power_status_vcpu_info(p_cli_request);
            break;  
        case POWER_IF_CMD_STATUS_WARMSTART: 
            print_power_status_warmstart_info(p_cli_request);
            break;
        default:
           break;
    }    

}


static void print_power_status_clinfo(ppower_service_cli_request_t p_cli_request) 
{

    power_loop_context_t* power_context = p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_ctrl_loop_context;
    power_ctrl_loop_detail_t* s_ctrl_loop = p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_ctrl_loop;   

    power_knobs_t* p_knobs = p_cli_request->fetch_data.pwr_intparams.p_knobs; 

    printf("SOC Power Module - Control loop info\n\n");

    bool disabled = ((p_cli_request->fetch_data.pwr_intparams.p_knobs->loops_disable & power_loops_disable_t_CTRL_LOOP) != 0);
    int32_t retval =
        power_loops_info(power_context->residency, POWER_CONTROL_STATE_MAX, POWER_CONTROL_STATE_ERROR, disabled, ctrl_loop_names);
    if (CLI_SUCCESS == retval) {
        print_cl_detail(s_ctrl_loop, p_knobs);
    }
}

static void print_cl_detail(power_ctrl_loop_detail_t* s_ctrl_loop, power_knobs_t* p_knobs)
{
    printf("\nFlags: ");
    printf(    "%s%s\n",
               (s_ctrl_loop->rack_limit) ? "(rack limit) " : "",
               (s_ctrl_loop->throttling) ? "(throttling) " : "");
    printf("Distributable Resources: %d/%d\n", s_ctrl_loop->curr_resources, s_ctrl_loop->max_resources);
    power_status_vcpu(s_ctrl_loop, p_knobs);
}


// "pwr status vcpu" command
static int32_t power_status_vcpu(power_ctrl_loop_detail_t* s_ctrl_loop, power_knobs_t* p_knobs)
{

    printf(   "\nVcpu required: %dmV   Vcpu currently: %dmV %s\n\n",
               s_ctrl_loop->required_vcpu,
               s_ctrl_loop->current_vcpu,
               ((p_knobs->forced_vrs.vr[VR_CPU_IDX] != 0) ? "(forced)" : ""));               

    return CLI_SUCCESS;
}

static int32_t power_loops_info(power_loop_residency_t *residency,
                                unsigned max_state,
                                unsigned error_state,
                                bool loop_disabled,
                                const char **loop_state_names)
{

    uint64_t total           = 0;
    uint64_t loop_busy       = 0;
    const uint64_t loop_idle = residency[POWER_CONTROL_STATE_IDLE].ticks;

    // Print health message; expect this to be useful for integration test
    printf("\nHealth: ");
    if (residency[POWER_CONTROL_STATE_IDLE].count < 2) {
        printf("insufficient data");
    } else if ((error_state == POWER_CONTROL_STATE_IDLE) || (residency[error_state].count == 0)) {
        printf("no errors");
    } else {
        printf("errors");
    }
    if (loop_disabled) {
        printf(" (disabled)");
    }
    printf("\n\n");

    printf("State    Iterations          Retries             MaxR  AvgR  Ticks in state        Avg Ticks\n");
    printf("============================================================================================\n");
    for (unsigned i = 0; i < max_state; ++i) {
        if (i != POWER_CONTROL_STATE_IDLE) {
            loop_busy += residency[i].ticks;
        }
        // printf is quite limited as far as formatting.. can only handle 9
        // spaces of padding, which is why some 64b values are split below
        printf(    "%s: 0x%08"PRIx32"%08"PRIx32"  0x%08"PRIx32"%08"PRIx32"  %04"PRId32"  %04"PRId32"  0x%08"PRIx32"%08"PRIx32"  0x%08"PRIx32" (%"PRId32")\n",
            loop_state_names[i],
            (uint32_t)(residency[i].count >> 32),
            (uint32_t)(residency[i].count),
            (uint32_t)(residency[i].retries >> 32),
            (uint32_t)(residency[i].retries),
            (uint32_t)(residency[i].max_retries),
            (uint32_t)(residency[i].retries/(residency[i].count?residency[i].count:1) ), 
            (uint32_t)(residency[i].ticks >> 32),
            (uint32_t)(residency[i].ticks),
            (uint32_t)(residency[i].ticks /(residency[i].count?residency[i].count:1 )), 
            (uint32_t)(residency[i].ticks/(residency[i].count?residency[i].count:1 ))); 
    }
    total                       = loop_busy + loop_idle;
    const uint32_t idle_percent = (uint32_t)((loop_idle * 100) / total);
      printf("\nIterations: 0x%"PRIx32"%08"PRIx32"  Busy ticks: 0x%"PRIx32"%08"PRIx32"  Idle ticks: 0x%"PRIx32"%08"PRIx32" (%"PRId32")\n",
               (uint32_t)(residency[POWER_CONTROL_STATE_IDLE].count >> 32),
               (uint32_t)(residency[POWER_CONTROL_STATE_IDLE].count),
               (uint32_t)(loop_busy >> 32),
               (uint32_t)(loop_busy),
               (uint32_t)(loop_idle >> 32),
               (uint32_t)(loop_idle),
               idle_percent);

    return 0; 
}



static void print_power_status_vrtlinfo(ppower_service_cli_request_t p_cli_request)
{

    power_loop_context_t* power_context = p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_vr_telem_loop_context;

    power_knobs_t* p_knobs = p_cli_request->fetch_data.pwr_intparams.p_knobs; 

    printf("SOC Power Module - VR telemetry loop info\n\n");

    bool disabled = ((p_knobs->loops_disable & power_loops_disable_t_VR_TELEM_LOOP) != 0);
    power_loops_info(power_context->residency, POWER_VR_TELEM_STATE_MAX, POWER_VR_TELEM_STATE_ERROR, disabled, vrtelem_loop_names);
       
}

static void print_power_status_pvttlinfo(ppower_service_cli_request_t p_cli_request)
{

    power_knobs_t* p_knobs = p_cli_request->fetch_data.pwr_intparams.p_knobs; 

    power_loop_context_t* power_context = p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_pvt_telem_loop_context;

    printf("SOC Power Module - PVT telemetry loop info\n\n");

    bool disabled  = ((p_knobs->loops_disable & power_loops_disable_t_PVT_TELEM_LOOP) != 0);
    int32_t retval = power_loops_info(power_context->residency, POWER_PVT_TELEM_STATE_MAX, POWER_CONTROL_STATE_IDLE, disabled, pvttelem_loop_names);
    if (CLI_SUCCESS == retval) {
        print_pvt_detail(p_cli_request);
    }
}


static void print_pvt_detail(ppower_service_cli_request_t p_cli_request)
{

    power_telem_loop_detail_t* p_telem_loop = p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_telem_loop;

    printf("\nSOC Top Voltage\n\n");
    printf("Id    High     Low      Last  (raw)\n");
    printf("=============================\n");
    for (unsigned i = 0; i < SOC_PVT_TOTAL_CHANNELS_VM; ++i) {
        print_pvt_entry(i, &p_telem_loop->soc_top_voltage_data[i], true);
    }
    printf("\nSOC Top Temperature\n\n");
    printf("Id    High     Low      Last  (raw)\n");
    printf("=============================\n");
    for (unsigned i = 0; i < SOC_PVT_TOTAL_CHANNELS_DTS; ++i) {
        print_pvt_entry(i, &p_telem_loop->soc_top_temp_data[i], false);
    }
    printf("\nLast SOC max: ");
    print_temp(p_telem_loop->soc_max_temp_dC);
    printf("\n");
}


static void print_pvt_entry(unsigned index, power_telemetry_pvt_state_t *state_data, bool voltage)
{
    void (*sample_print)(uint16_t) = NULL;
    printf("%02d: ", index);
    if (voltage) {
        sample_print = print_voltage;
    } else {
        sample_print = print_temp;
    }
    sample_print(state_data->sample_hi);
    printf("  ");
    sample_print(state_data->sample_lo);
    printf("  ");
    sample_print(state_data->last_sample);
    printf(" (0x%x)\n", state_data->last_sample_raw);
}


static void print_padding(uint32_t decimal, unsigned places)
{
    uint32_t value = 1;
    for (unsigned i = 0; i < places; ++i) {
        value *= 10;
        if (decimal < value) {
            printf(" ");
        }
    }
}
static void print_voltage(uint16_t voltage)
{
    print_padding(voltage, 4);
    printf("%dmV", voltage);
}
static void print_temp(uint16_t temp)
{
    uint16_t tenths = temp % 10;
    temp            = temp / 10;
    print_padding(temp, 3);
    printf("%d.%dC", temp, tenths);
}

static void print_power_status_capinfo(ppower_service_cli_request_t p_cli_request)
{
    power_knobs_t* p_knobs = p_cli_request->fetch_data.pwr_intparams.p_knobs;
    printf("\nSOC Power Cap : ");
    uint16_t soc_cap = get_current_soc_power_cap();
    if (soc_cap == NO_POWER_CAP) {
        printf("NONE\n");
    }
    else {
        printf("%dW\n", soc_cap);
    }
    printf("Vcpu Power Cap: %ffW\n", get_current_vrcpu_cap());
    printf("Inst Max Electrical Limit: Die0: %fW Die1: %fW\n", get_inst_max_electrical_limit(0), get_inst_max_electrical_limit(1));
    printf("Soc Max Thermal Limit: %dW\n", p_knobs->soc_maximum_thermal_watts_limit);
    printf("Soc Max Electrical Current Limit: (Vcpu0: %dA Vcpu1: %dA)\n", p_knobs->soc_maximum_electrical_current_limit_vcpu0, p_knobs->soc_maximum_electrical_current_limit_vcpu1);
}

static void print_power_status_droopcount_info(ppower_service_cli_request_t p_cli_request)
{
    FPFW_UNUSED(p_cli_request);
    power_runconfig_t* s_runconfig = power_runconfig_get();
    unsigned int core                                      = p_cli_request->pwrset_sub_command_args.minupdate_val;
    uint32_t droop_count                              = 0;

    if (core < NUM_AP_CORES_PER_DIE) {
            droop_count = power_hw_get_adclk_count(s_runconfig, core);
        printf("  core - %d  droopcount - %08" PRIx32 "\n", core, droop_count);
    } else {
        printf("  invalid core - %d\n", core);
    }
}

static void print_power_status_maxtemp_info(ppower_service_cli_request_t p_cli_request)
{
    power_ctrl_loop_detail_t* s_ctrl_loop = p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_ctrl_loop;
    power_telem_loop_detail_t* p_telem_loop = p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_telem_loop;
    printf("\nLast iteration core max temp: ");
    print_temp(p_telem_loop->soc_max_temp_dC * 10);  // core max temp is in degrees C and not tenths degrees C
    printf("\n\n");
    for (unsigned core_idx = 0; core_idx < NUM_AP_CORES_PER_DIE; ++core_idx) {
        power_core_t *core = &s_ctrl_loop->cores.core[core_idx];
        printf("\nCore %d temp: ", core_idx);
        print_temp(core->temperature_dC * 10);  // core max temp is in degrees C and not tenths degrees C
        printf("\n");
    }
    printf("\n\n");
}

static uint32_t power_vrs_get_recent_vcpu_power_mw()
{
    power_vrs_context_t* s_vrs_context = get_power_vrs_context();
    float total = 0.0F;
    for (int meas_idx = 0; meas_idx < SOC_POWER_AVG_COUNT; meas_idx++) {
        total += s_vrs_context->vcpu_power_log[meas_idx];
    }
    return (uint32_t)((total / SOC_POWER_AVG_COUNT) * 1000);
}

static uint32_t power_vrs_get_recent_vcpu_curr_ma()
{
    power_vrs_context_t* s_vrs_context = get_power_vrs_context();
    float total = 0.0F;
    for (int meas_idx = 0; meas_idx < SOC_POWER_AVG_COUNT; meas_idx++) {
        total += s_vrs_context->vcpu_current_log[meas_idx];
    }
    return (uint32_t)((total / SOC_POWER_AVG_COUNT) * 1000);
}

static void print_power_status_power_info(ppower_service_cli_request_t p_cli_request)
{
    FPFW_UNUSED(p_cli_request);
    power_vrs_context_t* s_vrs_context = get_power_vrs_context();
    printf("\nSOC power history: \n\n");
    for (unsigned index = 0; index < SOC_POWER_AVG_COUNT; ++index) {
        char socpwrstr[20];
        char remsocpwrstr[20];
        char voltstr[20];
        char currstr[20];
        snprintf(socpwrstr, sizeof(socpwrstr), "%fW", s_vrs_context->soc_power[index]);
        snprintf(remsocpwrstr, sizeof(remsocpwrstr), "%fW", s_vrs_context->remote_soc_power[index]);
        snprintf(voltstr, sizeof(voltstr), "%fW", s_vrs_context->vcpu_power_log[index]);
        snprintf(currstr, sizeof(currstr), "%fA", s_vrs_context->vcpu_current_log[index]);
        printf("  [%02d] Locally Cal SOC Power[%s]  Remote Fetched SOC Power[%s] VCPUPower[%s] VCPUCurr[%s] \n", index, socpwrstr, remsocpwrstr, voltstr, currstr);
    }
    unsigned avg_mw = power_vrs_get_recent_power_mw();
    printf("\nSOC Avg Power: %d.%03dW", avg_mw / 1000, avg_mw % 1000);
    printf("\n\n");
    avg_mw = power_vrs_get_recent_vcpu_power_mw();
    printf("\nVCPU Avg Power: %d.%03dW", avg_mw / 1000, avg_mw % 1000);
    printf("\n\n");
    avg_mw = power_vrs_get_recent_vcpu_curr_ma();
    printf("\nVCPU Avg Current: %d.%03dW", avg_mw / 1000, avg_mw % 1000);
    printf("\n\n");
}

static void print_power_status_plimit_info(ppower_service_cli_request_t p_cli_request)
{
    power_ctrl_loop_detail_t* s_ctrl_loop = p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_ctrl_loop;
    printf("\nCurrent/selected plimits\n\n");

    for (int count = 0; count < ST_COUNT; ++count) {
        printf("Core:Cur/Sel ");
    }
    printf("\n");
    for (int count = 0; count < ST_COUNT; ++count) {
        printf("=============");
    }
    printf("\n");
    for (unsigned core_idx = 0; core_idx < NUM_AP_CORES_PER_DIE; ++core_idx) {
        power_core_t *core = &s_ctrl_loop->cores.core[core_idx];
        printf("%04d:%03d/%03d ", core_idx, core->current_plimit, core->selected_plimit);
        if (core_idx % ST_COUNT == ST_COUNT - 1) {
            printf("\n");
        }
    }
    printf("\n");

    printf("Success msgs - Last: %" PRIu32 "s, Min: %" PRIu32 "s, Max: %" PRIu32 "s\n\n",
           s_ctrl_loop->plimit.last_us,
           s_ctrl_loop->plimit.min_us,
           s_ctrl_loop->plimit.max_us);
}

static void print_power_status_primary_core_info(ppower_service_cli_request_t p_cli_request)
{
    FPFW_UNUSED(p_cli_request);
    unsigned boot_core = 0xFF;
    power_runconfig_t* s_runconfig = power_runconfig_get();
    
    static_assert(NUM_AP_CORES_PER_DIE <= FPFW_ARRAY_SIZE(CORE_MAPPING_UMA), "Boot core mapping size unexpected");
    static_assert(NUM_AP_CORES_PER_DIE <= FPFW_ARRAY_SIZE(CORE_MAPPING_NUMA), "Boot core mapping size unexpected");

    const CONFIG_TYPE_UINT8* core_mapping = CORE_MAPPING_UMA;

    if (numa_cfg.NumaEnabled)
    {
        core_mapping = CORE_MAPPING_NUMA;
    }
    for (unsigned int core_idx = 0; core_idx < s_runconfig->p_sconfig->platform_die_core_count; ++core_idx)
    {
        if (corebits_is_bit_set(&s_runconfig->fuses.valid_cores, core_mapping[core_idx]))
        {
            boot_core = core_mapping[core_idx];
            break;
        }
    }
    
    printf("\nBoot Core: %u\n", boot_core);
}

void print_perc_bar(uint64_t max, uint64_t current, unsigned tick_count)
{
    printf("[");
    // avoid div 0
    max                      = FPFW_MAX(max, 1ULL);
    const unsigned num_ticks = (unsigned)((current * tick_count) / max);
    for (unsigned tick_idx = 0; tick_idx < tick_count; tick_idx++) {
        printf("%s", (tick_idx < num_ticks) ? "*" : " ");
    }
    printf("]");
}

static void print_power_status_selections_info(ppower_service_cli_request_t p_cli_request)
{
    power_ctrl_loop_detail_t* s_ctrl_loop = p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_ctrl_loop;
    power_runconfig_t* s_runconfig = power_runconfig_get();
    bool clear_selection = (p_cli_request->pwrset_sub_command_args.minupdate_val == 1 ? true : false);
    printf("\nPlimit selections per priority level\n\n");

    for (int pri = 0; pri < VM_THROT_COUNT; ++pri) {
        bool printed       = false;
        uint64_t max_count = 0;
        // find max
        for (int pstate = 0; pstate < NUM_PSTATES; ++pstate) {
            const uint64_t count = s_ctrl_loop->plimit.selections[pri].acc[pstate];
            max_count            = FPFW_MAX(max_count, count);
        }
        // display counts per pstate
        for (int pstate = 0; pstate < NUM_PSTATES; ++pstate) {
            const uint64_t count = s_ctrl_loop->plimit.selections[pri].acc[pstate];
            // print if there are counts; always print nominal
            if ((count > 0) || ((pstate == s_runconfig->derived.pnominal) && (max_count > 0))) {
                printf("Pri%d, P%02d: %016" PRIx64 " ", pri, pstate, count);
                // display percent of max as a text bar
                print_perc_bar(max_count, count, 25);
                printf("\n");
                printed = true;
            }
        }
        if (printed) {
            printf("\n");
        }
    }
    if (clear_selection) {
        // clear stats
        for (int pri = 0; pri < VM_THROT_COUNT; ++pri) {
            for (int pstate = 0; pstate < NUM_PSTATES; ++pstate) {
                s_ctrl_loop->plimit.selections[pri].acc[pstate] = 0;
            }
        }
        printf("Counts cleared.\n");
    }
    printf("\n");
}

static void print_power_status_vcpu_info(ppower_service_cli_request_t p_cli_request)
{
    power_ctrl_loop_detail_t* s_ctrl_loop = p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_ctrl_loop;
    power_knobs_t* p_knobs = p_cli_request->fetch_data.pwr_intparams.p_knobs;
    power_status_vcpu(s_ctrl_loop, p_knobs);
}   

static void print_power_status_warmstart_info(ppower_service_cli_request_t p_cli_request)
{
    FPFW_UNUSED(p_cli_request);
    printf("Not Implemented!!\n");
}
