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
#include <stdbool.h>                      // for bool
#include <stdint.h>                       // for uint32_t, uint16_t
#include <stdio.h>                        // for NULL
#include <stdlib.h>                       // for strtoul

/*-- Symbolic Constant Macros (defines) --*/
#define MAX_ENTRY_SIZE_BYTES (81)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

static FPFW_CLI_STATUS read_reg(int Argc, const char** Argv);
static FPFW_CLI_STATUS write_reg(int Argc, const char** Argv);
static FPFW_CLI_STATUS list_properties(int Argc, const char** Argv);
static FPFW_CLI_STATUS set_global_hw_enable(int Argc, const char** Argv);
static FPFW_CLI_STATUS reset_fifos(int Argc, const char** Argv);
static FPFW_CLI_STATUS fifo_enable(int Argc, const char** Argv);
static FPFW_CLI_STATUS read_entry(int Argc, const char** Argv);
static FPFW_CLI_STATUS write_entry(int Argc, const char** Argv);
static FPFW_CLI_STATUS update_stride_index(int Argc, const char** Argv);

/*-- Declarations (Statics and globals) --*/
static FPFW_CLI_COMMAND cli_sensor_fifo_commands[] = {
    {NULL_LIST_ENTRY, "snsrfifo", "rdreg", read_reg, "Read a Register", "Usage: rdreg <address in hex>"},
    {NULL_LIST_ENTRY, "snsrfifo", "wrreg", write_reg, "Write a Register", "Usage: wrreg <address in hex> <value in hex>"},
    {NULL_LIST_ENTRY, "snsrfifo", "lprop", list_properties, "List Fifo Properties", "Usage: lprop"},
    {NULL_LIST_ENTRY, "snsrfifo", "globalhwen", set_global_hw_enable, "Set Global HW Enable", "Usage: globalhwen <0,1>"},
    {NULL_LIST_ENTRY, "snsrfifo", "reset", reset_fifos, "Reset all Fifos", "Usage: reset"},
    {NULL_LIST_ENTRY, "snsrfifo", "fifoen", fifo_enable, "Enable/Disable a Fifo", "Usage: fifoen <fifo id> <0,1>"},
    {NULL_LIST_ENTRY, "snsrfifo", "rdentry", read_entry, "Read an entry from a fifo", "Usage: rdentry <fifo id> <read all 0,1>"},
    {NULL_LIST_ENTRY, "snsrfifo", "wrentry", write_entry, "Write an entry to a fifo", "Usage: wrentry <fifo id> <stride index> <quadword0> ... <quadwordn>"},
    {NULL_LIST_ENTRY, "snsrfifo", "updstride", update_stride_index, "Update stride index", "Usage: updstride <fifo id>"},
};

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

    bool enabled[SENSOR_FIFO_MAX_ID] = {0};
    bool empty[SENSOR_FIFO_MAX_ID] = {0};

    sensor_fifo_driver_inf_is_enabled(sp_sensor_fifo_driver_inf, &enabled);
    sensor_fifo_driver_inf_is_empty(sp_sensor_fifo_driver_inf, &empty);

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
        FpFwCliPrint("       enabled = %s: empty = %s\n", enabled[fifo_idx] ? "true" : "false", empty[fifo_idx] ? "yes" : "no");
    }

    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS set_global_hw_enable(int Argc, const char** Argv)
{
    if (Argc == 2)
    {
        bool enable = strtoul(Argv[1], NULL, 10);

        fpfw_status_t status = sensor_fifo_driver_inf_set_global_hw_enable(sp_sensor_fifo_driver_inf, enable);
        if (FPFW_STATUS_SUCCEEDED(status))
        {
            FpFwCliPrint("\nGlobal HW Enable set to %s\n", enable ? "enable" : "disable");
            return CLI_SUCCESS;
        }
    }

    FpFwCliPrint("\nERROR:: %s\n", cli_sensor_fifo_commands[3].Usage);
    return CLI_ERROR;
}

static FPFW_CLI_STATUS reset_fifos(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    fpfw_status_t status = sensor_fifo_driver_reset(sp_sensor_fifo_driver_inf);
    if (FPFW_STATUS_SUCCEEDED(status))
    {
        FpFwCliPrint("\nFifos have been reset\n");
        return CLI_SUCCESS;
    }

    FpFwCliPrint("\nERROR:: %s\n", cli_sensor_fifo_commands[4].Usage);
    return CLI_ERROR;
}

static FPFW_CLI_STATUS fifo_enable(int Argc, const char** Argv)
{
    if (Argc == 3)
    {
        uint32_t fifo_id = strtoul(Argv[1], NULL, 10);
        bool enable = strtoul(Argv[2], NULL, 10);

        fpfw_status_t status = sensor_fifo_driver_inf_set_fifo_enable(sp_sensor_fifo_driver_inf, fifo_id, enable);
        if (FPFW_STATUS_SUCCEEDED(status))
        {
            FpFwCliPrint("\nFifo %d Enable set to %s\n", fifo_id, enable ? "enable" : "disable");
            return CLI_SUCCESS;
        }
    }

    FpFwCliPrint("\nERROR:: %s\n", cli_sensor_fifo_commands[5].Usage);
    return CLI_ERROR;
}

