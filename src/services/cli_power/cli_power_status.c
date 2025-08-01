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
#include <dvfs.h>
#include <dvfs_nonsecure_regs.h> // for DVFS_NONSECURE_REGS
#include <dvfs_regs.h>
#include <inttypes.h>
#include <numa_config_variable.h>
#include <pex_regs.h>
#include <power_runconfig.h>
#include <scp_exp_csr_regs.h>
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
static void print_power_status_force_pmin(ppower_service_cli_request_t p_cli_request);
static void print_power_status_core_info(ppower_service_cli_request_t p_cli_request);
static void print_power_status_prio_cnt(ppower_service_cli_request_t p_cli_request);
static void print_power_status_prio_core(ppower_service_cli_request_t p_cli_request);
static void print_power_status_dvfs_fsm(ppower_service_cli_request_t p_cli_request);
static void print_power_status_vr_inst(ppower_service_cli_request_t p_cli_request);
static void print_power_status_dvfs_plimit(ppower_service_cli_request_t p_cli_request);
static void print_power_status_dvfs_cppc(ppower_service_cli_request_t p_cli_request);
static void print_power_status_pstate2cppc(ppower_service_cli_request_t p_cli_request);
static void print_power_status_dvfs_throt_sr(ppower_service_cli_request_t p_cli_request);
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
    {"forcepmin",         NULL,            POWER_IF_CMD_STATUS_FORCE_PMIN},
    {"core",              NULL,            POWER_IF_CMD_STATUS_CORE},
    {"dvfs_fsm",          NULL,            POWER_IF_CMD_STATUS_DVFS_FSM},
    {"dvfs_plimit",       NULL,            POWER_IF_CMD_STATUS_DVFS_PLIMIT},
    {"dvfs_cppc",         NULL,            POWER_IF_CMD_STATUS_DVFS_CPPC},
    {"core_prio",         NULL,            POWER_IF_CMD_STATUS_CORE_PRIO},
    {"prio_cnt",          NULL,            POWER_IF_CMD_STATUS_PRIO_CNT},
    {"vr_inst",           NULL,            POWER_IF_CMD_STATUS_VR_INST},
    {"pstate2cppc",       NULL,            POWER_IF_CMD_STATUS_PSTATE2CPPC},
    {"dvfs_throt_sr",     NULL,            POWER_IF_CMD_STATUS_DVFS_THROT_SR},
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
        case POWER_IF_CMD_STATUS_FORCE_PMIN:
            print_power_status_force_pmin(p_cli_request);
            break;
        case POWER_IF_CMD_STATUS_CORE:
            print_power_status_core_info(p_cli_request);
            break;
        case POWER_IF_CMD_STATUS_DVFS_FSM:
            print_power_status_dvfs_fsm(p_cli_request);
            break;
        case POWER_IF_CMD_STATUS_DVFS_PLIMIT:
            print_power_status_dvfs_plimit(p_cli_request);
            break;
        case POWER_IF_CMD_STATUS_DVFS_CPPC:
            print_power_status_dvfs_cppc(p_cli_request);
            break;
        case POWER_IF_CMD_STATUS_CORE_PRIO:
            print_power_status_prio_core(p_cli_request);
            break;
        case POWER_IF_CMD_STATUS_PRIO_CNT:
            print_power_status_prio_cnt(p_cli_request);
            break;
        case POWER_IF_CMD_STATUS_VR_INST:
            print_power_status_vr_inst(p_cli_request);
            break;
        case POWER_IF_CMD_STATUS_PSTATE2CPPC:
            print_power_status_pstate2cppc(p_cli_request);
            break;
        case POWER_IF_CMD_STATUS_DVFS_THROT_SR:
            print_power_status_dvfs_throt_sr(p_cli_request);
            break;
        default:
           break;
    }    

    // Print completion message
    printf("pwr_cli_comp\n");
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
    printf("%s%s%s%s%s%s\n",
           (s_ctrl_loop->rack_limit) ? "(rack limit) " : "",
           (s_ctrl_loop->throttling) ? "(throttling) " : "",
           (s_ctrl_loop->throttle_query) ? "(throttle_query) " : "",
           (s_ctrl_loop->loop_failure) ? "(loop_failure) " : "",
           (s_ctrl_loop->loop_fail_query) ? "(loop_fail_query) " : "",
           "");
    printf("Distributable Resources: %d/%d\n", s_ctrl_loop->curr_resources, s_ctrl_loop->max_resources);
    power_status_vcpu(s_ctrl_loop, p_knobs);

    // Print snapshot of local and remote power from control loop
    printf("Control Loop Power Snapshot (Local):\n");
    printf("  SOC Power: %.3f W\n", s_ctrl_loop->local.power.soc_power);
    printf("  VCPU Power: %.3f W\n", s_ctrl_loop->local.power.vcpu_power);
    printf("  VCPU AVS Voltage: %u mV\n", s_ctrl_loop->local.power.vcpu_avs_voltage);
    printf("  VCPU AVS Current: %u mA\n", s_ctrl_loop->local.power.vcpu_avs_current);

    printf("\nControl Loop Power Snapshot (Remote):\n");
    printf("  SOC Power: %.3f W\n", s_ctrl_loop->remote.power.soc_power);
    printf("  VCPU Power: %.3f W\n", s_ctrl_loop->remote.power.vcpu_power);
    printf("  VCPU AVS Voltage: %u mV\n", s_ctrl_loop->remote.power.vcpu_avs_voltage);
    printf("  VCPU AVS Current: %u mA\n", s_ctrl_loop->remote.power.vcpu_avs_current);

    printf("\nLocal Max Resources: %u\n", s_ctrl_loop->local.max_resources);
    printf("Remote Max Resources: %u\n", s_ctrl_loop->remote.max_resources);

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

