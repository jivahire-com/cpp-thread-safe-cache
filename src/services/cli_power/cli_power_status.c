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

#include <FpFwUtils.h>           // for FPFW_UNUSED
#include <assert.h>              // for static_assert
#include <cli_power_common.h>    // for ST_COUNT
#include <cli_power_interface.h> // for power_cli_sub_command_dictionary_el...
#include <cli_power_status.h>
#include <inttypes.h>
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

/*--------- Function Prototypes ----------*/

static void print_power_status_clinfo(ppower_service_cli_request_t p_cli_request);
static void print_power_status_vrtlinfo(ppower_service_cli_request_t p_cli_request);
static void print_power_status_pvttlinfo(ppower_service_cli_request_t p_cli_request);

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
    {"cl",                NULL,               POWER_IF_CMD_STATUS_CL},
    {"vrtl",              NULL,             POWER_IF_CMD_STATUS_VRTL},
    {"pvttl",             NULL,            POWER_IF_CMD_STATUS_PVTTL},

};

//clang-format on



const uint32_t length_status_commands_dictionary =
    sizeof(power_cli_status_sub_command_dictionary) / sizeof(power_cli_sub_command_dictionary_element_t);

/*-------------- Functions ---------------*/

power_if_cmd_t cli_power_status_get_cmd_id(char* sub_command)
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
        // cli_printf is quite limited as far as formatting.. can only handle 9
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


