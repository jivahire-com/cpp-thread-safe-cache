//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file  config_manager_cli.c
 * This file contains the implementation of the  cfg_mgr CLI interface
 * and related functionality.
 */

/*------------- Includes -----------------*/
#include <FpFwCli.h>
#include <FpFwUtils.h>
#include <config_manager.h>
#include <config_manager_cli.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MAX_KNOB_SIZE 128
/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
static FPFW_CLI_STATUS cfg_mgr_knob_list_cli(int argc, const char** argv);
static FPFW_CLI_STATUS cfg_mgr_knob_data_dump_cli(int argc, const char** argv);
static FPFW_CLI_STATUS cfg_mgr_set_knob_cli(int argc, const char** argv);
static FPFW_CLI_STATUS cfg_mgr_reset_knob_cli(int argc, const char** argv);

/*-- Declarations (Statics and globals) --*/
extern knob_data_t g_knob_data[];

static FPFW_CLI_COMMAND cfg_mgr_cli_list[] = {
    {NULL_LIST_ENTRY, "cfg_mgr", "cfg_mgr_list", cfg_mgr_knob_list_cli, "show knobs list", "Usage: cfg_mgr_list"},
    {NULL_LIST_ENTRY, "cfg_mgr", "cfg_mgr_dump", cfg_mgr_knob_data_dump_cli, "show all knobs data or by name, index", "Usage: cfg_mgr_dump <<name or index>>"},
    {NULL_LIST_ENTRY, "cfg_mgr", "cfg_mgr_set", cfg_mgr_set_knob_cli, "overwrites the value of the specified knob", "Usage: cfg_mgr_set <<name or index>> <data bytes>"},
    {NULL_LIST_ENTRY, "cfg_mgr", "cfg_mgr_reset", cfg_mgr_reset_knob_cli, "reset the value of the specified knob", "Usage: cfg_mgr_reset <<name or index>>"}};

/*------------- Functions ----------------*/
static cached_knob_data_t* get_knob_data_by(const char* argument, uint32_t* knob_idx)
{
    cached_knob_data_t* current_entry = NULL;

    // If the argument is a number, return the index directly
    if (argument[0] >= '0' && argument[0] <= '9')
    {
        *knob_idx = atoi(argument);

        if (*knob_idx >= cached_knob_data_size())
        {
            current_entry = NULL;
        }
        else
        {
            current_entry = &get_cached_knob_data()[*knob_idx];
        }
    }
    else
    {
        for (uint32_t idx = 0; idx < cached_knob_data_size(); idx++)
        {
            cached_knob_data_t* current = &(get_cached_knob_data()[idx]);
            if (0 == strcmp(current->name, argument))
            {
                *knob_idx = idx;
                current_entry = current;
            }
        }
    }

    return current_entry;
}

static void dump_knob_data(cached_knob_data_t* current_entry, uint32_t idx)
{
    FpFwCliPrint("Knob@[%d] : Name[%s], Size[%d], Namespace[", idx, current_entry->name, current_entry->size);
    FpFwCliPrint("%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X]\n",
                 current_entry->knob_namespace->data[0],
                 current_entry->knob_namespace->data[1],
                 current_entry->knob_namespace->data[2],
                 current_entry->knob_namespace->data[3],
                 current_entry->knob_namespace->data[4],
                 current_entry->knob_namespace->data[5],
                 current_entry->knob_namespace->data[6],
                 current_entry->knob_namespace->data[7],
                 current_entry->knob_namespace->data[8],
                 current_entry->knob_namespace->data[9],
                 current_entry->knob_namespace->data[10],
                 current_entry->knob_namespace->data[11],
                 current_entry->knob_namespace->data[12],
                 current_entry->knob_namespace->data[13],
                 current_entry->knob_namespace->data[14],
                 current_entry->knob_namespace->data[15]);
    FpFwCliPrint("Knob Value : [Byte View]");

    for (uint32_t dataIdx = 0; dataIdx < current_entry->size; dataIdx++)
    {
        if ((dataIdx % 16) == 0)
        {
            FpFwCliPrint("\n\t\t");
        }
        else
        {
            FpFwCliPrint(" ");
        }

        FpFwCliPrint("%02x", ((char*)current_entry->data)[dataIdx]);
    }

    FpFwCliPrint("\nKnob Value : [Primitive Data If Possible(uint8_t <-> uint64_t)]");

    switch (current_entry->size)
    {
    case 1:
        FpFwCliPrint("\n\t\t(%u)\n", *(uint8_t*)current_entry->data);
        break;
    case 2:
        FpFwCliPrint("\n\t\t(%u)\n", *(uint16_t*)current_entry->data);
        break;
    case 4:
        FpFwCliPrint("\n\t\t(%u)\n", *(uint32_t*)current_entry->data);
        break;
    case 8:
        FpFwCliPrint("\n\t\t(%llu)\n", *(uint64_t*)current_entry->data);
        break;
    default:
        FpFwCliPrint("\n\t\tData type is not primitive type\n");
    }
}

