//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_power.c
 * Source file for power CLI definition
 */

/*------------- Includes -----------------*/
#include "FpFwCli.h"        // for FPFW_CLI_STATUS, CLI_ERROR, FpFwCli...
#include "FpFwLinkedList.h" // for NULL_LIST_ENTRY

#include <DbgPrint.h>
#include <DfwkClient.h>           // for DfwkAsyncRequestInitialize, DfwkAs...
#include <FpFwUtils.h>            // for FPFW_ARRAY_SIZE
#include <cli_power.h>            // for cli_power_init
#include <cli_power_accel.h>      // for cli_power_accel_cmd_id
#include <cli_power_config.h>     // for cli_power_config_get_cmd_id, cli_po...
#include <cli_power_interface.h>  // for cli_power_cmd_context_t
#include <cli_power_log.h>        // for cli_power_log_async_print
#include <cli_power_set.h>        // for cli_power_set_get_cmd_id, cli_power...
#include <cli_power_status.h>     // for cli_power_status_async_print
#include <icc_platform_defines.h> // for D2D_MBOX_FIFO_DEPTH, HSP_...
#include <idsw.h>                 // SW platform id
#include <idsw_kng.h>
#include <power_dfwk.h>      // for power_service_cli_request_t, (anony...
#include <power_runconfig.h> // for POWER_IF_CMD_UNKNOWN, power_if_cmd_t
#include <stdint.h>          // for uint8_t
#include <stdio.h>           // for NULL
#include <stdlib.h>
#include <string.h>
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/
#define BW_REDUCTION_MAX_PERCENTAGE (90)

/*-- Declarations (Statics and globals) --*/
static cli_power_cmd_context_t power_cli_cmd_context;
static fpfw_icc_base_ctx_t* icc_base_rmss_d2d_mbx_ctx = NULL;
static fpfw_icc_base_recv_req_t d2d_recv_params = {0}; //! Generic recv
static remote_pwrcli_request_t d2d_cli_recv_msg = {.icc_cmd_code = RMSS_D2D_MAILBOX_PWR_CLI_REQ};
/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
static FPFW_CLI_STATUS cli_power_config_command(int argc, const char** argv);
static FPFW_CLI_STATUS cli_power_set_command(int argc, const char** argv);
static FPFW_CLI_STATUS cli_power_status_command(int argc, const char** argv);
static FPFW_CLI_STATUS cli_power_log_command(int argc, const char** argv);
static FPFW_CLI_STATUS cli_power_accel_command(int argc, const char** argv);

static FPFW_CLI_STATUS cli_power_set_cmd_param_conversion(int argc, const char** argv, _pwrset_subcommand_args* p_pwrset_sub_command_args);
static uint8_t cli_power_cmd_arg_count(int subcommand_id);
static fpfw_status_t pwr_cli_d2d_mbox_recv_subscribe(void);
static void pwr_cli_d2d_recv_cb(void* context, size_t output_size_bytes, fpfw_status_t status);
/* TODO: Current implementation is for single die only. How do we pass on a request to set/retrieve values for the other die?
Option 1: Add another set of commands for the other die.
Option2: Add a field for every command to indicate redirection to the other die.

Option 2 is preferred as it will allow for a single set of commands to be used for both dies. TBD.
*/

// clang-format off
static FPFW_CLI_COMMAND cli_power_commands[] = {
    {NULL_LIST_ENTRY, "pwr", "cfg",    cli_power_config_command, "Display module config (knobs/fuses)",  "Usage: pwr cfg <sub_command>"   },
    {NULL_LIST_ENTRY, "pwr", "set",    cli_power_set_command,    "Set specific internal values",         "Usage: pwr set <sub_command>"   },
    {NULL_LIST_ENTRY, "pwr", "status", cli_power_status_command, "Display module status",                "Usage: pwr status <sub_command>"},
    {NULL_LIST_ENTRY, "pwr", "log",    cli_power_log_command,    "Access power log",                     "Usage: pwr log <sub_command>"   },
    {NULL_LIST_ENTRY, "pwr", "accel",  cli_power_accel_command,  "Accelerator power management",         "Usage: pwr accel <sub_command>" },
    };
//clang-format on

/*-------------- Functions ---------------*/
static power_if_cmd_t cli_power_get_cmd_id(e_cli_power_command_id_t command, char* subcommand)
{
    switch (command) {
        case CLI_COMMANDS_POWER_CONFIG:
            return cli_power_config_get_cmd_id(subcommand);
        case CLI_COMMANDS_POWER_SET:
            return cli_power_set_get_cmd_id(subcommand);
        case CLI_COMMANDS_POWER_STATUS:
            return cli_power_status_get_cmd_id(subcommand);
        case CLI_COMMANDS_POWER_LOG :
            return cli_power_log_get_cmd_id(subcommand);
        case CLI_COMMANDS_POWER_ACCEL:
            return cli_power_accel_cmd_id(subcommand);
        default:
            return POWER_IF_CMD_UNKNOWN;
    }
}

