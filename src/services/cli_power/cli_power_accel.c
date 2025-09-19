//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_power_accel.c
 * Source file to implement power accelerator management.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <accelip_id.h>
#include <atu_init.h>
#include <cli_power_accel.h>
#include <sdm_qos.h>
#include <stdio.h>
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/
#define POWER_CLI_STR "PWR CLI"

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
const power_cli_sub_command_dictionary_element_t power_cli_accel_sub_command_dictionary[] = {
    {"bw_reduce", NULL, POWER_IF_CMD_ACCEL_BW_REDUCE},
};

static const uint32_t length_accel_commands_dictionary =
    sizeof(power_cli_accel_sub_command_dictionary) / sizeof(power_cli_sub_command_dictionary_element_t);

PLACED_CODE void cli_power_accel_complete(PDFWK_ASYNC_REQUEST_HEADER p_request, void* completion_context)
{
    FPFW_UNUSED(completion_context);
    ppower_service_cli_request_t p_cli_request = (ppower_service_cli_request_t)p_request;

    if (p_cli_request == NULL)
    {
        FPFW_DBGPRINT_ERROR("[%s] Invalid cli request\n", POWER_CLI_STR);
        return;
    }

    switch (p_cli_request->power_ext_if_cmd_id)
    {
    case POWER_IF_CMD_ACCEL_BW_REDUCE: {
        uintptr_t sdm_pf_cfg_addr = atu_svc_accel_atu_addr(p_cli_request->pwrset_sub_command_args.accelparams.accel_id);
        silibs_status_t silibs_status;

        FPFW_DBGPRINT_VERBOSE("[%s] sdm_pf_cfg_addr: %x\n", POWER_CLI_STR, sdm_pf_cfg_addr);
        FPFW_DBGPRINT_VERBOSE("[%s] accel_id: %x\n",
                              POWER_CLI_STR,
                              p_cli_request->pwrset_sub_command_args.accelparams.accel_id);
        FPFW_DBGPRINT_VERBOSE("[%s] bw_reduction: %d\n",
                              POWER_CLI_STR,
                              p_cli_request->pwrset_sub_command_args.accelparams.bw_reduction_perc);
        FPFW_DBGPRINT_VERBOSE("[%s] accel_id: %d\n",
                              POWER_CLI_STR,
                              p_cli_request->pwrset_sub_command_args.accelparams.dual_bus);

        silibs_status = sdm_bw_management(sdm_pf_cfg_addr,
                                          p_cli_request->pwrset_sub_command_args.accelparams.bw_reduction_perc,
                                          p_cli_request->pwrset_sub_command_args.accelparams.dual_bus);
        if (silibs_status != SILIBS_SUCCESS)
        {
            FPFW_DBGPRINT_ERROR("[%s] Error while setting bandwidth management. Error code: %d\n", POWER_CLI_STR, silibs_status);
        }
        else
        {
            FPFW_DBGPRINT_INFO("[%s] Accelerator bandwidth power reduction complete\n", POWER_CLI_STR);
        }
        break;
    }

    default: {
        FPFW_DBGPRINT_ERROR("[%s] Unsupported power accel subcommand: %d\n", POWER_CLI_STR, p_cli_request->power_ext_if_cmd_id);
        break;
    }
    }
}

power_if_cmd_t cli_power_accel_cmd_id(const char* sub_command)
{
    if (sub_command == NULL)
    {
        return POWER_IF_CMD_UNKNOWN;
    }

    /* Parse dictionary for subcommand and fetch corresponding power_ext_if_cmd_id */
    for (uint32_t index = 0; index < length_accel_commands_dictionary; index++)
    {
        if (strcmp(sub_command, power_cli_accel_sub_command_dictionary[index].sub_command) == 0)
        {
            return power_cli_accel_sub_command_dictionary[index].power_ext_if_cmd_id;
        }
    }

    /* If sub-command is not found in the dictionary */
    return POWER_IF_CMD_UNKNOWN;
}