static FPFW_CLI_STATUS cfg_mgr_knob_list_cli(int argc, const char** argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(argv);

    cached_knob_data_t* current_entry = NULL;

    FpFwCliPrint("\n--System Knob Configuration Entities--\n");
    for (uint32_t idx = 0; idx < cached_knob_data_size(); idx++)
    {
        current_entry = &(get_cached_knob_data()[idx]);
        FpFwCliPrint("Knob Index[%d], Name[%s]\n", idx, current_entry->name);
    }
    FpFwCliPrint("\n--System Knob Configuration Entities Completed--\n");

    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS cfg_mgr_knob_data_dump_cli(int argc, const char** argv)
{
    cached_knob_data_t* current_entry = NULL;

    if (argc == 1)
    {
        FpFwCliPrint("\n--System Knob Configuration Entities--\n");
        for (uint32_t idx = 0; idx < cached_knob_data_size(); idx++)
        {
            current_entry = &(get_cached_knob_data()[idx]);
            dump_knob_data(current_entry, idx);
        }
        FpFwCliPrint("\n--System Knob Configuration Entities Completed--\n");
    }
    else if (argc == 2)
    {
        uint32_t knob_idx = 0;
        current_entry = get_knob_data_by(argv[1], &knob_idx);

        if (current_entry == NULL)
        {
            FpFwCliPrint("Failed: Check Index is valid or Knob name\n");
            return CLI_ERROR;
        }

        dump_knob_data(current_entry, knob_idx);
    }
    else
    {
        FpFwCliPrint("Failed: Invalid Parameter\n");
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS cfg_mgr_set_knob_cli(int argc, const char** argv)
{
    uint32_t knob_idx = 0;
    cached_knob_data_t* current_entry = NULL;

    if (argc < 2)
    {
        FpFwCliPrint("Failed: Invalid Parameter\n");
        return CLI_ERROR;
    }

    current_entry = get_knob_data_by(argv[1], &knob_idx);

    if (current_entry == NULL)
    {
        FpFwCliPrint("Failed: Check Index is valid or Knob name\n");
        return CLI_ERROR;
    }

    if (argc != (int32_t)(2 + current_entry->size))
    {
        FpFwCliPrint("Data size doesn't match what [%s]knob has : expected size[%d]\n",
                     current_entry->name,
                     current_entry->size);
        return CLI_ERROR;
    }

    FpFwCliPrint("\n--Knob configuration update--\n");
    FpFwCliPrint("--Current knob data dump\n");
    dump_knob_data(current_entry, knob_idx);

    uint8_t buffer[MAX_KNOB_SIZE] = {};
    for (uint32_t i = 0; i < current_entry->size; i++)
    {
        ((char*)buffer)[i] = strtol(argv[2 + i], NULL, 16) & 0xff;
    }

    bool result = update_knob_data(current_entry, buffer, current_entry->size, true);

    if (!result)
    {
        FpFwCliPrint("Knob update into HSP get failed as HSP not present\n");
        FpFwCliPrint("Knob update data will not be permanent\n");
    }

    FpFwCliPrint("--New knob data dump\n");
    dump_knob_data(current_entry, knob_idx);

    FpFwCliPrint("\n--Knob configuration update completed--\n");

    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS cfg_mgr_reset_knob_cli(int argc, const char** argv)
{
    uint32_t knob_idx = 0;
    cached_knob_data_t* current_entry = NULL;

    if (argc < 2)
    {
        FpFwCliPrint("Incorrect Parameters\n");
        FpFwCliPrint("Syntax: cfg_mgr_reset <<name or index>>\n");
        return CLI_ERROR;
    }

    current_entry = get_knob_data_by(argv[1], &knob_idx);

    if (current_entry == NULL)
    {
        FpFwCliPrint("Failed: Check Index is valid or Knob name\n");
        return CLI_ERROR;
    }

    FpFwCliPrint("--knob data dump before reset\n");
    dump_knob_data(current_entry, knob_idx);

    if (current_entry->overridden)
    {
        bool result =
            update_knob_data(current_entry, g_knob_data[knob_idx].default_value_address, current_entry->size, true);

        if (!result)
        {
            FpFwCliPrint("Knob update into HSP get failed as HSP not present\n");
            FpFwCliPrint("Knob update data will not be permanent\n");
        }
        current_entry->overridden = false;
    }
    else
    {
        FpFwCliPrint("Knob is not overridden, nothing to reset\n");
    }

    FpFwCliPrint("--knob data dump after reset\n");
    dump_knob_data(current_entry, knob_idx);

    return CLI_SUCCESS;
}

void cfg_mgr_cli_init(void)
{
    FpFwCliRegisterTable(cfg_mgr_cli_list, FPFW_ARRAY_SIZE(cfg_mgr_cli_list));
}