static PLACED_CODE FPFW_CLI_STATUS dispatch_power_cli_async_request(uint8_t die, e_cli_power_command_id_t command, char* subcommand, _pwrset_subcommand_args* p_set_data, DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine )
{

        power_cli_cmd_context.request.die = die;

        power_cli_cmd_context.request.header.RequestType = command;
        power_cli_cmd_context.request.sub_command = subcommand;

        power_cli_cmd_context.request.fetch_data.p_requested_data = NULL;
        if (p_set_data) {
            power_cli_cmd_context.request.pwrset_sub_command_args = *p_set_data;
        }

        power_cli_cmd_context.request.power_ext_if_cmd_id = cli_power_get_cmd_id(command, subcommand);

        if (power_cli_cmd_context.request.power_ext_if_cmd_id == POWER_IF_CMD_UNKNOWN)
        {
            FpFwCliPrint("Sub-command unsupported!!\n");
            return CLI_ERROR;
        }

        DfwkAsyncRequestSetCompletionRoutine(&power_cli_cmd_context.request.header, CompletionRoutine, NULL);
        DfwkInterfaceSendAsync(&power_cli_cmd_context.p_interface->header, &power_cli_cmd_context.request.header);

        return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS cli_power_config_command(int argc, const char** argv)
{
    if (argc < 2) {
        FpFwCliPrint("Usage: pwr cfg <sub_cmd>\n");
        return CLI_ERROR;
    }

    if ((strcmp(argv[1], "??") == 0) || (strcmp(argv[1], "help") == 0) || (strcmp(argv[1], "-h") == 0))
    {
        FpFwCliPrint("\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg ??", "- help menu\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg fuse", "All Power fuse configs\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg dts", "DTS Coefficients\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg memasst", "Memasst values\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg intervals", "Config loop intervals\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg limits", "Config control loop limits\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg pid", "Control loop PID config\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg pvt", "PVT thresholds\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg ctrlloop", "Control loop configs\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg srvmode", "Survivability Mode\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg tel", "Telemetry config\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg throttle", "Throttle config\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg static", "Static rails\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg allowed_min_plimit", "Allowed plimit min\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg allowed_max_plimit", "Allowed plimit max\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg fgpll", "FGPLL config\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg knobs", "All Power config knobs\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg vf", "VF curveset from fuses\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg vft", "Derived VFT from fuses, memasst\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg vftpre", "Pre-calculated current\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg vcpucalc", "VCPU calc inputs \n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg itd", "ITD config\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg avs_ds", "AVS DS config\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg lkg_temp_scaler", "Leakage temp scaler config\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg force_vrs", "Force VRS config\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg adclk_offset", "ADCLK offset config\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg tdp", "TDP fuse config\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg ldo2volt", "LDO to voltage slope fuse config\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg vcpuinterp", "VCPU interpolation fuse config\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg vsys_override", "VSYS override config knobs\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg vsys_vid", "VSYS VID fuse value\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg cppc_lowest_nonlin", "CPPC lowest non-linear perf config\n");
        FpFwCliPrint("\n");

        return CLI_SUCCESS;
    }

    /* Die - 0 (Default), Cmd ID - CLI_COMMANDS_POWER_CONFIG, subcommand - argv[1], setval - None (Unused), callback - cli_power_config_async_print */
    return dispatch_power_cli_async_request(0, CLI_COMMANDS_POWER_CONFIG, (char*)argv[1], NULL, cli_power_config_async_print);
}

static PLACED_CODE uint8_t cli_power_cmd_arg_count(int subcommand_id)
{
    uint8_t expected_argc = 0;

    switch (subcommand_id)
    {
        case POWER_IF_CMD_SET_CAP:
        case POWER_IF_CMD_SET_RACK_LIMIT:
        case POWER_IF_CMD_SET_MINUPDATE:
        case POWER_IF_CMD_SET_NOMINAL:
        case POWER_IF_CMD_LOG_MASK:
        case POWER_IF_CMD_LOG_DDR:
        case POWER_IF_CMD_SET_FORCE_PMIN:
        case POWER_IF_CMD_SET_SOC_HOT:
        case POWER_IF_CMD_SET_MEM_HOT:
        case POWER_IF_CMD_SET_THERM_TRIP:
        case POWER_IF_CMD_STATUS_DROOPCOUNT:
        case POWER_IF_CMD_STATUS_DVFS_THROT_SR:
            expected_argc = 3;
            break;

        case POWER_IF_CMD_SET_PLIMIT:
        case POWER_IF_CMD_SET_FORCED:
        case POWER_IF_CMD_SET_LOOP_DISABLES:
            expected_argc = 4;
            break;

        case POWER_IF_CMD_SET_DESIRED_PSTATE:
            expected_argc = 5;
            break;

        case POWER_IF_CMD_SET_PSTATE_FREQ:
        case POWER_IF_CMD_SET_CURR_THROTTLE:
            expected_argc = 6;
            break;

        case POWER_IF_CMD_SET_ALARM_THRESHOLD:
            expected_argc = 9;
            break;
            
        case POWER_IF_CMD_ACCEL_BW_REDUCE:
            expected_argc = 5;
        default:
            break;
    }

   return expected_argc;
}

static PLACED_CODE FPFW_CLI_STATUS cli_power_set_cmd_param_conversion(int argc, const char** argv, _pwrset_subcommand_args* p_pwrset_sub_command_args)
{
    char* pwr_strings[] = {
        "", // intentional empty string to align with enum
        "", // intentional empty string to align with enum
        "", // intentional empty string to align with enum
        "", // intentional empty string to align with enum
        "- set rack power cap (W)\n",
        "- sets OS desired pstate register\n",
        "- sets plimit\n",
        "- sets loop disable bits (1-ctrl, 2-vrtelem, 4-pvttelem)\n",
        "- sets rack limit gpio for simulated implementations\n",
        "- sets min. plimit update per loop iteration, 0 disables\n",
        "- sets nominal pstate used in loop (does not affect DVFS/ACPI)\n",
        "- sets forced pstate and ldodacin\n",
        "- sets pstate frequency control\n",
        "- sets alarm & hist threshold for Temp/VR hot throttle\n",
        "- sets force pmin\n",
        "- assert SOC_HOT\n",
        "- assert MEM_HOT\n",
        "- assert THERM_TRIP\n",
        "- sets current thresholds for throttling\n"
    };

    if (argc == 2)
    {
        if ((strcmp(argv[1], "??") == 0) || (strcmp(argv[1], "help") == 0) || (strcmp(argv[1], "-h") == 0))
        {
            FpFwCliPrint("\n");
            FpFwCliPrint("%-72s%s", "Usage: pwr set ??", "- help menu\n");
            FpFwCliPrint("%-72s%s", "Usage: pwr set cap <value>", pwr_strings[POWER_IF_CMD_SET_CAP]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set desired <core/all> <desired_0-31> <throttle_pri_0-15>", pwr_strings[POWER_IF_CMD_SET_DESIRED_PSTATE]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set plimit <core/all> <plimit_0-31>", pwr_strings[POWER_IF_CMD_SET_PLIMIT]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set loopdis <disable bits> <single/dual>", pwr_strings[POWER_IF_CMD_SET_LOOP_DISABLES]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set racklimit <0/1>", pwr_strings[POWER_IF_CMD_SET_RACK_LIMIT]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set minupdate <0-8>", pwr_strings[POWER_IF_CMD_SET_MINUPDATE]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set nominal <1-31>", pwr_strings[POWER_IF_CMD_SET_NOMINAL]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set forced <0-31> <ldodacin>", pwr_strings[POWER_IF_CMD_SET_FORCED]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set pstate_freq <pstate 0-31> <freq_ctrl> <fb_div> <frac_div>", pwr_strings[POWER_IF_CMD_SET_PSTATE_FREQ]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set alarm <core/all> <pex_group> <alarm type> <alarm_id> <alarm_threshold> <hist_threshold> <dual_die>", pwr_strings[POWER_IF_CMD_SET_ALARM_THRESHOLD]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set force_pmin <0=clear, 1=set lockup_ue_rr, 2=fw_pmin_ctrl>", pwr_strings[POWER_IF_CMD_SET_FORCE_PMIN]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set soc_hot <0/1>", pwr_strings[POWER_IF_CMD_SET_SOC_HOT]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set mem_hot <0/1>", pwr_strings[POWER_IF_CMD_SET_MEM_HOT]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set therm_trip <0/1>", pwr_strings[POWER_IF_CMD_SET_THERM_TRIP]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set curr_throttle <core/all> <thresh1(hex)> <thresh2(hex)> <thresh3(hex)>", pwr_strings[POWER_IF_CMD_SET_CURR_THROTTLE]);
            FpFwCliPrint("\n");

            return CLI_SUCCESS;
        }
    }

    unsigned char all = 0;
    unsigned char core = 0;
    uint8_t desired = 0;
    uint8_t throttle = 0;

    int subcommand_id = cli_power_set_get_cmd_id(argv[1]);

    switch (subcommand_id)
    {
        case POWER_IF_CMD_SET_CAP: {
            if(argc != cli_power_cmd_arg_count(subcommand_id))
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr set cap <value>", pwr_strings[POWER_IF_CMD_SET_CAP]);

                return CLI_ERROR;
            }

            p_pwrset_sub_command_args->cap_val = (uint16_t)strtoul(argv[2],NULL,10);
            break;
        }

        case POWER_IF_CMD_SET_DESIRED_PSTATE: {
            if(argc != cli_power_cmd_arg_count(subcommand_id))
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr set desired <core/all> <desired_0-31> <throttle_pri_0-15>", pwr_strings[POWER_IF_CMD_SET_DESIRED_PSTATE]);
                return CLI_ERROR;
            }

            all     = (strcmp(argv[2], "all") == 0);
            core    = all ? 0 : (unsigned)strtoul(argv[2], 0, 0);
            // desired is a 8 bit CPPC value, convert to that from the pstate/plimit
            // value of 0-31

            desired = (uint8_t)(strtoul(argv[3], 0, 0) & 0xFF);

            //! throttle priority range is 0-15
            throttle = (uint8_t)(strtoul(argv[4], 0, 0) & 0xF);

            p_pwrset_sub_command_args->desiredparams.all = all;
            p_pwrset_sub_command_args->desiredparams.core = core;
            p_pwrset_sub_command_args->desiredparams.state = desired;
            p_pwrset_sub_command_args->desiredparams.throttle = throttle;
            break;
        }

        case POWER_IF_CMD_SET_PLIMIT: {
            if(argc != cli_power_cmd_arg_count(subcommand_id))
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr set plimit <core/all> <plimit_0-31>", pwr_strings[POWER_IF_CMD_SET_PLIMIT]);
                return CLI_ERROR;
            }

            all = (unsigned char)(strcmp(argv[2], "all") == 0);
            core = (all  ? 0 : (unsigned)strtoul(argv[2], 0, 68));
            desired = (uint8_t)(strtoul(argv[3], 0, 0) & 0x1F);
            p_pwrset_sub_command_args->plimitparams.all = all;
            p_pwrset_sub_command_args->plimitparams.core = core;
            p_pwrset_sub_command_args->plimitparams.state = desired;
            break;
        }

        case POWER_IF_CMD_SET_LOOP_DISABLES: {
            if(argc == cli_power_cmd_arg_count(subcommand_id))
            {
                p_pwrset_sub_command_args->loopdis_params.loopdis_bits = (uint16_t)strtoul(argv[2],0,0);
                //! check if the argv[3] is single or dual
                if (argc == 4 && (strcmp(argv[3], "dual") == 0 || strcmp(argv[3], "single") == 0))
                {
                    p_pwrset_sub_command_args->loopdis_params.mode = (strcmp(argv[3], "dual") == 0) ? LOOP_DISABLE_MODE_DUAL_DIE : LOOP_DISABLE_MODE_SINGLE_DIE;
                }
                else
                {
                    FpFwCliPrint("Unknown arg 4, using default: pwr set loopdis %d single", p_pwrset_sub_command_args->loopdis_params.loopdis_bits);
                    p_pwrset_sub_command_args->loopdis_params.mode = LOOP_DISABLE_MODE_SINGLE_DIE; // default to single die
                }
            }
            else if (argc == cli_power_cmd_arg_count(subcommand_id) - 1)
            {
                // If the mode is not specified, default to single die
                p_pwrset_sub_command_args->loopdis_params.loopdis_bits = (uint16_t)strtoul(argv[2],0,0);
                p_pwrset_sub_command_args->loopdis_params.mode = LOOP_DISABLE_MODE_SINGLE_DIE;
            }
            else
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr set loopdis <disable bits> <single/dual>", pwr_strings[POWER_IF_CMD_SET_LOOP_DISABLES]);
                return CLI_ERROR;
            }
            FpFwCliPrint("[PWR CLI] loopdis_bits: 0x%x, mode: 0x%x\n",
                    p_pwrset_sub_command_args->loopdis_params.loopdis_bits,
                    p_pwrset_sub_command_args->loopdis_params.mode);
            break;
        }

        case POWER_IF_CMD_SET_RACK_LIMIT: {
            if(argc != cli_power_cmd_arg_count(subcommand_id))
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr set racklimit <0/1>", pwr_strings[POWER_IF_CMD_SET_RACK_LIMIT]);
                return CLI_ERROR;
            }

            p_pwrset_sub_command_args->racklimit = (uint16_t)strtoul(argv[2],NULL,0);

            break;
        }

        case POWER_IF_CMD_SET_MINUPDATE: {
            if(argc != cli_power_cmd_arg_count(subcommand_id))
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr set minupdate <0-8>", pwr_strings[POWER_IF_CMD_SET_MINUPDATE]);
                return CLI_ERROR;
            }

            p_pwrset_sub_command_args->minupdate_val = (uint16_t)strtoul(argv[2],NULL,10);
            break;
        }

        case POWER_IF_CMD_SET_NOMINAL: {
            if(argc != cli_power_cmd_arg_count(subcommand_id))
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr set nominal <1-31>", pwr_strings[POWER_IF_CMD_SET_NOMINAL]);
                return CLI_ERROR;
            }
            p_pwrset_sub_command_args->nominalparams.current_val = (uint16_t)strtoul(argv[2],NULL,0);
            break;
        }

        case POWER_IF_CMD_SET_FORCED: {
            if(argc != cli_power_cmd_arg_count(subcommand_id))
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr set forced <pstate 0-30> <ldodacin>", "sets forced Pstate and ldodacin " );
                return CLI_ERROR;
            }

            uint8_t forced_pstate;
            uint16_t forced_ldodacin;

            forced_pstate = (uint8_t)strtoul(argv[2],0,0);
            forced_ldodacin = (uint16_t)strtoul(argv[3],0,0);

            if(forced_pstate > 30) // only allow setting to P30
            {
                FpFwCliPrint("Invalid pstate value\n");
                return CLI_ERROR;
            }

            p_pwrset_sub_command_args->forcedparams.pstate = forced_pstate;
            p_pwrset_sub_command_args->forcedparams.ldodacin = forced_ldodacin;
            FpFwCliPrint("pwr set forced: pstate: %d, ldodacin - 0x%04x \n", p_pwrset_sub_command_args->forcedparams.pstate, p_pwrset_sub_command_args->forcedparams.ldodacin);
            break;
        }

        case POWER_IF_CMD_SET_PSTATE_FREQ: {
            if(argc != cli_power_cmd_arg_count(subcommand_id))
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr set pstate_freq <pstate 0-31> <freq_ctrl> <fb_div> <frac_div>", "- sets Pstate freq. " );
                return CLI_ERROR;
            }

            uint8_t cli_pstate = 0;
            uint32_t cli_freq_ctrl = 0;
            uint16_t cli_fb_div = 0;
            uint32_t cli_frac_div = 0;

            cli_pstate = (uint8_t)strtoul(argv[2],0,0);
            cli_freq_ctrl = (uint32_t)strtoul(argv[3],0,16);
            cli_fb_div = (uint16_t)strtoul(argv[4],0,16);
            cli_frac_div = (uint32_t)strtoul(argv[5],0,16);

            if(cli_pstate > MAX_PLIMIT)
            {
                FpFwCliPrint("Invalid pstate value\n");
                return CLI_ERROR;
            }

            p_pwrset_sub_command_args->pstatefreq.pstate = cli_pstate;
            p_pwrset_sub_command_args->pstatefreq.freq_ctrl = cli_freq_ctrl;
            p_pwrset_sub_command_args->pstatefreq.fb_div = cli_fb_div;
            p_pwrset_sub_command_args->pstatefreq.frac_div = cli_frac_div;

            FpFwCliPrint("pwr set pstate freq: pstate = %d, freq_ctrl = 0x%lx, fb_div = 0x%lx, frac_div = 0x%lx\n",
                p_pwrset_sub_command_args->pstatefreq.pstate, p_pwrset_sub_command_args->pstatefreq.freq_ctrl,
                p_pwrset_sub_command_args->pstatefreq.fb_div, p_pwrset_sub_command_args->pstatefreq.frac_div);
            break;
        }
        case POWER_IF_CMD_SET_ALARM_THRESHOLD: {
            if(argc != cli_power_cmd_arg_count(subcommand_id))
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr set alarm <core/all> <pex_group> <alarm type> <alarm_id> <alarm_threshold(hex)> <hist_threshold(hex)> <dual_die>", pwr_strings[POWER_IF_CMD_SET_ALARM_THRESHOLD]);
                return CLI_ERROR;
            }
            all = (strcmp(argv[2], "all") == 0);
            core = all ? 0 : (unsigned)strtoul(argv[2], 0, 0);
            uint8_t pex_group = (uint8_t)strtoul(argv[3], 0, 0);
            const char* alarm_type = argv[4];
            uint8_t alarm_id = (uint8_t)strtoul(argv[5], 0, 0);
            uint16_t alarm_threshold = (uint16_t)strtoul(argv[6], 0, 16);
            uint16_t hist_threshold = (uint16_t)strtoul(argv[7], 0, 16);
            bool dual_die = (strcmp(argv[8], "1") == 0);
            if (alarm_id >= POWER_IF_ALARM_ID_MAX)
            {
                FpFwCliPrint("Invalid alarm id %d, valid range is 0-%d\n", alarm_id, POWER_IF_ALARM_ID_MAX - 1);
                return CLI_ERROR;
            }
            if (core >= NUM_AP_CORES_PER_DIE) {
                FpFwCliPrint("Invalid core id %d, valid range is 0-%d\n", core, NUM_AP_CORES_PER_DIE - 1);
                return CLI_ERROR;
            }
            
            if (strcmp(alarm_type, "a") != 0 && strcmp(alarm_type, "b") != 0) {
                FpFwCliPrint("Invalid alarm type '%s'. Valid types are 'a' or 'b'.\n", alarm_type);
                return CLI_ERROR;
            }
            if (pex_group != 0 && pex_group != 1) {
                FpFwCliPrint("Invalid pex_group %d, valid values are 0 or 1\n", pex_group);
                return CLI_ERROR;
            }
            if (dual_die) {
                FpFwCliPrint("Dual die mode is not supported yet. Proceeding with single die mode.\n");
                dual_die = false;
            }
            
            FpFwCliPrint(
                "User Requested Alarm Config:\n"
                "  Core ID: %d\n"
                "  All Cores: %s\n"
                "  PEX Group: %d\n"
                "  Alarm Type: %s\n"
                "  Alarm ID: %d\n"
                "  Alarm Threshold: %x\n"
                "  Hysteresis Threshold: %x\n"
                "  Die Mode: %s\n",
                core,
                all ? "Yes" : "No",
                pex_group,
                alarm_type,
                alarm_id,
                alarm_threshold,
                hist_threshold,
                dual_die ? "Dual Die" : "Single Die"
            );
            
            p_pwrset_sub_command_args->alarm_cfg.all = all;
            p_pwrset_sub_command_args->alarm_cfg.core = core;
            p_pwrset_sub_command_args->alarm_cfg.ab_selector = alarm_type[0];
            p_pwrset_sub_command_args->alarm_cfg.alarm_id = alarm_id;
            p_pwrset_sub_command_args->alarm_cfg.alarm_threshold = alarm_threshold;
            p_pwrset_sub_command_args->alarm_cfg.hist_threshold = hist_threshold;
            p_pwrset_sub_command_args->alarm_cfg.dual_die = dual_die;
            p_pwrset_sub_command_args->alarm_cfg.pex_group = pex_group;
            break;
        }
        case POWER_IF_CMD_SET_FORCE_PMIN: {
            if(argc != cli_power_cmd_arg_count(subcommand_id))
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr set force_pmin <0=clear, 1=set lockup_ue_rr, 2=fw_pmin_ctrl>", pwr_strings[POWER_IF_CMD_SET_FORCE_PMIN]);
                return CLI_ERROR;
            }
            uint8_t pmin_type = (uint8_t)strtoul(argv[2], NULL, 0);
            if (pmin_type > 2) {
                FpFwCliPrint("Invalid pmin type %d, valid range is 0-2\n", pmin_type);
                return CLI_ERROR;
            }
            p_pwrset_sub_command_args->pmin_type = pmin_type;
            break;
        }
        case POWER_IF_CMD_SET_SOC_HOT: {
            if(argc != cli_power_cmd_arg_count(subcommand_id))
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr set soc_hot <0/1>", pwr_strings[POWER_IF_CMD_SET_SOC_HOT]);
                return CLI_ERROR;
            }
            p_pwrset_sub_command_args->io_thermal = (uint8_t)strtoul(argv[2], NULL, 0);
            break;
        }
        case POWER_IF_CMD_SET_MEM_HOT: {
            if(argc != cli_power_cmd_arg_count(subcommand_id))
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr set mem_hot <0/1>", pwr_strings[POWER_IF_CMD_SET_MEM_HOT]);
                return CLI_ERROR;
            }
            p_pwrset_sub_command_args->io_thermal = (uint8_t)strtoul(argv[2], NULL, 0);
            break;
        }
        case POWER_IF_CMD_SET_THERM_TRIP: {
            if(argc != cli_power_cmd_arg_count(subcommand_id))
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr set therm_trip <0/1>", pwr_strings[POWER_IF_CMD_SET_THERM_TRIP]);
                return CLI_ERROR;
            }
            p_pwrset_sub_command_args->io_thermal = (uint8_t)strtoul(argv[2], NULL, 0);
            break;
        }
        case POWER_IF_CMD_SET_CURR_THROTTLE: {
            if(argc != cli_power_cmd_arg_count(subcommand_id))
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr set curr_throttle <core/all> <curr_threshold_1(hex)> <curr_threshold_2(hex)> <curr_threshold_3(hex)>", pwr_strings[POWER_IF_CMD_SET_CURR_THROTTLE]);
                return CLI_ERROR;
            }
            all = (strcmp(argv[2], "all") == 0);
            core = all ? 0 : (unsigned)strtoul(argv[2], 0, 0);
            if (core >= NUM_AP_CORES_PER_DIE) {
                FpFwCliPrint("Invalid core id %d, valid range is 0-%d\n", core, NUM_AP_CORES_PER_DIE - 1);
                return CLI_ERROR;
            }
            
            p_pwrset_sub_command_args->currthresh_params.all = all;
            p_pwrset_sub_command_args->currthresh_params.core = core;
            p_pwrset_sub_command_args->currthresh_params.curr_threshold_1 = (uint16_t)strtoul(argv[3], 0, 16);
            p_pwrset_sub_command_args->currthresh_params.curr_threshold_2 = (uint16_t)strtoul(argv[4], 0, 16);
            p_pwrset_sub_command_args->currthresh_params.curr_threshold_3 = (uint16_t)strtoul(argv[5], 0, 16);
            break;
        }
        default:
            break;

    }

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS cli_power_log_cmd_param_conversion(int argc, const char** argv, _pwrset_subcommand_args* p_pwrset_sub_command_args)
{
    if (argc == 2)
    {
        if ((strcmp(argv[1], "??") == 0) || (strcmp(argv[1], "help") == 0) || (strcmp(argv[1], "-h") == 0))
        {
            FpFwCliPrint("\n");
            FpFwCliPrint("%-72s%s", "Usage: pwr log ??", "- Help menu\n");
            FpFwCliPrint("%-72s%s", "Usage: pwr log list", "- Dump the DDR power log entries\n");
            FpFwCliPrint("%-72s%s", "Usage: pwr log ddr <1/0>", "- Enable/Disable power logging in DDR\n");
            FpFwCliPrint("%-72s%s", "Usage: pwr log mask <mask>", "- Set or display current power log mask\n");
            FpFwCliPrint("\n");

            return CLI_SUCCESS;
        }
    }
    int subcommand_id = cli_power_log_get_cmd_id(argv[1]);

    switch (subcommand_id)
    {
        case POWER_IF_CMD_LOG_DDR : {
            if(argc != cli_power_cmd_arg_count(subcommand_id))
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr log ddr <1 or 0>", "- sets power log DDR enable or disable\n");
                return CLI_ERROR;
            }
            p_pwrset_sub_command_args->minupdate_val = (uint16_t)strtoul(argv[2],0,0);
            break;
        }

        case POWER_IF_CMD_LOG_MASK : {
            if(argc == cli_power_cmd_arg_count(subcommand_id))
            {
                p_pwrset_sub_command_args->minupdate_val = (uint16_t)strtoul(argv[2],0,0);
            }
            else{
                FpFwCliPrint("%-72s%s", "Usage: pwr log mask <value>", "- Updates power log mask\n");
                FpFwCliPrint("%-72s%s", "Usage: pwr log mask", "- Display Current power log mask\n");
                p_pwrset_sub_command_args->minupdate_val = 0xFFFF;
            }
            break;
        }

        case POWER_IF_CMD_LOG_DUMP : {
            if(argc != 2)
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr log list", "- list power log entries\n");
                return CLI_ERROR;
            }
            break;
        }
    }
    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS cli_power_set_command(int argc, const char** argv)
{
    if (argc < 2) {
        FpFwCliPrint("Usage: pwr set <sub_command>\n");
        return CLI_ERROR;
    }
    _pwrset_subcommand_args pwrset_sub_command_args = {};

    if (cli_power_set_cmd_param_conversion(argc,argv, &pwrset_sub_command_args) == 0) {
        return dispatch_power_cli_async_request((uint8_t)idsw_get_die_id(), CLI_COMMANDS_POWER_SET, (char*)argv[1], &pwrset_sub_command_args, cli_power_set_async_print);
    }
    return CLI_ERROR;

}