static void print_power_status_prio_core(ppower_service_cli_request_t p_cli_request)
{
    power_ctrl_loop_detail_t* s_ctrl_loop = p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_ctrl_loop;
    power_runconfig_t* s_runconfig = power_runconfig_get();
    const CONFIG_TYPE_UINT8* core_mapping = CORE_MAPPING_UMA;
    if (numa_cfg.NumaEnabled) {
        core_mapping = CORE_MAPPING_NUMA;
    }
    printf("\nThrottle and Boost Priority per enabled core:\n");
    printf("Core  Mapping  Throttle Priority  Boost Priority\n");
    printf("===============================================\n");
    for (unsigned core_idx = 0; core_idx < NUM_AP_CORES_PER_DIE; ++core_idx) {
        // Check if core is enabled
        if (corebits_is_bit_set(&s_runconfig->fuses.valid_cores, core_idx)) {
            printf("%4u    %4u    ", core_idx, core_mapping[core_idx]);
            // Print throttle priorities for this core
            for (int pri = 0; pri < VM_THROT_COUNT; ++pri) {
                if (corebits_is_bit_set(&s_ctrl_loop->throttle_priority[pri], core_idx)) {
                    printf("%d ", pri);
                }
            }
            printf("           ");
            // Print boost priorities for this core
            for (int pri = 0; pri < VM_THROT_COUNT; ++pri) {
                if (corebits_is_bit_set(&s_ctrl_loop->boost_priority[pri], core_idx)) {
                    printf("%d ", pri);
                }
            }
            printf("\n");
        }
    }
}

