//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file cli_mem.c Memory access cli commands
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FpFwCli.h>
#include <FpFwUtils.h>     // for FPFW_ARRAY_SIZE, FPFW_UNUSED
#include <arm_intrinsic.h> // for __DSB on Windows builds (empty define)
#include <atu_lib.h>
#include <inttypes.h> // for PRIx64
#include <kng_soc_constants.h>
#include <mscp_error_domain.h>
#include <stdio.h>
#include <stdlib.h>

// clang-format off
#include <cmsis_m7.h>
#ifdef PLATFORM_CACHING_ENABLED
#include <cmsis_compiler.h>
#include <m-profile/armv7m_cachel1.h>
#endif
// clang-format on

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
static FPFW_CLI_STATUS ap_mem_read(int argc, const char* argv[]);
static FPFW_CLI_STATUS ap_mem_write(int argc, const char* argv[]);
static FPFW_CLI_STATUS local_mem_read(int argc, const char* argv[]);
static FPFW_CLI_STATUS local_mem_write(int argc, const char* argv[]);
static void invalidate_cacheable_address(uint32_t addr, uint32_t width);

/*-- Declarations (Statics and globals) --*/
static FPFW_CLI_COMMAND s_mem_cmd_list[] = {
    {NULL_LIST_ENTRY, "mem", "ap_mem_read", ap_mem_read, "Read AP memory", "Usage: ap_mem_read <size in 32-bit words> <addr>"},
    {NULL_LIST_ENTRY, "mem", "ap_mem_write", ap_mem_write, "Write AP memory", "Usage: ap_mem_write <addr> <32-bit data>"},
    {NULL_LIST_ENTRY, "mem", "readmem", local_mem_read, "Read Local memory", "Usage: readmem <addr> <width in bytes 1|2|4|8>\nWARNING: READING FROM ILLEGAL ADDRESSES CAN CRASH THE SYSTEM."},
    {NULL_LIST_ENTRY, "mem", "writemem", local_mem_write, "Write Local memory", "Usage: writemem <base_addr> <width in bytes 1|2|4|8> <value to write>\nWARNING: WRITING ILLEGAL ADDRESSES CAN CRASH THE SYSTEM."}};

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

static FPFW_CLI_STATUS local_mem_read(int argc, const char* argv[])
{
    /* Checking for the correct number of arguments. */
    if (argc != 3)
    {
        FpFwCliPrint("Usage: readmem <addr> <width in bytes 1|2|4|8>\n"
                     "WARNING: READING FROM ILLEGAL ADDRESSES CAN CRASH THE SYSTEM.\n");
        return CLI_ERROR;
    }

    uint64_t addr_ull = strtoull(argv[1], NULL, 0);

    if (addr_ull > 0xFFFFFFFFULL)
    {
        FpFwCliPrint("Usage: readmem <addr> <width in bytes 1|2|4|8>\n"
                     "ERROR: addr range must in 0x0 to 0xFFFFFFFF.\n");
        return CLI_ERROR;
    }

    /* Getting address of access. */
    uintptr_t addr = (uintptr_t)addr_ull;

    /* Getting width of access and making sure it is valid. */
    uint32_t width = strtoul(argv[2], NULL, 0);

    /* Invalidate D-cache before memory read */
    invalidate_cacheable_address((uint32_t)addr_ull, width);

    /* Switching based on width. */
    FpFwCliPrint("Value: 0x");
    switch (width)
    {
    case 1:
        /* No boundary restrictions on single byte accesses. */
        FpFwCliPrint("%02x", *((volatile uint8_t*)addr));
        break;
    case 2:
        if (addr % 2)
        {
            /* 16 bit accesses need to be aligned on 2 byte boundaries. */
            return CLI_ERROR;
        }
        FpFwCliPrint("%04x", *((volatile uint16_t*)addr));
        break;
    case 4:
        if (addr % 4)
        {
            /* 32 bit accesses need to be aligned to 4 byte boundaries. */
            return CLI_ERROR;
        }
        FpFwCliPrint("%08x", *((volatile uint32_t*)addr));
        break;
    case 8:
        if (addr % 4)
        {
            /* 64 bit accesses need to be aligned on 4 byte boundaries. */
            return CLI_ERROR;
        }
        FpFwCliPrint("%" PRIx64 "", *((volatile uint64_t*)addr));
        break;
    default:
        return CLI_ERROR;
    }

    FpFwCliPrint("\n");

    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS local_mem_write(int argc, const char* argv[])
{
    /* Checking for the correct number of arguments. */
    if (argc != 4)
    {
        FpFwCliPrint("Usage: writemem <base_addr> <width in bytes 1|2|4|8> <value to write>\n"
                     "WARNING: WRITING ILLEGAL ADDRESSES CAN CRASH THE SYSTEM.\n");
        return CLI_ERROR;
    }

    uint64_t addr_ull = strtoull(argv[1], NULL, 0);

    if (addr_ull > 0xFFFFFFFFULL)
    {
        FpFwCliPrint("Usage: writemem <base_addr> <width in bytes 1|2|4|8> <value to write>\n"
                     "ERROR: base_addr range must in 0x0 to 0xFFFFFFFF.\n");
        return CLI_ERROR;
    }

    /* Getting address of access. */
    uintptr_t base_addr = (uintptr_t)addr_ull;

    /* Getting width of access and making sure it is valid. */
    uint32_t width = strtoul(argv[2], NULL, 0);

    /* Switching based on width. */
    switch (width)
    {
    case 1:
        /* No boundary restrictions on single byte accesses. */
        *((volatile uint8_t*)base_addr) = (uint8_t)strtoul(argv[3], NULL, 0);
        break;
    case 2:
        if (base_addr % 2)
        {
            /* 16 bit accesses need to be aligned on 2 byte boundaries. */
            return CLI_ERROR;
        }
        *((volatile uint16_t*)base_addr) = (uint16_t)strtoul(argv[3], NULL, 0);
        break;
    case 4:
        if (base_addr % 4)
        {
            /* 32 bit accesses need to be aligned to 4 byte boundaries. */
            return CLI_ERROR;
        }
        *((volatile uint32_t*)base_addr) = (uint32_t)strtoul(argv[3], NULL, 0);
        break;
    case 8:
        if (base_addr % 4)
        {
            /* 64 bit accesses need to be aligned on 4 byte boundaries. */
            return CLI_ERROR;
        }
        *((volatile uint64_t*)base_addr) = (uint64_t)strtoull(argv[3], NULL, 0);
        break;
    default:
        return CLI_ERROR;
    }

    /* Invalidate D-cache line after memory write */
    invalidate_cacheable_address((uint32_t)addr_ull, width);

    return CLI_SUCCESS;
}

static void invalidate_cacheable_address(uint32_t addr, uint32_t width)
{
    __DSB();

    int size = (width > 4) ? 8 : 4;

    if (is_cached_space(addr))
    {
        /* Cache Coherence Consistency */
        SCB_InvalidateDCache_by_Addr((volatile uint32_t*)addr, size);
    }
}

// Register commands
void cli_mem_init(void)
{
    FpFwCliRegisterTable(s_mem_cmd_list, FPFW_ARRAY_SIZE(s_mem_cmd_list));
    FPFW_DBGPRINT_INFO("CLI Memory commands initialized\n");
}