static PLACED_CODE FPFW_CLI_STATUS cli_power_status_command(int argc, const char** argv)
{
    if (argc < 2) {
        FpFwCliPrint("Usage: pwr status <sub_cmd>\n");
        return CLI_ERROR;
    }

    if ((strcmp(argv[1], "??") == 0) || (strcmp(argv[1], "help") == 0) || (strcmp(argv[1], "-h") == 0))
    {
        FpFwCliPrint("\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status ??", "- help menu\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status cl", "Control loop info\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status vrtl", "VR telemetry loop info\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status pvttl", "PVT telemetry loop info\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status cap", "Power Cap Info\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status droopcount <core>", "Droop count for a specific core\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status maxtemp", "Max temperature info\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status power", "Power info\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status plimit", "Plimit info\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status primary_core", "Primary core info\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status selections <clear>", "Dump plimit selection counts and clear if desired\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status vcpu", "VCPU calc inputs\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status warmstart", "Warmstart info\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status force_pmin", "Force Pmin info\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status core", "Core info\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status dvfs_fsm", "DVFS FSM info\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status dvfs_plimit", "DVFS plimit info\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status dvfs_cppc", "DVFS CPPC info\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status core_prio", "Core priority info\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status prio_cnt", "Priority count info\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status vr_inst", "VR instance info\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status pstate2cppc", "Pstate to CPPC mapping info\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status dvfs_throt_sr <core>", "DVFS throttle status info for core\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status vmin <core>", "Vmin/LDO voltage at P31 (all cores if no core specified)\n");
        FpFwCliPrint("\n");

        return CLI_SUCCESS;
    }
    
    _pwrset_subcommand_args pwrset_sub_command_args = {};
    int subcommand_id = cli_power_status_get_cmd_id(argv[1]);

    switch (subcommand_id)
    {
        case POWER_IF_CMD_STATUS_DROOPCOUNT : {
            if(argc != cli_power_cmd_arg_count(subcommand_id))
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr status droopcount <core>", "- gets droop count for a specific core\n");
                return CLI_ERROR;
            }
            pwrset_sub_command_args.minupdate_val = (uint16_t)strtoul(argv[2],0,0);
            break;
        }
        case POWER_IF_CMD_STATUS_SELECTIONS : {
            if(argc == cli_power_cmd_arg_count(subcommand_id) && strcmp(argv[2], "clear") == 0)
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr status selections <clear>", "- Dump plimit selection counts and clear if desired\n");
                pwrset_sub_command_args.minupdate_val = 0x1;
            }
            else{
                FpFwCliPrint("%-72s%s", "Usage: pwr status selections", "- Dump plimit selection counts\n");
                pwrset_sub_command_args.minupdate_val = 0x0;
            }
            break;
        }
        case POWER_IF_CMD_STATUS_DVFS_THROT_SR : {
            if(argc != cli_power_cmd_arg_count(subcommand_id))
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr status dvfs_throt_sr <core>", "- get DVFS throttle status info for a specific core\n");
                return CLI_ERROR;
            }
            pwrset_sub_command_args.minupdate_val = (uint16_t)strtoul(argv[2],0,0);
            break;
        }
        case POWER_IF_CMD_STATUS_VMIN : {
            // Optional core argument: if argc==3, use specified core; if argc==2, use 0xFF to indicate all cores
            if(argc == 3)
            {
                pwrset_sub_command_args.minupdate_val = (uint16_t)strtoul(argv[2],0,0);
            }
            else
            {
                // Use 0xFF as sentinel to indicate "all cores"
                pwrset_sub_command_args.minupdate_val = 0xFF;
            }
            break;
        }

    }
    /* Die - 0 (Default), Cmd ID - CLI_COMMANDS_POWER_CONFIG, subcommand - argv[1], setval - None (Unused), callback - cli_power_config_async_print */
    return dispatch_power_cli_async_request((uint8_t)idsw_get_die_id(), CLI_COMMANDS_POWER_STATUS, (char*)argv[1], &pwrset_sub_command_args, cli_power_status_async_print);
}

