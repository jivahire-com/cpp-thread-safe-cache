//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_power_set.c
 * Source file with implementations of power set CLI commands.
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h>
#include <cli_power_common.h>
#include <cli_power_interface.h>
#include <cli_power_status.h>
#include <corebits.h>
#include <power_dfwk.h>
#include <power_runconfig.h>
#include <silibs_common.h>
#include <stdio.h>
#include <string.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-- Declarations (Statics and globals) --*/

// clang-format off
/* Structure storing the dictionary of sub-command string against its corresponding command id and callback function */
/* For now, since the set functions are only dummy calls, the completion callbacks are set to NULL. To be updated (ADO: 1887411)*/
const power_cli_sub_command_dictionary_element_t power_cli_set_sub_command_dictionary[] = {
    {"cap",        NULL, POWER_IF_CMD_SET_CAP            },
    {"desired",    NULL, POWER_IF_CMD_SET_DESIRED_PSTATE },
    {"plimit",     NULL, POWER_IF_CMD_SET_PLIMIT         },
    {"loopdis",    NULL, POWER_IF_CMD_SET_LOOP_DISABLES  },
    {"minupdate",  NULL, POWER_IF_CMD_SET_RACK_LIMIT     },
    {"nominal",    NULL, POWER_IF_CMD_SET_MINUPDATE      },
    {"racklimit",  NULL, POWER_IF_CMD_SET_NOMINAL        },
};
//clang-format on

const uint32_t length_set_commands_dictionary =
    sizeof(power_cli_set_sub_command_dictionary) / sizeof(power_cli_sub_command_dictionary_element_t);

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Functions ---------------*/

power_if_cmd_t cli_power_set_get_cmd_id(char* sub_command)
{
    if (sub_command == NULL)
    {
        return POWER_IF_CMD_UNKNOWN;
    }

    /* Parse dictionary for subcommand and fetch corresponding power_ext_if_cmd_id */
    for (uint32_t index = 0; index < length_set_commands_dictionary; index++)
    {
        if (strcmp(sub_command, power_cli_set_sub_command_dictionary[index].sub_command) == 0)
        {
            return power_cli_set_sub_command_dictionary[index].power_ext_if_cmd_id;
        }
    }

    /* If sub-command is not found in the dictionary */
    return POWER_IF_CMD_UNKNOWN;
}

void cli_power_set_async_print(PDFWK_ASYNC_REQUEST_HEADER p_request, void* completion_context)
{
    FPFW_UNUSED(p_request);
    FPFW_UNUSED(completion_context);
}
