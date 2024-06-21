//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implementation of the sensor fifo service.
 */

/*------------- Includes -----------------*/

#include "FpFwLinkedList.h" // for NULL_LIST_ENTRY

#include <FpFwCli.h>                      // for FPFW_CLI_STATUS, FpFwCliRe...
#include <FpFwUtils.h>                    // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <sensor_fifo_cli_service.h>      // for sensor_fifo_cli_service_in...
#include <sensor_fifo_driver_interface.h> // for sensor_fifo_driver_interfa...
#include <stdio.h>                        // for printf

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

static FPFW_CLI_STATUS read_scf(int Argc, const char** Argv);
static FPFW_CLI_STATUS write_scf(int Argc, const char** Argv);

/*-- Declarations (Statics and globals) --*/

static FPFW_CLI_COMMAND cli_sensor_fifo_commands[] = {
    {NULL_LIST_ENTRY, "snsrfifo", "read_scf", read_scf, "Read an SCF CSR", "Usage: read_scf <address in hex>"},
    {NULL_LIST_ENTRY, "snsrfifo", "write_scf", write_scf, "Write an SCF CSR", "Usage: write_scf <address in hex> <value in hex>"}};

/*------------- Functions ----------------*/

static sensor_fifo_driver_interface_t* sp_sensor_fifo_driver_inf;

void sensor_fifo_cli_svc_initialize(sensor_fifo_driver_interface_t* driver_interface)
{
    sp_sensor_fifo_driver_inf = driver_interface;

    FpFwCliRegisterTable(&cli_sensor_fifo_commands[0], FPFW_ARRAY_SIZE(cli_sensor_fifo_commands));
}

static FPFW_CLI_STATUS read_scf(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    printf("read_scf from snsrfifo cli!!\n");

    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS write_scf(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    printf("write_scf from snsrfifo cli!!\n");

    return CLI_SUCCESS;
}