static PLACED_CODE FPFW_CLI_STATUS cli_power_log_command(int argc, const char** argv)
{
    if (argc < 2) {
        FpFwCliPrint("Usage: pwr log <sub_cmd>\nTo see available subcmds-> pwr log ??\n");
        return CLI_ERROR;
    }

    _pwrset_subcommand_args pwrset_sub_command_args = {};
    if (cli_power_log_cmd_param_conversion(argc,argv, &pwrset_sub_command_args) == 0) {
        return dispatch_power_cli_async_request((uint8_t)idsw_get_die_id(), CLI_COMMANDS_POWER_LOG, (char*)argv[1], &pwrset_sub_command_args, cli_power_log_async_print);
    }
    return CLI_ERROR;
}

static PLACED_CODE void pwr_cli_d2d_recv_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_DBGPRINT_INFO("[PWR CLI] D2D recv callback invoked with status: %d, output size: %zu\n", status, output_size_bytes);
    if (context == NULL) {
        FPFW_DBGPRINT_ERROR("[PWR CLI] D2D recv callback context is NULL\n");
        return; //! crash?
    }

    if ((status != FPFW_STATUS_SUCCESS) || (output_size_bytes != sizeof(rmss_d2d_mailbox_msg)))
    {
        FPFW_DBGPRINT_ERROR("[PWR CLI] D2D recv failed with status: %d, output size: %zu\n", status, output_size_bytes);
        return; //! crash?
    }

    fpfw_icc_base_recv_req_t* req_params = (fpfw_icc_base_recv_req_t*)context;
    p_remote_pwrcli_request_t d2d_recvd_msg = (p_remote_pwrcli_request_t)(req_params->payload_buffer);
    rmss_d2d_mailbox_msg_header* recv_header = (rmss_d2d_mailbox_msg_header*)&d2d_recvd_msg->icc_cmd_code;
    if (recv_header->cmd != RMSS_D2D_MAILBOX_PWR_CLI_REQ) {
        FPFW_DBGPRINT_ERROR("[PWR CLI] Unknown cmd code received: 0x%08x\n", recv_header->cmd);
        return; //crash?
    }
    //! get subcommand from the received payload
    switch (d2d_recvd_msg->power_ext_if_cmd_id) {
        case POWER_IF_CMD_SET_LOOP_DISABLES:
            // Handle loop disable command
            FPFW_DBGPRINT_INFO("[PWR CLI] Received D2D loop disable command from Remote, Value: %d\n", d2d_recvd_msg->pwrset_sub_command_args.loopdis_params.loopdis_bits);
            //! set loop disable for the current die
            // Simulate CLI input: "pwr set loopdis single"
            char loopdis_bits_str[16];
            snprintf(loopdis_bits_str, sizeof(loopdis_bits_str), "%d", d2d_recvd_msg->pwrset_sub_command_args.loopdis_params.loopdis_bits);
            const char* argv[] = {"set", "loopdis", loopdis_bits_str, "single" };
            FPFW_CLI_STATUS cli_stat = cli_power_set_command((int)cli_power_cmd_arg_count(POWER_IF_CMD_SET_LOOP_DISABLES), argv);
            if (cli_stat != CLI_SUCCESS) {
                FPFW_DBGPRINT_ERROR("[PWR CLI] Failed to set loop disable command\n");
            }
            break;
        // Add more cases as needed for other command codes
        default:
            FPFW_DBGPRINT_ERROR("[PWR CLI] Unsupported Remote Cmd: 0x%08x\n", d2d_recvd_msg->power_ext_if_cmd_id);
            break;
    }

    //! resubscribe to receive the next message
    if (FPFW_STATUS_SUCCESS != pwr_cli_d2d_mbox_recv_subscribe())
    {
        FPFW_DBGPRINT_ERROR("[PWR CLI] D2D mailbox subscribe failed with status: %d\n", status);
        return;
    }
}