static void print_power_status_prio_cnt(ppower_service_cli_request_t p_cli_request)
{
    power_ctrl_loop_detail_t* s_ctrl_loop = p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_ctrl_loop;
    
    // Print throttle/boost core count data for local and remote
    printf("\nThrottle/Boost Core Count Data (Local):\n");
    printf("Throttle Priority Counts:\n");
    printf("Throttle Priority | Number of Cores\n");
    printf("------------------|----------------\n");
    for (int i = 0; i < VM_THROT_COUNT; ++i) {
        printf("        %2d        |       %3u\n", i, s_ctrl_loop->local.pri_counts.throt_pri_count[i]);
    }
    printf("Required Nominal for Throttle (Number of Cores):\n");
    // Print table header
    printf("Pstate |");
    for (int v = 0; v < VM_PRI_COUNT; ++v) {
        printf(" ThrotPri %2d |", v);
    }
    printf("\n");

    // Print separator
    printf("-------|");
    for (int v = 0; v < VM_PRI_COUNT; ++v) {
        printf("-------------|");
    }
    printf("\n");

    // Print table rows
    for (int p = 0; p < NUM_PSTATES; ++p) {
        printf("  %2d   |", p);
        for (int v = 0; v < VM_PRI_COUNT; ++v) {
            printf("     %3u     |", s_ctrl_loop->remote.pri_counts.required_nominal_for_throttle[p][v]);
        }
        printf("\n");
    }

    printf("Required for Boost (Number of Cores):\n");
    printf("Pstate |");
    for (int v = 0; v < VM_PRI_COUNT; ++v) {
        printf(" BoostPri %2d |", v);
    }
    printf("\n");

    // Print separator
    printf("-------|");
    for (int v = 0; v < VM_PRI_COUNT; ++v) {
        printf("-------------|");
    }
    printf("\n");

    // Print matrix rows: each row is a Pstate, columns are Boost Priority
    for (int p = 0; p < NUM_PSTATES; ++p) {
        printf("  %2d   |", p);
        for (int v = 0; v < VM_PRI_COUNT; ++v) {
            printf("     %3u     |", s_ctrl_loop->remote.pri_counts.required_for_boost[p][v]);
        }
        printf("\n");
    }

    printf("\nThrottle/Boost Core Count Data (Remote):\n");
    printf("Throttle Priority | Number of Cores\n");
    printf("------------------|----------------\n");
    for (int i = 0; i < VM_THROT_COUNT; ++i) {
        printf("        %2d        |       %3u\n", i, s_ctrl_loop->remote.pri_counts.throt_pri_count[i]);
    }
    printf("Required Nominal for Throttle (Number of Cores):\n");
    // Print table header
    printf("Pstate |");
    for (int v = 0; v < VM_PRI_COUNT; ++v) {
        printf(" ThrotPri %2d |", v);
    }
    printf("\n");

    // Print separator
    printf("-------|");
    for (int v = 0; v < VM_PRI_COUNT; ++v) {
        printf("-------------|");
    }
    printf("\n");

    // Print table rows
    for (int p = 0; p < NUM_PSTATES; ++p) {
        printf("  %2d   |", p);
        for (int v = 0; v < VM_PRI_COUNT; ++v) {
            printf("     %3u     |", s_ctrl_loop->remote.pri_counts.required_nominal_for_throttle[p][v]);
        }
        printf("\n");
    }
    
    // Print table header for Boost Priority matrix
    printf("Required for Boost (Number of Cores):\n");
    printf("Pstate |");
    for (int v = 0; v < VM_PRI_COUNT; ++v) {
        printf(" BoostPri %2d |", v);
    }
    printf("\n");

    // Print separator
    printf("-------|");
    for (int v = 0; v < VM_PRI_COUNT; ++v) {
        printf("-------------|");
    }
    printf("\n");

    // Print matrix rows: each row is a Pstate, columns are Boost Priority
    for (int p = 0; p < NUM_PSTATES; ++p) {
        printf("  %2d   |", p);
        for (int v = 0; v < VM_PRI_COUNT; ++v) {
            printf("     %3u     |", s_ctrl_loop->remote.pri_counts.required_for_boost[p][v]);
        }
        printf("\n");
    }
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
    printf("Vcpu Power Cap: %fW\n", get_current_vrcpu_cap());
    printf("Inst Max Electrical Limit: Die0: %fW Die1: %fW\n", get_inst_max_electrical_limit(0), get_inst_max_electrical_limit(1));
    printf("Soc Max Thermal Limit: %dW\n", p_knobs->soc_maximum_thermal_watts_limit);
    printf("Soc Max Electrical Current Limit: (Vcpu0: %dA Vcpu1: %dA)\n", p_knobs->soc_maximum_electrical_current_limit_vcpu0, p_knobs->soc_maximum_electrical_current_limit_vcpu1);
}