static FPFW_CLI_STATUS read_entry(int Argc, const char** Argv)
{
    FPFW_CLI_STATUS ret_status = CLI_ERROR;
    if (Argc == 3)
    {
        uint32_t fifo = strtoul(Argv[1], NULL, 10);
        bool read_all = strtoul(Argv[2], NULL, 10);
        uint32_t count;
        uint64_t read_buffer[MAX_ENTRY_SIZE_BYTES / QUADWORD_SIZE] = {0};
        uint16_t num_entries_read;
        uint16_t num_entries_remaining;
        uint16_t stride_index;

        sensor_fifo_properties_t properties;
        sensor_fifo_svc_get_properties(fifo, &properties);

        if (properties.entry_size_bytes <= MAX_ENTRY_SIZE_BYTES)
        {
            do
            {
                fpfw_status_t status = sensor_fifo_driver_read_entry(sp_sensor_fifo_driver_inf,
                                                                     fifo,
                                                                     properties.entry_size_bytes,
                                                                     (uint8_t*)read_buffer,
                                                                     &num_entries_read,
                                                                     &num_entries_remaining,
                                                                     &stride_index);
                if (FPFW_STATUS_SUCCEEDED(status))
                {
                    if (num_entries_read > 0)
                    {
                        for (count = 0; count < properties.entry_size_bytes / QUADWORD_SIZE; count++)
                        {
                            // long long print not working
                            uint32_t high = (uint32_t)(read_buffer[count] >> 32);
                            uint32_t low = (uint32_t)(read_buffer[count] & 0xFFFFFFFF);
                            FpFwCliPrint("    Buffer[%d] = 0x%08x%08x\n", count, high, low);
                        }
                        FpFwCliPrint("NOTE:: Number entries remaining = %d, stride index = %d\n\n",
                                     num_entries_remaining,
                                     stride_index);
                    }
                    else
                    {
                        FpFwCliPrint("\n%s is empty!\n", properties.name);
                    }
                }
                else
                {
                    FpFwCliPrint("\nERROR:: sensor_fifo_driver_read_entry returned 0x%x\n", status);
                    break;
                }
            } while (read_all && num_entries_remaining > 0);
            ret_status = CLI_SUCCESS;
        }
        else
        {
            FpFwCliPrint("\nNOTE:: Increase CLI MAX_ENTRY_SIZE_BYTES to > %d\n", properties.entry_size_bytes);
        }
    }
    if (ret_status != CLI_SUCCESS)
    {
        FpFwCliPrint("\nERROR:: %s\n", cli_sensor_fifo_commands[6].Usage);
    }
    return ret_status;
}

static FPFW_CLI_STATUS write_entry(int Argc, const char** Argv)
{
    uint32_t fifo = strtoul(Argv[1], NULL, 10);
    uint32_t stride_index = strtoul(Argv[2], NULL, 10);
    uint32_t num_quadword = Argc - 3; // subtract name, fifo and stride index
    uint64_t write_buffer[MAX_ENTRY_SIZE_BYTES / QUADWORD_SIZE] = {0};

    if (Argc < 4)
    {
        FpFwCliPrint("\nERROR:: %s\n", cli_sensor_fifo_commands[7].Usage);
        return CLI_ERROR;
    }

    if (num_quadword * QUADWORD_SIZE > MAX_ENTRY_SIZE_BYTES)
    {
        FpFwCliPrint("\nERROR:: %s\n", cli_sensor_fifo_commands[7].Usage);
        FpFwCliPrint("\nERROR:: Number of entries exceeds MAX_ENTRY_SIZE_BYTES\n");
        return CLI_ERROR;
    }

    sensor_fifo_properties_t properties;
    sensor_fifo_svc_get_properties(fifo, &properties);

    if ((properties.entry_size_bytes == properties.stride_size_bytes) && (stride_index != 0))
    {
        FpFwCliPrint("\nERROR:: Stride Index should be zero for this FIFO\n");
        return CLI_ERROR;
    }

    if (properties.entry_size_bytes == num_quadword * QUADWORD_SIZE)
    {
        for (uint32_t count = 0; count < num_quadword; count++)
        {
            write_buffer[count] = strtoull(Argv[3 + count], NULL, 16);
        }
        fpfw_status_t status = sensor_fifo_driver_write_entries(sp_sensor_fifo_driver_inf,
                                                                fifo,
                                                                (uint8_t*)write_buffer,
                                                                properties.entry_size_bytes,
                                                                1,
                                                                stride_index);

        if (FPFW_STATUS_FAILED(status))
        {
            FpFwCliPrint("\nERROR:: sensor_fifo_driver_write_entries returned 0x%x\n", status);
        }
        else
        {
            FpFwCliPrint("\nEntry written to Fifo %s!\n", properties.name);
            return CLI_SUCCESS;
        }
    }
    else
    {
        FpFwCliPrint("\nNOTE:: Provide %d bytes for fifo %s\n", properties.entry_size_bytes, properties.name);
    }

    FpFwCliPrint("\nERROR:: %s\n", cli_sensor_fifo_commands[7].Usage);
    return CLI_ERROR;
}

static FPFW_CLI_STATUS update_stride_index(int Argc, const char** Argv)
{
    if (Argc == 2)
    {
        uint32_t fifo_id = strtoul(Argv[1], NULL, 10);
        fpfw_status_t status = sensor_fifo_driver_inf_update_write_stride(sp_sensor_fifo_driver_inf, fifo_id);

        if (FPFW_STATUS_SUCCEEDED(status))
        {
            FpFwCliPrint("\nUpdated stride index for Fifo %d\n", fifo_id);
            return CLI_SUCCESS;
        }
    }

    FpFwCliPrint("\nERROR:: %s\n", cli_sensor_fifo_commands[8].Usage);
    return CLI_ERROR;
}