static PLACED_CODE fpfw_status_t pwr_cli_d2d_mbox_recv_subscribe(void)
{
    //! Prepare recv request
    memset(&d2d_cli_recv_msg, 0, sizeof(d2d_cli_recv_msg));
    d2d_recv_params.payload_buffer = &d2d_cli_recv_msg;
    d2d_recv_params.buffer_size = sizeof(d2d_cli_recv_msg);
    d2d_recv_params.recv_cmd_code = RMSS_D2D_MAILBOX_PWR_CLI_REQ;
    d2d_recv_params.cb = pwr_cli_d2d_recv_cb;
    d2d_recv_params.cb_ctx = &d2d_recv_params;

    //! Register for recv thru icc base
    fpfw_status_t status = fpfw_icc_base_recv(icc_base_rmss_d2d_mbx_ctx, &d2d_recv_params);
    return status;
}

static PLACED_CODE FPFW_CLI_STATUS cli_power_accel_command(int argc, const char** argv)
{
    _pwrset_subcommand_args pwrset_sub_command_args = {};

    if (argc < 2) {
        FpFwCliPrint("Usage: pwr accel <sub_cmd>\nTo see available sub_cmd -> pwr accel -h\n");
        return CLI_ERROR;
    }

    if ((strcmp(argv[1], "??") == 0) || (strcmp(argv[1], "help") == 0) || (strcmp(argv[1], "-h") == 0)) {
        FpFwCliPrint("\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr accel ??", "- help menu\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr accel bw_reduce <accelerator id> <bandwidth reduction percentage> <dual bus>", "bandwidth reduction\n"
                                "                           accelerator id                 - 0 for SDM, 1 - for CDED\n"
                                "                           bandwidth reduction percentage - between 0 and 90\n"
                                "                           dual bus                       - 1 for SDM standalone, 0 for SDM CDED\n");
        FpFwCliPrint("\n");
        return CLI_SUCCESS;
    }

    switch (cli_power_accel_cmd_id(argv[1])) {
        case POWER_IF_CMD_ACCEL_BW_REDUCE : {
            if (argc != cli_power_cmd_arg_count(POWER_IF_CMD_ACCEL_BW_REDUCE)) {
                FpFwCliPrint("%-72s%s", "Usage: pwr accel bw_reduce <accelerator id> <bandwidth reduction percentage> <dual bus>", "bandwidth reduction\n"
                                        "                           accelerator id                 - 0 for SDM, 1 - for CDED\n"
                                        "                           bandwidth reduction percentage - between 0 and 90\n"
                                        "                           dual bus                       - 1 for SDM standalone, 0 for SDM CDED\n");
                return CLI_ERROR;
            }

            pwrset_sub_command_args.accelparams.accel_id = (uint8_t)strtoul(argv[2], NULL, 10);
            if (pwrset_sub_command_args.accelparams.accel_id > ACCEL_ID_CDED) {
                FPFW_DBGPRINT_ERROR("[PWR CLI] Invalid accel id\n");
                return CLI_ERROR;
            }

            pwrset_sub_command_args.accelparams.bw_reduction_perc = (uint8_t)strtoul(argv[3], NULL, 10);
            if (pwrset_sub_command_args.accelparams.bw_reduction_perc > BW_REDUCTION_MAX_PERCENTAGE) {
                FPFW_DBGPRINT_ERROR("[PWR CLI] Invalid bandwidth reduction percentage\n");
                return CLI_ERROR;
            }

            pwrset_sub_command_args.accelparams.dual_bus = ((uint8_t)strtoul(argv[4], NULL, 10) != 0);

            break;
        }

        default: {
            FPFW_DBGPRINT_ERROR("[PWR CLI] Unsupported power accel subcommand\n");
            return CLI_ERROR;
        }
    }

    return dispatch_power_cli_async_request((uint8_t)idsw_get_die_id(), CLI_COMMANDS_POWER_ACCEL, (char*)argv[1], &pwrset_sub_command_args, cli_power_accel_complete);
}

