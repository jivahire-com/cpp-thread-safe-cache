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

#include <DfwkClient.h>          // for DfwkAsyncRequestInitialize, DfwkAs...
#include <FpFwUtils.h>           // for FPFW_ARRAY_SIZE
#include <cli_power.h>           // for cli_power_init
#include <cli_power_config.h>    // for cli_power_config_get_cmd_id, cli_po...
#include <cli_power_interface.h> // for cli_power_cmd_context_t
#include <cli_power_log.h>       // for cli_power_log_async_print
#include <cli_power_set.h>       // for cli_power_set_get_cmd_id, cli_power...
#include <cli_power_status.h>    // for cli_power_status_async_print
#include <idsw.h>                // SW platform id
#include <idsw_kng.h>
#include <power_dfwk.h>      // for power_service_cli_request_t, (anony...
#include <power_runconfig.h> // for POWER_IF_CMD_UNKNOWN, power_if_cmd_t
#include <stdint.h>          // for uint8_t
#include <stdio.h>           // for NULL
#include <stdlib.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-- Declarations (Statics and globals) --*/
static cli_power_cmd_context_t power_cli_cmd_context;

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
static FPFW_CLI_STATUS cli_power_config_command(int argc, const char** argv);
static FPFW_CLI_STATUS cli_power_set_command(int argc, const char** argv);
static FPFW_CLI_STATUS cli_power_status_command(int argc, const char** argv);
static FPFW_CLI_STATUS cli_power_log_command(int argc, const char** argv);

static FPFW_CLI_STATUS cli_power_set_cmd_param_conversion(int argc, const char** argv, _pwrset_subcommand_args* p_pwrset_sub_command_args);

static uint8_t cli_power_cmd_arg_count(int subcommand_id);

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
        default:
            return POWER_IF_CMD_UNKNOWN;
    }
}

static FPFW_CLI_STATUS dispatch_power_cli_async_request(uint8_t die, e_cli_power_command_id_t command, char* subcommand, _pwrset_subcommand_args* p_set_data, DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine )
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

static FPFW_CLI_STATUS cli_power_config_command(int argc, const char** argv)
{
    if (argc < 2) {
        FpFwCliPrint("Usage: pwr cfg <sub_cmd>\n");
        return CLI_ERROR;
    }

    if ((strcmp(argv[1], "??") == 0))
    {
        FpFwCliPrint("\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg ??", "- help menu\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg fuse", "Misc fuse configs\n");
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
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg knobs", "Power config knobs\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg vftpre", "Pre-calculated current\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr cfg vcpucalc", "VCPU calc inputs \n");
        FpFwCliPrint("\n");

        return CLI_SUCCESS;
    }

    /* Die - 0 (Default), Cmd ID - CLI_COMMANDS_POWER_CONFIG, subcommand - argv[1], setval - None (Unused), callback - cli_power_config_async_print */
    return dispatch_power_cli_async_request(0, CLI_COMMANDS_POWER_CONFIG, (char*)argv[1], NULL, cli_power_config_async_print);
}

static uint8_t cli_power_cmd_arg_count(int subcommand_id)
{
    uint8_t expected_argc = 0;

    switch (subcommand_id)
    {
        case POWER_IF_CMD_SET_CAP:
        case POWER_IF_CMD_SET_LOOP_DISABLES:
        case POWER_IF_CMD_SET_RACK_LIMIT:
        case POWER_IF_CMD_SET_MINUPDATE:
        case POWER_IF_CMD_SET_NOMINAL:
        case POWER_IF_CMD_LOG_MASK:
        case POWER_IF_CMD_LOG_DDR:
            expected_argc = 3;
            break;

        case POWER_IF_CMD_SET_PLIMIT:
        case POWER_IF_CMD_SET_FORCED:
            expected_argc = 4;
            break;

        case POWER_IF_CMD_SET_DESIRED_PSTATE:
            expected_argc = 5;
            break;

        case POWER_IF_CMD_SET_PSTATE_FREQ:
            expected_argc = 6;
            break;

        default:
            break;
    }

   return expected_argc;
}

