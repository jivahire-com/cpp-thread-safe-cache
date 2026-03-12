//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file tlm_fuses_cli_service.c
 * Implementation of the telemetry fuses cli service.
 */

/*------------- Includes -----------------*/

#include <FpFwCli.h>
#include <FpFwUtils.h>
#include <fpfw_status.h>
#include <kng_soc_constants.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tlm_fuses.h>
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

static FPFW_CLI_STATUS read_ecid(int Argc, const char** Argv);
static FPFW_CLI_STATUS read_tile_dts_coeff(int Argc, const char** Argv);
static FPFW_CLI_STATUS read_soc_dts_coeff(int Argc, const char** Argv);

/*-- Declarations (Statics and globals) --*/
static FPFW_CLI_COMMAND cli_tlm_fuses_commands[] = {
    {NULL_LIST_ENTRY, "tlmFuses", "ecid", read_ecid, "Read the ECID", "ecid"},
    {NULL_LIST_ENTRY, "tlmFuses", "tileDtsCoeff", read_tile_dts_coeff, "Read tile dts coefficients", "tileDtsCoeff"},
    {NULL_LIST_ENTRY, "tlmFuses", "socDtsCoeff", read_soc_dts_coeff, "Read soc top dts coefficients", "socDtsCoeff"},
};

/*------------- Functions ----------------*/

PLACED_CODE void tlm_fuses_cli_svc_init(void)
{
    FpFwCliRegisterTable(&cli_tlm_fuses_commands[0], FPFW_ARRAY_SIZE(cli_tlm_fuses_commands));
}

static PLACED_CODE FPFW_CLI_STATUS read_ecid(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    FPFW_CLI_STATUS cli_status = CLI_SUCCESS;

    ecid_t fuses_ecid = {0};
    if (tlm_fuses_get_ecid(&fuses_ecid) != FPFW_STATUS_SUCCESS)
    {
        FpFwCliPrint("\nFailed to read ECID.\n");
        cli_status = CLI_ERROR;
    }
    else
    {
        FpFwCliPrint("\nECID:\n");
        FpFwCliPrint("\tWafer Lot Number:\n");
        for (uint8_t i = 0; i < ECID_WAFER_LOT_NUMBER_CHAR_SIZE; ++i)
        {
            FpFwCliPrint("\t\tChar[%u]: %c\n", i, fuses_ecid.wafer_lot_num[i]);
        }
        FpFwCliPrint("\tWafer Number: 0x%x\n", fuses_ecid.wafer_num);
        FpFwCliPrint("\tX Coordinate: 0x%x\n", fuses_ecid.x_coord);
        FpFwCliPrint("\tY Coordinate: 0x%x\n", fuses_ecid.y_coord);
        FpFwCliPrint("\tParity Bits:  0x%x\n", fuses_ecid.parity_bits);
    }

    return cli_status;
}

static PLACED_CODE FPFW_CLI_STATUS read_tile_dts_coeff(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    FPFW_CLI_STATUS cli_status = CLI_SUCCESS;

    dts_tlm_coeff_t tileDtsCoeffs[NUM_CPU_TILES] = {0};
    memset(tileDtsCoeffs, 0, sizeof(tileDtsCoeffs));

    // Init power fuse values
    if (tlm_fuses_get_dts_coeff_tile(tileDtsCoeffs, NUM_CPU_TILES) != FPFW_STATUS_SUCCESS)
    {
        FpFwCliPrint("\nFailed to read all Tile DTS coefficients. Reporting what we have.\n");
        cli_status = CLI_ERROR;
    }

    FpFwCliPrint("\nTile DTS coefficients:\n");
    for (int i = 0; i < NUM_CPU_TILES; i++)
    {
        FpFwCliPrint("\tTile[%d] DTS coefficients: k_val = %d y_val = %d\n",
                     i,
                     tileDtsCoeffs[i].k_val,
                     tileDtsCoeffs[i].y_val);
    }

    return cli_status;
}

static PLACED_CODE FPFW_CLI_STATUS read_soc_dts_coeff(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    FPFW_CLI_STATUS cli_status = CLI_SUCCESS;

    dts_tlm_coeff_t soctopDtsCoefficientsTest[NUM_CPU_TILES] = {0};
    memset(soctopDtsCoefficientsTest, 0, sizeof(soctopDtsCoefficientsTest));

    // Init power fuse values
    if (tlm_fuses_get_dts_coeff_soctop(soctopDtsCoefficientsTest, NUM_CPU_TILES) != FPFW_STATUS_SUCCESS)
    {
        FpFwCliPrint("\nFailed to read all SoC top DTS coefficients. Reporting what we have.\n");
        cli_status = CLI_ERROR;
    }

    FpFwCliPrint("\nSoC top DTS coefficients:\n");
    for (int i = 0; i < NUM_CPU_TILES; i++)
    {
        FpFwCliPrint("\tSoC Top[%d] DTS coefficients: k_val = %d y_val = %d\n",
                     i,
                     soctopDtsCoefficientsTest[i].k_val,
                     soctopDtsCoefficientsTest[i].y_val);
    }

    return cli_status;
}
