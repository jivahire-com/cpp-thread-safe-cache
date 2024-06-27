//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implementation of the sensor fifo service.
 */

/*------------- Includes -----------------*/

#include "FpFwLinkedList.h" // for NULL_LIST_ENTRY
#include "fpfw_status.h"    // for FPFW_STATUS_SUCCEEDED, fpf...

#include <FpFwCli.h>                      // for FpFwCliPrint, FPFW_CLI_COM...
#include <FpFwUtils.h>                    // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <sensor_fifo_cli_service.h>      // for sensor_fifo_cli_svc_initia...
#include <sensor_fifo_driver_interface.h> // for sensor_fifo_driver_inf_rea...
#include <sensor_fifo_service.h>          // for sensor_fifo_properties_t
#include <stdint.h>                       // for uint32_t, uint16_t
#include <stdio.h>                        // for NULL
#include <stdlib.h>                       // for strtoul

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

static FPFW_CLI_STATUS read_reg(int Argc, const char** Argv);
static FPFW_CLI_STATUS write_reg(int Argc, const char** Argv);
static FPFW_CLI_STATUS list_properties(int Argc, const char** Argv);

/*-- Declarations (Statics and globals) --*/

static FPFW_CLI_COMMAND cli_sensor_fifo_commands[] = {
    {NULL_LIST_ENTRY, "snsrfifo", "rdreg", read_reg, "Read an SCF CSR", "Usage: rdreg <address in hex>"},
    {NULL_LIST_ENTRY, "snsrfifo", "wrreg", write_reg, "Write an SCF CSR", "Usage: wrreg <address in hex> <value in hex>"},
    {NULL_LIST_ENTRY, "snsrfifo", "lprop", list_properties, "List Fifo Properties", "Usage: lprop"}};

/*------------- Functions ----------------*/

static psensor_fifo_driver_interface_t sp_sensor_fifo_driver_inf;

void sensor_fifo_cli_svc_initialize(sensor_fifo_driver_interface_t* driver_interface)
{
    sp_sensor_fifo_driver_inf = driver_interface;

    FpFwCliRegisterTable(&cli_sensor_fifo_commands[0], FPFW_ARRAY_SIZE(cli_sensor_fifo_commands));
}

static FPFW_CLI_STATUS read_reg(int Argc, const char** Argv)
{
    if (Argc == 2)
    {
        uint32_t register_address = strtoul(Argv[1], NULL, 16);
        uint32_t value;

        fpfw_status_t status = sensor_fifo_driver_inf_read_reg(sp_sensor_fifo_driver_inf, register_address, &value);
        if (FPFW_STATUS_SUCCEEDED(status))
        {
            FpFwCliPrint("\nRead SCF Register: Address 0x%x = 0x%x\n", register_address, value);
            return CLI_SUCCESS;
        }
    }

    FpFwCliPrint("\nERROR:: %s\n", cli_sensor_fifo_commands[0].Usage);
    return CLI_ERROR;
}

static FPFW_CLI_STATUS write_reg(int Argc, const char** Argv)
{
    if (Argc == 3)
    {
        uint32_t register_address = strtoul(Argv[1], NULL, 16);
        uint32_t value = strtoul(Argv[2], NULL, 16);

        fpfw_status_t status = sensor_fifo_driver_inf_write_reg(sp_sensor_fifo_driver_inf, register_address, value);
        if (FPFW_STATUS_SUCCEEDED(status))
        {
            FpFwCliPrint("\nWrite SCF Register: Address 0x%x = 0x%x\n", register_address, value);
            return CLI_SUCCESS;
        }
    }

    FpFwCliPrint("\nERROR:: %s\n", cli_sensor_fifo_commands[1].Usage);
    return CLI_ERROR;
}

static FPFW_CLI_STATUS list_properties(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    sensor_fifo_properties_t properties;

    for (uint16_t fifo_idx = 0; fifo_idx < SENSOR_FIFO_MAX_ID; fifo_idx++)
    {
        sensor_fifo_svc_get_properties(fifo_idx, &properties);
        FpFwCliPrint("\nFifo ID = %d: Fifo Name = %s\n", fifo_idx, properties.name);
        FpFwCliPrint("       entry size = %d: stride size = %d: epoch count = %d\n",
                     properties.entry_size_bytes,
                     properties.stride_size_bytes,
                     properties.epoch_count);
        FpFwCliPrint("       start addr = 0x%x: end addr = 0x%x\n", properties.start_address, properties.end_address);
    }

    return CLI_SUCCESS;
}