static FPFW_CLI_STATUS cli_power_set_cmd_param_conversion(int argc, const char** argv, _pwrset_subcommand_args* p_pwrset_sub_command_args)
{
    char* pwr_strings[] = {
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
    };

    if (argc == 2)
    {
        if( (strcmp(argv[1], "??") == 0) )
        {
            FpFwCliPrint("\n");
            FpFwCliPrint("%-72s%s", "Usage: pwr set ??", "- help menu\n");
            FpFwCliPrint("%-72s%s", "Usage: pwr set cap <value>", pwr_strings[POWER_IF_CMD_SET_CAP]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set desired <core/all> <desired_0-31> <throttle_pri_0-7>", pwr_strings[POWER_IF_CMD_SET_DESIRED_PSTATE]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set plimit <core/all> <plimit_0-31>", pwr_strings[POWER_IF_CMD_SET_PLIMIT]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set loopdis <disable bits>", pwr_strings[POWER_IF_CMD_SET_LOOP_DISABLES]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set racklimit <0/1>", pwr_strings[POWER_IF_CMD_SET_RACK_LIMIT]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set minupdate <0-8>", pwr_strings[POWER_IF_CMD_SET_MINUPDATE]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set nominal <1-31>", pwr_strings[POWER_IF_CMD_SET_NOMINAL]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set forced <0-31> <ldodacin>", pwr_strings[POWER_IF_CMD_SET_FORCED]);
            FpFwCliPrint("%-72s%s", "Usage: pwr set pstate_freq <pstate 0-31> <freq_ctrl> <fb_div> <frac_div>", pwr_strings[POWER_IF_CMD_SET_PSTATE_FREQ]);
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
                FpFwCliPrint("%-72s%s", "Usage: pwr set desired <core/all> <desired_0-31> <throttle_pri_0-7>", pwr_strings[POWER_IF_CMD_SET_DESIRED_PSTATE]);
                return CLI_ERROR;
            }

            all     = (strcmp(argv[2], "all") == 0);
            core    = all ? 0 : (unsigned)strtoul(argv[2], 0, 0);
            // desired is a 8 bit CPPC value, convert to that from the pstate/plimit
            // value of 0-31

            desired = (uint8_t)(strtoul(argv[3], 0, 0) & 0xFF);

            // throttle is the throttle priority 0-7
            throttle = (uint8_t)(strtoul(argv[4], 0, 0) & 0x7);

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
            if(argc != cli_power_cmd_arg_count(subcommand_id))
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr set loopdis <disable bits>", pwr_strings[POWER_IF_CMD_SET_LOOP_DISABLES]);
                return CLI_ERROR;
            }

            p_pwrset_sub_command_args->loopdis_bits = (uint16_t)strtoul(argv[2],0,0);
            break;
        }

        case POWER_IF_CMD_SET_RACK_LIMIT: {
            if(argc != cli_power_cmd_arg_count(subcommand_id))
            {
                FpFwCliPrint("%-72s%s", "Usage: pwr set racklimit <0/1>", pwr_strings[POWER_IF_CMD_SET_RACK_LIMIT]);
                return CLI_ERROR;
            }

            p_pwrset_sub_command_args->racklimit = (uint16_t)strtoul(argv[2],NULL,0);

            if (power_hw_gpio_connected()) {
                FpFwCliPrint("\n  pwr set racklimit does not work on this platform (GPIO connected)\n");
                return CLI_SUCCESS;
            }
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

        default:
            break;

    }

    FpFwCliPrint("pwr_cli_comp\n");
    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS cli_power_log_cmd_param_conversion(int argc, const char** argv, _pwrset_subcommand_args* p_pwrset_sub_command_args)
{
    if (argc == 2)
    {
        if( (strcmp(argv[1], "??") == 0) )
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

static FPFW_CLI_STATUS cli_power_set_command(int argc, const char** argv)
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

static FPFW_CLI_STATUS cli_power_status_command(int argc, const char** argv)
{
    if (argc < 2) {
        FpFwCliPrint("Usage: pwr status <sub_cmd>\n");
        return CLI_ERROR;
    }

    if ((strcmp(argv[1], "??") == 0))
    {
        FpFwCliPrint("\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status ??", "- help menu\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status cl", "Control loop info\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status vrtl", "VR telemetry loop info\n");
        FpFwCliPrint("%-72s%s", "Usage: pwr status pvttl", "PVT telemetry loop info\n");
        FpFwCliPrint("\n");

        return CLI_SUCCESS;
    }
    
    /* Die - 0 (Default), Cmd ID - CLI_COMMANDS_POWER_CONFIG, subcommand - argv[1], setval - None (Unused), callback - cli_power_config_async_print */
    return dispatch_power_cli_async_request((uint8_t)idsw_get_die_id(), CLI_COMMANDS_POWER_STATUS, (char*)argv[1], NULL, cli_power_status_async_print);
}

static FPFW_CLI_STATUS cli_power_log_command(int argc, const char** argv)
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

FPFW_CLI_STATUS cli_power_init(ppower_service_interface_t p_interface)
{
    if (p_interface == NULL)
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

    return CLI_SUCCESS;
}
