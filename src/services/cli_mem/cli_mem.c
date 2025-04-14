//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file cli_mem.c Memory access cli commands
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FpFwCli.h>
#include <FpFwUtils.h> // for FPFW_ARRAY_SIZE, FPFW_UNUSED
#include <atu_lib.h>
#include <kng_soc_constants.h>
#include <stdio.h>
#include <stdlib.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
static FPFW_CLI_STATUS ap_mem_read(int argc, const char* argv[]);
static FPFW_CLI_STATUS ap_mem_write(int argc, const char* argv[]);

/*-- Declarations (Statics and globals) --*/
static FPFW_CLI_COMMAND s_mem_cmd_list[] = {
    {NULL_LIST_ENTRY, "mem", "ap_mem_read", ap_mem_read, "Read AP memory", "Usage: ap_mem_read <size in 32-bit words> <addr>"},
    {NULL_LIST_ENTRY, "mem", "ap_mem_write", ap_mem_write, "Write AP memory", "Usage: ap_mem_write <addr> <32-bit data>"}};

/*------------- Functions ----------------*/
// Command to read memory
static FPFW_CLI_STATUS ap_mem_read(int argc, const char* argv[])
{
    if (argc != 3)
    {
        FpFwCliPrint("Usage: ap_mem_read <size in 32-bit words> <addr>\n");
        return CLI_ERROR;
    }

    int size = atoi(argv[1]);
    uint64_t addr = strtoull(argv[2], NULL, 0);

    // if an existing mapping is not found, create a new one
    atu_map_entry_t existing_map_entry = {
        .ap_base_address = addr,
        .mscp_start_address = 0,
        .mscp_end_address = 0,
        .attribute = {.as_uint32 = 0} // Default attributes
    };

    bool found_existing_mapping = false;
    uintptr_t mapped_addr;
    if (atu_find_map(ATU_ID_MSCP, &existing_map_entry) == SILIBS_SUCCESS)
    {
        found_existing_mapping = true;
        mapped_addr = existing_map_entry.mscp_start_address + (addr - existing_map_entry.ap_base_address);
        FpFwCliPrint("Found existing mapping for address 0x%llX\n", addr);
    }

    atu_entry_attr_t atu_root_attr = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT};
    atu_map_entry_t map_entry = {.ap_base_address = ALIGN_DOWN(addr, ATU_PAGE_SIZE),
                                 .mscp_start_address = 0,
                                 .mscp_end_address = ALIGN_UP(size * sizeof(uint32_t), ATU_PAGE_SIZE) - 1,
                                 .attribute = {.as_uint32 = atu_root_attr.as_uint32}};

    if (!found_existing_mapping)
    {
        if (atu_map(ATU_ID_MSCP, &map_entry) != 0)
        {
            FpFwCliPrint("Failed to map ATU for address 0x%llX\n", addr);
            return CLI_ERROR;
        }
        mapped_addr = map_entry.mscp_start_address + (addr - map_entry.ap_base_address);
    }

    for (int i = 0; i < size; i++)
    {
        uint32_t data = *((volatile uint32_t*)(mapped_addr + i * sizeof(uint32_t)));
        FpFwCliPrint("0x%08llX: 0x%08X\n", addr + i * sizeof(uint32_t), data);
    }

    if (!found_existing_mapping)
    {
        atu_unmap(ATU_ID_MSCP, &map_entry);
    }
    return CLI_SUCCESS;
}

// Command to write memory
static FPFW_CLI_STATUS ap_mem_write(int argc, const char* argv[])
{
    if (argc != 3)
    {
        FpFwCliPrint("Usage: ap_mem_write <addr> <32-bit data>\n");
        return CLI_ERROR;
    }

    uint64_t addr = strtoull(argv[1], NULL, 0);
    uint32_t data = strtoul(argv[2], NULL, 0);

    // if an existing mapping is not found, create a new one
    atu_map_entry_t existing_map_entry = {
        .ap_base_address = addr,
        .mscp_start_address = 0,
        .mscp_end_address = 0,
        .attribute = {.as_uint32 = 0} // Default attributes
    };

    bool found_existing_mapping = false;
    uintptr_t mapped_addr;
    if (atu_find_map(ATU_ID_MSCP, &existing_map_entry) == SILIBS_SUCCESS)
    {
        found_existing_mapping = true;
        mapped_addr = existing_map_entry.mscp_start_address + (addr - existing_map_entry.ap_base_address);
        FpFwCliPrint("Found existing mapping for address 0x%llX\n", addr);
    }

    atu_entry_attr_t atu_root_attr = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT};
    atu_map_entry_t map_entry = {.ap_base_address = ALIGN_DOWN(addr, ATU_PAGE_SIZE),
                                 .mscp_start_address = 0,
                                 .mscp_end_address = ALIGN_UP(sizeof(uint32_t), ATU_PAGE_SIZE) - 1,
                                 .attribute = {.as_uint32 = atu_root_attr.as_uint32}};

    if (!found_existing_mapping)
    {
        if (atu_map(ATU_ID_MSCP, &map_entry) != 0)
        {
            FpFwCliPrint("Failed to map ATU for address 0x%llX\n", addr);
            return CLI_ERROR;
        }
        mapped_addr = map_entry.mscp_start_address + (addr - map_entry.ap_base_address);
    }

    *((volatile uint32_t*)mapped_addr) = data;
    FpFwCliPrint("Wrote 0x%08X to 0x%08llX\n", data, addr);

    if (!found_existing_mapping)
    {
        atu_unmap(ATU_ID_MSCP, &map_entry);
    }
    return CLI_SUCCESS;
}

// Register commands
void cli_mem_init(void)
{
    FpFwCliRegisterTable(s_mem_cmd_list, FPFW_ARRAY_SIZE(s_mem_cmd_list));
    FPFW_DBGPRINT_INFO("CLI Memory commands initialized\n");
}