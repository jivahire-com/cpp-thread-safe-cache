//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_power.c
 * Source file for power CLI definition
 */

/*------------- Includes -----------------*/
#include <DfwkClient.h>
#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT>
#include <FpFwUtils.h>
#include <cli_power.h>
#include <cli_power_common.h>
#include <cli_power_config.h>
#include <cli_power_interface.h>
#include <cli_power_log.h>
#include <cli_power_set.h>
#include <cli_power_status.h>
#include <power_dfwk.h>
#include <power_runconfig.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-- Declarations (Statics and globals) --*/
static cli_power_cmd_context_t power_cli_cmd_context;

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
static FPFW_CLI_STATUS cli_power_config_command(int argc, const char** argv);
static FPFW_CLI_STATUS cli_power_set_command(int argc, const char** argv);
static FPFW_CLI_STATUS cli_power_status_command(int argc, const char** argv);
static FPFW_CLI_STATUS cli_power_log_command(int argc, const char** argv);

/* TODO: Current implementation is for single die only. How do we pass on a request to set/retrieve values for the other die?
Option 1: Add another set of commands for the other die.
Option2: Add a field for every command to indicate redirection to the other die.

Option 2 is preferred as it will allow for a single set of commands to be used for both dies. TBD.
*/

// clang-format off
static FPFW_CLI_COMMAND cli_power_commands[] = {
    {NULL_LIST_ENTRY, "pwr", "cfg",    cli_power_config_command, "Pwr CLI cmd to display module config (knobs/fuses)",  "Usage: pwr cfg <sub_command>"   },
    {NULL_LIST_ENTRY, "pwr", "set",    cli_power_set_command,    "Pwr CLI cmd to set specific internal values",         "Usage: pwr set <sub_command>"   },
    {NULL_LIST_ENTRY, "pwr", "status", cli_power_status_command, "Pwr CLI cmd to display module status",                "Usage: pwr status <sub_command>"},
    {NULL_LIST_ENTRY, "pwr", "log",    cli_power_log_command,    "Pwr CLI cmd to access power log",                     "Usage: pwr log <sub_command>"   },
    };      
//clang-format on

/*-------------- Functions ---------------*/

static FPFW_CLI_STATUS dispatch_power_cli_async_request(e_cli_power_command_id_t command_id, char* subcommand, DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE CompletionRoutine )
{
    
        power_cli_cmd_context.request.header.RequestType = command_id;
        power_cli_cmd_context.request.sub_command = subcommand;
        power_cli_cmd_context.request.power_runconfig_element_id = cli_power_config_get_runconfig_element_id(subcommand);

        if (power_cli_cmd_context.request.power_runconfig_element_id == POWER_RUNCONFIG_UNKNOWN)
        {
            printf("Sub-command unsupported!!\n");
            return CLI_ERROR;
        }

        DfwkAsyncRequestSetCompletionRoutine(&power_cli_cmd_context.request.header, CompletionRoutine, NULL); 
        DfwkInterfaceSendAsync(&power_cli_cmd_context.p_interface->header, &power_cli_cmd_context.request.header);

        return CLI_SUCCESS;
}

static FPFW_CLI_STATUS cli_power_config_command(int argc, const char** argv)
{
    if (argc < 2) {
        printf("Usage: pwr cfg <sub_cmd>\n");
        return CLI_ERROR;
    }

    return dispatch_power_cli_async_request(CLI_COMMANDS_POWER_CONFIG, (char*)argv[1], cli_power_config_async_print);
}

static FPFW_CLI_STATUS cli_power_set_command(int argc, const char** argv)
{ 
    if (argc < 2) {
        printf("Usage: pwr set <sub_cmd>\n");
        return CLI_ERROR;
    }

    return dispatch_power_cli_async_request(CLI_COMMANDS_POWER_SET, (char*)argv[1], cli_power_set_async_print);
}

static FPFW_CLI_STATUS cli_power_status_command(int argc, const char** argv)
{ 
    if (argc < 2) {
        printf("Usage: pwr status <sub_cmd>\n");
        return CLI_ERROR;
    }

    return dispatch_power_cli_async_request(CLI_COMMANDS_POWER_STATUS, (char*)argv[1], cli_power_status_async_print);
}

static FPFW_CLI_STATUS cli_power_log_command(int argc, const char** argv)
{
    if (argc < 2) {
        printf("Usage: pwr log <sub_cmd>\n");
        return CLI_ERROR;
    }

    return dispatch_power_cli_async_request(CLI_COMMANDS_POWER_LOG, (char*)argv[1], cli_power_log_async_print);
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
    DfwkAsyncRequestInititalize(&power_cli_cmd_context.request.header, sizeof(power_cli_cmd_context.request.header));

    return CLI_SUCCESS;
}