static void print_power_status_droopcount_info(ppower_service_cli_request_t p_cli_request)
{
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
    printf("\nVCPU Avg Current: %d.%03dA", avg_mw / 1000, avg_mw % 1000);
    printf("\n\n");

    printf("Latest Local Power Measurements:\n");
    printf("  SOC Power: %.3f W\n", s_vrs_context->latest_power.soc_power);
    printf("  VCPU Power: %.3f W\n", s_vrs_context->latest_power.vcpu_power);
    printf("  VCPU AVS Voltage: %u mV\n", s_vrs_context->latest_power.vcpu_avs_voltage);
    printf("  VCPU AVS Current: %u mA\n", s_vrs_context->latest_power.vcpu_avs_current);
}

static void print_power_status_vr_inst(ppower_service_cli_request_t p_cli_request)
{
    FPFW_UNUSED(p_cli_request);
    power_runconfig_t* s_runconfig = power_runconfig_get();
    power_vrs_context_t* s_vrs_context = get_power_vrs_context();
    static const char* vr_names[10] = {
        "VCPU0",      // 0 Die0
        "VPCIE0P75",  // 1 Die0
        "VSYS",       // 2 Die0
        "VDDQ1P1",    // 3 Die0
        "VSOC",       // 4 Die0
        "VD2D0P55",   // 5 Die0
        "VPLL1P2",    // 6 Die0
        "VGPIO1P2",   // 7 Die0
        "VCPU1",      // 8 Die1
        "VD2D0P875"   // 9 Die1
    };

    printf("\nInstantaneous VR Inputs:\n");
    for (int vr_idx = s_runconfig->p_sconfig->vr_idx_info.flattened_vr_start_idx - s_runconfig->p_sconfig->vr_idx_info.vcpu_idx; vr_idx < s_runconfig->p_sconfig->num_vr; ++vr_idx) {
        power_vrs_avs_latest_t* vr = &s_vrs_context->vr_inputs[vr_idx];
        const char* vr_name = (vr_idx < (int)(sizeof(vr_names)/sizeof(vr_names[0]))) ? vr_names[vr_idx + s_runconfig->p_sconfig->vr_idx_info.vcpu_idx] : "UNKNOWN";
        printf("  VR[%d] (%s): Voltage = %dmV, Current = %dmA, Temp = %d.%dC\n",
               vr_idx,
               vr_name,
               vr->voltage,
               vr->current * 10,
               vr->temperature / 10,
               vr->temperature % 10);
    }
    printf("\n");
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

    printf("Pending plimits: ");
    bool any_pending = false;
    for (unsigned core_idx = 0; core_idx < NUM_AP_CORES_PER_DIE; ++core_idx) {
        if (corebits_is_bit_set(&s_ctrl_loop->plimits_pending, core_idx)) {
            printf("%d ", core_idx);
            any_pending = true;
        }
    }
    if (!any_pending) {
        printf("NONE");
    }
    printf("\n");
    printf("Successful plimits: ");
    bool any_successful = false;
    for (unsigned core_idx = 0; core_idx < NUM_AP_CORES_PER_DIE; ++core_idx) {
        if (corebits_is_bit_set(&s_ctrl_loop->plimits_successful, core_idx)) {
            printf("%d ", core_idx);
            any_successful = true;
        }
    }
    if (!any_successful) {
        printf("NONE");
    }
    printf("\n");
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

static void print_power_status_force_pmin(ppower_service_cli_request_t p_cli_request)
{
    FPFW_UNUSED(p_cli_request);
    power_runconfig_t* runconfig = power_runconfig_get();
    vptr_scp_exp_csr_reg scp_exp_csr_regs_base = (vptr_scp_exp_csr_reg)runconfig->p_sconfig->scp_exp_csr_base;
    printf("\nForce Pmin Reg addr:0x%x  fw_pmin_control = %d lockup_ue_rr = %d\n", (uintptr_t)&scp_exp_csr_regs_base->force_pmin_reg, scp_exp_csr_regs_base->force_pmin_reg.fw_pmin_control, scp_exp_csr_regs_base->force_pmin_reg.lockup_ue_rr);
}

static void print_power_status_warmstart_info(ppower_service_cli_request_t p_cli_request)
{
    FPFW_UNUSED(p_cli_request);
    printf("Not Implemented!!\n");
}

static void print_power_status_core_info(ppower_service_cli_request_t p_cli_request)
{
    power_ctrl_loop_detail_t* s_ctrl_loop = p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_ctrl_loop;
    printf("\nCore Info:\n");
    
    // Table 1: Core, Temp, PState, CState, ThrotStatus, ThrotPriority, BoostPriority, Plimit, SelPlimit, DesPstate, BasePstate, MinPlimit
    printf("Core  Temp     PState  CState  ThrotStatus  ThrotPriority  BoostPriority  Plimit  SelPlimit  DesPstate  BasePstate  MinPlimit\n");
    printf("=============================================================================================================================\n");
    for (unsigned core_idx = 0; core_idx < NUM_AP_CORES_PER_DIE; ++core_idx) {
        power_core_t *core = &s_ctrl_loop->cores.core[core_idx];
        printf("%4u  %3d.%1dC  %7u  %6u  %11u  %13u  %13u  %6u  %9u  %9u  %10u  %9u\n",
               core_idx,
               core->temperature_dC / 10,
               core->temperature_dC % 10,
               core->current_pstate,
               core->current_cstate,
               core->current_throt_status,
               core->current_throt_priority,
               core->current_boost_priority,
               core->current_plimit,
               core->selected_plimit,
               core->desired_pstate,
               core->current_base_pstate,
               core->min_plimit);
    }

    printf("\n");

    // Table 2: DesPstateInUse, DesPstateForCount, DesPstateCount, PlimitT1, PlimitT2, PlimitT3
    printf("Core  DesPstateInUse  DesPstateForCount  DesPstateCount  PlimitT1    PlimitT2    PlimitT3\n");
    printf("=========================================================================================\n");
    for (unsigned core_idx = 0; core_idx < NUM_AP_CORES_PER_DIE; ++core_idx) {
        power_core_t *core = &s_ctrl_loop->cores.core[core_idx];
        printf("%4u  %15u  %17u  %15u  0x%06X  0x%06X  0x%06X\n",
               core_idx,
               core->desired_pstate_in_use,
               core->desired_pstate_for_count,
               core->desired_pstate_count,
               core->plimit_t1,
               core->plimit_t2,
               core->plimit_t3);
    }
    
}

static void print_power_status_pstate2cppc(ppower_service_cli_request_t p_cli_request)
{
    FPFW_UNUSED(p_cli_request);
    printf("\nPstate/CPPC/Freq Conversion Table:\n");
    printf("Pstate  CPPC Value (hex)  Frequency (MHz)\n");
    printf("==========================================\n");
    for (unsigned pstate_index = 0; pstate_index < NUM_PSTATES; ++pstate_index) {
        uint8_t cppc_value = dvfs_get_cppc_from_pstate(pstate_index);
        uint16_t freq_mhz  = dvfs_get_freq_from_plimit(pstate_index);
        printf("%6u      0x%02X         %15u\n", pstate_index, cppc_value, freq_mhz);
    }
}

static void print_power_status_dvfs_cppc(ppower_service_cli_request_t p_cli_request)
{
    FPFW_UNUSED(p_cli_request);
    power_runconfig_t* s_runconfig = power_runconfig_get();
    uint8_t cppc_desired = 0;
    uint8_t cppc_base_perf = 0;
    uint8_t throttle_pri = 0;
    uint8_t boost_pri = 0;

    // Print combined table header
    printf("\nCPPC Combined Info Table:\n");
    printf("+------------+------------+--------------+-----------+--------------+-----------+---------------+---------------------+---------------+---------------+---------------+---------------+---------------+---------------+\n");
    printf("| Core Index |  Address   | CPPD Desired | Base Perf | Throttle Pri | Boost Pri | Lowest Perf   | Lowest Nonlin Perf  | Nominal Perf  | Highest Perf  | Lowest Freq   | Nominal Freq  | GTD Perf      | Perf LTD      |\n");
    printf("+------------+------------+--------------+-----------+--------------+-----------+---------------+---------------------+---------------+---------------+---------------+---------------+---------------+---------------+\n");

    for (unsigned int core = 0; core < NUM_AP_CORES_PER_DIE; ++core)
    {
        const uintptr_t cluster_pex_base_addr =
            (s_runconfig->p_sconfig->cluster_pex_base + (s_runconfig->p_sconfig->cluster_stride * core));
        const bool core_enabled = corebits_is_bit_set(&s_runconfig->fuses.valid_cores, core);

        if (corebits_is_bit_set(s_runconfig->p_sconfig->platform_cores_in_die, core) && core_enabled)
        {
            // Fetch CPPC desired perf
            int ret = dvfs_ns_get_cppc_desired_perf(cluster_pex_base_addr, &cppc_desired, &cppc_base_perf, &throttle_pri, &boost_pri);

            // Fetch perf threshold
            vptr_dvfs_nonsecure_cppc_perf_threshold_sr perf_threshold_sr = (vptr_dvfs_nonsecure_cppc_perf_threshold_sr)(cluster_pex_base_addr + PEX_DVFS_NON_ADDRESS + 0x4);
            // Fetch freq threshold
            vptr_dvfs_nonsecure_cppc_freq_threshold_sr freq_threshold_sr = (vptr_dvfs_nonsecure_cppc_freq_threshold_sr)(cluster_pex_base_addr + PEX_DVFS_NON_ADDRESS + 0x8);
            // Fetch GTD perf
            vptr_dvfs_nonsecure_cppc_gtd_perf_sr gtd_perf_sr = (vptr_dvfs_nonsecure_cppc_gtd_perf_sr)(cluster_pex_base_addr + PEX_DVFS_NON_ADDRESS + 0xC);
            // Fetch perf ltd
            vptr_dvfs_nonsecure_cppc_perf_ltd perf_ltd = (vptr_dvfs_nonsecure_cppc_perf_ltd)(cluster_pex_base_addr + PEX_DVFS_NON_ADDRESS + 0x10);

            if (ret != 0) {
                printf("| %10u | 0x%08" PRIxPTR " |   ERROR(%2d)   |           |              |           |               |                     |               |               |               |               |               |               |\n",
                       core, (uintptr_t)cluster_pex_base_addr, ret);
                continue;
            }

            printf("| %10u | 0x%08" PRIxPTR " |     0x%02X     |   0x%02X   |     0x%02X     |   0x%02X   |     0x%04X     |        0x%04X        |     0x%04X     |    0x%04X     |  %7u      |  %7u      |    0x%04X     |    0x%04X     |\n",
                   core,
                   (uintptr_t)cluster_pex_base_addr,
                   cppc_desired,
                   cppc_base_perf,
                   throttle_pri,
                   boost_pri,
                   perf_threshold_sr->lowest_perf,
                   perf_threshold_sr->lowest_nonlin_perf,
                   perf_threshold_sr->nominal_perf,
                   perf_threshold_sr->highest_perf,
                   freq_threshold_sr->lowest_freq,
                   freq_threshold_sr->nominal_freq,
                   gtd_perf_sr->gtd_perf,
                   perf_ltd->desired_exc);
        } else {
            printf("| %10u | 0x%08" PRIxPTR " |   DISABLED     |           |              |           |               |                     |               |               |               |               |               |               |\n",
                   core, (uintptr_t)cluster_pex_base_addr);
        }
    }
    printf("+------------+------------+--------------+-----------+--------------+-----------+---------------+---------------------+---------------+---------------+---------------+---------------+---------------+---------------+\n");
}

static void print_power_status_dvfs_plimit(ppower_service_cli_request_t p_cli_request)
{
    FPFW_UNUSED(p_cli_request);
    power_runconfig_t* s_runconfig = power_runconfig_get();
    // Print combined table header
    printf("\nPLIMIT REG INFO Table:\n");
    printf("| Core Index |   Address   | VF Index | Power Cap | CurrThresh1 | CurrThresh2 | CurrThresh3 |\n");
    printf("|------------|-------------|----------|-----------|-------------|-------------|-------------|\n");

    for (unsigned int core = 0; core < NUM_AP_CORES_PER_DIE; ++core)
    {
        const uintptr_t cluster_pex_base_addr =
            (s_runconfig->p_sconfig->cluster_pex_base + (s_runconfig->p_sconfig->cluster_stride * core));
        vptr_dvfs_plimit plimit_reg =
            (vptr_dvfs_plimit)(cluster_pex_base_addr + PEX_DVFS_ROOT_ADDRESS + DVFS_PLIMIT_ADDRESS);
        const bool core_enabled = corebits_is_bit_set(&s_runconfig->fuses.valid_cores, core);
        if (corebits_is_bit_set(s_runconfig->p_sconfig->platform_cores_in_die, core) && core_enabled)
        {
            // Fetch PLIMIT
                printf("| %10u | 0x%08" PRIxPTR " |     %02d     |     %d     |      %d      |      %d      |      %d      |\n",
                       core,
                       (uintptr_t)plimit_reg,
                       plimit_reg->vf_index,
                       plimit_reg->power_cap,
                       plimit_reg->currthresh_1,
                       plimit_reg->currthresh_2,
                       plimit_reg->currthresh_3);
        } else {
            printf("| %10u | 0x%08" PRIxPTR " |   DISABLED   |           |             |             |             |\n",
                   core, (uintptr_t)plimit_reg);
        }
    }
    printf("|------------|-------------|----------|-----------|-------------|-------------|-------------|\n");
}

static void print_power_status_dvfs_fsm(ppower_service_cli_request_t p_cli_request)
{
    FPFW_UNUSED(p_cli_request);
    power_runconfig_t* s_runconfig = power_runconfig_get();
    const char* vfsm_state_table[9] = {"PSM","PCT","PCTE","DECV","DECF","INCF","INCV","EVAL","RST"};
    // Print combined table header
    printf("\nDVFS FSM SR Table:\n");
    printf("| Core Index |   Address   |  VFSM State  | PState | CPPC Desired PState |\n");
    printf("|------------|-------------|--------------|--------|---------------------|\n");

    for (unsigned int core = 0; core < NUM_AP_CORES_PER_DIE; ++core)
    {
        const uintptr_t cluster_pex_base_addr =
            (s_runconfig->p_sconfig->cluster_pex_base + (s_runconfig->p_sconfig->cluster_stride * core));
        vptr_dvfs_dvfs_fsm_sr fsm_sr = (vptr_dvfs_dvfs_fsm_sr)(cluster_pex_base_addr + PEX_DVFS_ROOT_ADDRESS + DVFS_DVFS_FSM_SR_ADDRESS);
        const bool core_enabled = corebits_is_bit_set(&s_runconfig->fuses.valid_cores, core);
        if (corebits_is_bit_set(s_runconfig->p_sconfig->platform_cores_in_die, core) && core_enabled)
        {
           
            uint16_t vfsm_state_idx = 0;
            for (uint16_t i = 0; i < 9; ++i) {
                if (fsm_sr->vfsm_state & (1 << i)) {
                    vfsm_state_idx = i;
                    break;
                }
            }
            const char* vfsm_state_str = (vfsm_state_idx < (sizeof(vfsm_state_table)/sizeof(vfsm_state_table[0])))
                ? vfsm_state_table[vfsm_state_idx]
                : "UNK";
            printf("| %10u | 0x%08" PRIxPTR " | %10s   |   %02d  |         %02d        |\n",
                   core,
                   (uintptr_t)fsm_sr,
                   vfsm_state_str,
                   fsm_sr->vfsm_pstate,
                   fsm_sr->cppc_desired_pstate);
        } else {
            printf("| %10u | 0x%08" PRIxPTR " |   DISABLED   |           |                     |\n",
                   core, (uintptr_t)fsm_sr);
        }
    }
    printf("|------------|-------------|--------------|--------|---------------------|\n");
}

static void print_power_status_dvfs_throt_sr(ppower_service_cli_request_t p_cli_request)
{
    unsigned int core  = p_cli_request->pwrset_sub_command_args.minupdate_val;
    power_runconfig_t* s_runconfig = power_runconfig_get();

    if (core < NUM_AP_CORES_PER_DIE) {
        const uintptr_t cluster_pex_base_addr =
            (s_runconfig->p_sconfig->cluster_pex_base + (s_runconfig->p_sconfig->cluster_stride * core));
        uintptr_t dvfs_throt_sr = cluster_pex_base_addr + PEX_DVFS_ROOT_ADDRESS + DVFS_DVFS_THROT_SR_ADDRESS;
        parse_dvfs_throttle_status(dvfs_throt_sr);
    } else {
        printf("  invalid core - %d\n", core);
    }
}