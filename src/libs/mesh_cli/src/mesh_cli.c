//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mesh_cli.c
 * This file contains the implementation of the Mesh CLI interface
 * and related functionality.
 */

/*------------- Includes -----------------*/
#include <DfwkClient.h> // for DfwkAsyncRequestInititalize, PDFWK_INTER...
#include <FpFwCli.h>
#include <FpFwLinkedList.h>
#include <FpFwUtils.h>
#include <MboxPrimitives.h> // for FPFW_MBX_PAYLOAD, FpFwMailbox...
#include <mesh_cli.h>
#include <stdio.h>
#include <stdlib.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
static FPFW_CLI_STATUS mesh_echo_cli(int argc, const char** argv);

/*-- Declarations (Statics and globals) --*/

static FPFW_CLI_COMMAND mesh_cli_list[] = {
    {NULL_LIST_ENTRY, "mesh", "mesh_echo", mesh_echo_cli, "mesh echo data", "Usage: mesh_echo <32-bit address(in Hex)> <32-bit data(in Hex)>"},

};

/*------------- Functions ----------------*/

static FPFW_CLI_STATUS mesh_echo_cli(int argc, const char** argv)
{
    FpFwCliPrint("\nmesh_echo_cli func. call\n");

    if (argc == 3)
    {
        char* endptr;
        unsigned long addr32 = strtoul(argv[1], &endptr, 16);
        if (*endptr != '\0')
        {
            FpFwCliPrint("1st arg %s is Invalid Hex value\n", argv[1]);
            return CLI_ERROR;
        }
        unsigned long data32 = strtoul(argv[2], &endptr, 16);
        if (*endptr != '\0')
        {
            FpFwCliPrint("2nd arg %s is Invalid Hex value\n", argv[2]);
            return CLI_ERROR;
        }
        FpFwCliPrint("mesh echo\n");
        FpFwCliPrint("addr32: 0x%x, data32: 0x%x\n", addr32, data32);
    }
    else
    {
        FpFwCliPrint(" Mesh Echo CLI Help\n");
        FpFwCliPrint("Cmds: 2, <32-bit address(in Hex)> <32-bit data(in Hex)\n");
        return CLI_ERROR;
    }
    return CLI_SUCCESS;
}

void mesh_cli_initialize(void)
{
    FpFwCliRegisterTable(mesh_cli_list, FPFW_ARRAY_SIZE(mesh_cli_list));

    FpFwCliPrint("Mesh CLI init complete\n");
}
