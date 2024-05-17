//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_power.c
 * Source file to implement power cli
 */

/*------------- Includes -----------------*/
#include <FpFwCli.h>
#include <FpFwLinkedList.h>
#include <FpFwUtils.h>
#include <cli_power.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/* Added 2 placeholder commands to set the structure - Functions to be ported soon */
static FPFW_CLI_STATUS hey(int Argc, const char** Argv);
static FPFW_CLI_STATUS howyoudoin(int Argc, const char** Argv);

/*-- Declarations (Statics and globals) --*/

static FPFW_CLI_COMMAND cli_power_commands[] = {
    {NULL_LIST_ENTRY, "pwr", "hey", hey, "This command prints says hey from power cli", "Usage: hey"},
    {NULL_LIST_ENTRY, "pwr", "howyoudoin", howyoudoin, "This command prints replies how power cli is doing", "Usage: howyoudoin"}};

/*-------------- Functions ---------------*/
static FPFW_CLI_STATUS hey(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    printf("Hey from power cli!!\n");

    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS howyoudoin(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    printf("I'm just getting started on this, figuring things out. How are you doing?\n");

    return CLI_SUCCESS;
}

FPFW_CLI_STATUS cli_power_init(void)
{
    FpFwCliRegisterTable(&cli_power_commands[0], FPFW_ARRAY_SIZE(cli_power_commands));

    return CLI_SUCCESS;
}