PLACED_CODE FPFW_CLI_STATUS cli_power_init(ppower_service_interface_t p_interface, fpfw_icc_base_ctx_t* p_icc_base_ctx)
{
    if (p_interface == NULL || p_icc_base_ctx == NULL)
    {
        return CLI_ERROR;
    }

    // Cache the power interface pointer
    power_cli_cmd_context.p_interface = p_interface;

    // Register List of CLI commands with CLI
    FpFwCliRegisterTable(cli_power_commands, FPFW_ARRAY_SIZE(cli_power_commands));

    // Open Driver Framework Interface for Power CLI, initialize async requests
    DfwkClientInterfaceOpen(&power_cli_cmd_context.p_interface->header);
    DfwkAsyncRequestInitialize(&power_cli_cmd_context.request.header, sizeof(power_cli_cmd_context.request.header));

    //! subscribe to recv over icc
    icc_base_rmss_d2d_mbx_ctx = p_icc_base_ctx;
    fpfw_status_t status = pwr_cli_d2d_mbox_recv_subscribe();
    if (status != FPFW_STATUS_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("[PWR CLI] D2D mailbox subscribe failed with status: %d\n", status);
        return CLI_ERROR;
    }
    FPFW_DBGPRINT_INFO("[PWR CLI] Initialized successfully\n");
    return CLI_SUCCESS;
}
