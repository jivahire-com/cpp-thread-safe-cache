//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_ddr.c
 * Source file to implement ddr cli
 */

/*------------- Includes -----------------*/
#include <FpFwCli.h>        // for FPFW_CLI_STATUS, CLI_SUCCESS, FpFwCliReg...
#include <FpFwLinkedList.h> // for NULL_LIST_ENTRY
#include <FpFwUtils.h>      // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <ddrss_lib.h>
#include <stdio.h> // for printf

#ifdef UNIT_TEST
    #define STATIC
#else
    #define STATIC static
#endif

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/* Added 3 placeholder commands to set the structure - Functions to be plugged when Silibs apis are available */
STATIC FPFW_CLI_STATUS ecc_ce_error_injection(int Argc, const char** Argv);
STATIC FPFW_CLI_STATUS ecc_ue_error_injection(int Argc, const char** Argv);
STATIC FPFW_CLI_STATUS cmd_addr_parity_error_injection(int Argc, const char** Argv);

/*-- Declarations (Statics and globals) --*/

// TODO Silibs to provide APIs with arguments list
// https://dev.azure.com/ms-tsd/Base_IP/_workitems/edit/584974
STATIC FPFW_CLI_COMMAND cli_ddr_commands[] = {
    {NULL_LIST_ENTRY, "ddr_err_inj", "ecc_ce_err", ecc_ce_error_injection, "ECC CE error injection", "Usage: ecc_ce_err <parameter list undefined>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "ecc_ue_err", ecc_ue_error_injection, "ECC UE error injection", "Usage: ecc_ue_err <parameter list undefined>"},
    {NULL_LIST_ENTRY, "ddr_err_inj", "capar_err", cmd_addr_parity_error_injection, "CAPAR (Cmd/Addr Parity) error", "Usage: caddr_parity_err <parameter list undefined>"},
};

/*-------------- Functions ---------------*/
STATIC FPFW_CLI_STATUS ecc_ce_error_injection(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    printf("Work in progress: ecc_ce_error_injection!!\n");

    return CLI_SUCCESS;
}

STATIC FPFW_CLI_STATUS ecc_ue_error_injection(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    // TODO Silibs to provide APIs with exact arguments list
    // https://dev.azure.com/ms-tsd/Base_IP/_workitems/edit/584974
    printf("Work in progress: ecc_ue_error_injection!!\n");

    return CLI_SUCCESS;
}

STATIC FPFW_CLI_STATUS cmd_addr_parity_error_injection(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    // TODO Silibs to provide APIs with exact arguments list
    // https://dev.azure.com/ms-tsd/Base_IP/_workitems/edit/584974
    printf("Work in progress: cmd_addr_parity_error_injection!!\n");

    return CLI_SUCCESS;
}

void cli_ddr_init(void)
{
    FpFwCliRegisterTable(&cli_ddr_commands[0], FPFW_ARRAY_SIZE(cli_ddr_commands));
}
