//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scmi_cli.c - SCMI CLI commands
 */

/*------------- Includes -----------------*/
#include <FpFwCli.h>   // for FpFwCliPrint, _FPFW_CLI_STATUS, FPF...
#include <FpFwUtils.h> // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <scmi_cli.h>  // for icc_cli_init
#include <scmi_prim.h> // for
#include <stdint.h>    // for uint32_t, uint8_t
#include <stdlib.h>    // for atoi
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

static FPFW_CLI_STATUS scmi_setm(int argc, const char** argv);
static FPFW_CLI_STATUS scmi_send(int argc, const char** argv);

/*-- Declarations (Statics and globals) --*/

static FPFW_CLI_COMMAND s_scmi_cmd_list[] = {
    {NULL_LIST_ENTRY, "scmi", "scmi_setm", scmi_setm, "Sets SCMI Debug Mode", "scmi_setm <mode>"},
    {NULL_LIST_ENTRY, "scmi", "scmi_send", scmi_send, "Sends SCMI response", "scmi_send <procol_id> <command> <size> <data0> .. <datan>"},
};

/*------------- Functions ----------------*/

PLACED_CODE void scmi_cli_init(void)
{
    FpFwCliRegisterTable(s_scmi_cmd_list, FPFW_ARRAY_SIZE(s_scmi_cmd_list));
}

static PLACED_CODE FPFW_CLI_STATUS scmi_setm(int argc, const char** argv)
{
    FPFW_UNUSED(argv);
    if (argc != 2)
    {
        FpFwCliPrint("ERROR! Insufficient Args\n");
        return CLI_ERROR;
    }
    uint8_t mode = atoi(argv[1]);
    scmi_set_debug_mode(mode);
    FpFwCliPrint("Set SCMI Mode: %x\n", mode);

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS scmi_send(int argc, const char** argv)
{

    if (argc < 4)
    {
        FpFwCliPrint("ERROR! Insufficient Args\n");
        return CLI_ERROR;
    }
    uint8_t protocol_id = atoi(argv[1]);
    uint8_t cmd_id = atoi(argv[2]);
    size_t size = atoi(argv[3]);
    uint8_t data[32];

    if (size <= sizeof(data))
    {
        if (size != 0 && argc > 4 && (size <= (size_t)(argc - 4)))
        {
            for (uint8_t count = 0; count < size; count++)
            {
                data[count] = atoi(argv[4 + count]);
            }
        }

        scmi_send_resp(protocol_id, cmd_id, data, size);
        FpFwCliPrint("SCMI Send Response \n");
    }
    else
    {
        FpFwCliPrint("too many payloads \n");
    }

    return CLI_SUCCESS;
}
