//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file warm_start_cli.c
 * This file contains the implementation of the Warm Start CLIs RD, WR & DISP
 */

/*------------- Includes -----------------*/
#include <DfwkCommon.h> // for DfwkAsyncRequestInitialize, PDFW...
#include <FpFwCli.h>
#include <FpFwLinkedList.h>
#include <FpFwUtils.h>
#include <bug_check.h> // for BUG_ASSERT
#include <startup_shutdown.h>
#include <startup_shutdown_init.h>
#include <startup_shutdown_ssi.h> // for ssi_shutdown_type_t, ssi_startup_s...
#include <stdint.h>
#include <stdlib.h> // for atoi
#include <string.h> // for strcmp
#include <warm_start.h>
#include <warm_start_cli.h> // for avs_client_init_completion_routine, pavs...
#include <warm_start_i.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MAX_DATA_COUNT 16

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

static FPFW_CLI_STATUS ws_write_cli(int argc, const char** argv);
static FPFW_CLI_STATUS ws_read_cli(int argc, const char** argv);
static FPFW_CLI_STATUS ws_disp_cli(int argc, const char** argv);
static FPFW_CLI_STATUS ws_reset(int argc, const char** argv);
void boot_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context);

/*-- Declarations (Statics and globals) --*/

extern ws_data_list_t* p_ws_list;

static FPFW_CLI_COMMAND warm_start_cli_list[] = {
    {NULL_LIST_ENTRY, "warm_start", "wsrd", ws_read_cli, "Warm Start Read Data", "syntax: wsrd <id>\n"},
    {NULL_LIST_ENTRY, "warm_start", "wswr", ws_write_cli, "Warm Start Write Data", "syntax: wswr <id> <byte>..\n"},
    {NULL_LIST_ENTRY, "warm_start", "wsdisp", ws_disp_cli, "Warm Start Display Data", "syntax: wsdisp\n"},
    {NULL_LIST_ENTRY, "warm_start", "wsreset", ws_reset, "Warm Start Cli to perform Reset", "syntax: wsreset [cold, subsys, warm, shutdown]"},
};

char* ws_reason_strings[] = {"Reset Reason unknown\n", "Power On Reset\n", "Pre AP core boot Warm Reset\n", "Post AP core boot Warm Reset\n"};

char* id_strings[] = WARM_START_ID_STRINGS;

static sos_interface_t sos_interface;

/*------------- Functions ----------------*/

/**
 *  CLI command for writing warm data
 *
 *  @param argc
 *      Number of arguments provided
 *
 *  @param argv
 *      pointer array for the argument strings
 *
 *  @return
 *      CLI_SUCCESS (always)
 */
static FPFW_CLI_STATUS ws_write_cli(int argc, const char** argv)
{
    uint32_t id;
    uint8_t data[MAX_DATA_COUNT];

    if (argc < 3)
    {
        FpFwCliPrint("Incorrect args\n");
        FpFwCliPrint("Syntax: wswr <ID> <space separated bytes data>\n");
        return CLI_SUCCESS;
    }

    if (argc - 2 > MAX_DATA_COUNT)
    {
        FpFwCliPrint("Maximum byte allowed is %d\n", MAX_DATA_COUNT);
        return CLI_SUCCESS;
    }

    id = atoi(argv[1]) & 0xFF;

    if (id < WARM_START_ID_LAST)
    {
        // Load data from other arguments
        for (int32_t i = 2; (i < argc) && ((i - 3) < MAX_DATA_COUNT); i++)
        {
            data[i - 2] = atoi(argv[i]) & 0xFF;
        }

        ws_data_put((mod_ws_data_id_t)id, data, argc - 2);
    }
    else
    {
        FpFwCliPrint("Invalid ID %d\n", id);
    }

    return CLI_SUCCESS;
}

/**
 *  CLI command for reading warm data
 *
 *  @param argc
 *      Number of arguments provided
 *
 *  @param argv
 *      pointer array for the argument strings
 *
 *  @return
 *      CLI_SUCCESS (always)
 */
static FPFW_CLI_STATUS ws_read_cli(int argc, const char** argv)
{
    uint32_t count;
    uint32_t id;
    uint8_t* p_data;

    if (argc < 2)
    {
        FpFwCliPrint("Incorrect parameters\n");
        FpFwCliPrint("Syntax: wsrd <ID>\n");
        return CLI_SUCCESS;
    }

    id = atoi(argv[1]) & 0xFF;

    if (id < WARM_START_ID_LAST)
    {
        p_data = ws_data_get(id, &count);

        if (p_data == NULL)
        {
            FpFwCliPrint("Data for %d doesn't exist\n", id);
            return CLI_SUCCESS;
        }

        FpFwCliPrint("Data for %d ", id);
        if (id < WARM_START_ID_LAST)
        {
            FpFwCliPrint("(%s)", id_strings[id]);
        }
        for (uint32_t i = 0; i < count; i++)
        {
            if ((i % MAX_DATA_COUNT) == 0)
            {
                FpFwCliPrint("\n");
                FpFwCliPrint("%04x: ", i);
            }
            FpFwCliPrint("%02x ", p_data[i]);
        }

        FpFwCliPrint("\n");
    }
    else
    {
        FpFwCliPrint("Invalid ID %d\n", id);
    }

    return CLI_SUCCESS;
}

/**
 *  CLI command for displaying the warm data manifest
 *
 *  @param argc
 *      Number of arguments provided
 *
 *  @param argv
 *      pointer array for the argument strings
 *
 *  @return
 *      CLI_SUCCESS (always)
 */
static FPFW_CLI_STATUS ws_disp_cli(int argc, const char** argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(argv);
    BUG_ASSERT(p_ws_list != NULL);
    ws_data_entry_t* p_entry = &p_ws_list->entry;
    uint32_t total_reserved = 0;
    uint32_t total_fragments = 0;
    uint32_t total_taken = 0;
    uint32_t total_entries = 0;
    void* p_last = NULL;

    if (p_ws_list->magic_id != WARM_START_MAGIC_ID)
    {
        FpFwCliPrint("No warm data present\n");
        return CLI_SUCCESS;
    }

    // Display entry data
    while (p_entry != NULL)
    {
        if (p_entry->id < WARM_START_ID_LAST)
        {
            FpFwCliPrint("ID: %s (%d), Size: %d, Location: 0x%08X \n",
                         id_strings[p_entry->id],
                         p_entry->id,
                         p_entry->size,
                         &p_entry->data);
        }
        else
        {
            FpFwCliPrint("ID: %d, Size: %d: Location: 0x%08X \n", p_entry->id, p_entry->size, &p_entry->data);
        }

        // Keep track of things
        if (p_entry->id == WARM_START_ID_RESERVED)
        {
            total_reserved += p_entry->size;
            if (p_entry->p_next != NULL)
            {
                total_fragments += p_entry->size;
            }
        }
        else
        {
            total_taken += p_entry->size;
        }
        total_entries++;

        p_last = p_entry;
        p_entry = p_entry->p_next;
    }

    FpFwCliPrint("Number of Entries: %d\n", total_entries);
    FpFwCliPrint("Space not used: %d\n", total_reserved);
    FpFwCliPrint("Space used: %d\n", total_taken);
    FpFwCliPrint("Fragmented Spaced: %d\n", total_fragments);
    FpFwCliPrint("Expected Memory Taken: %d\n",
                 ((total_entries - 1) * (sizeof(ws_data_entry_t) - 4)) + sizeof(ws_data_list_t) + total_taken + total_fragments);
    FpFwCliPrint("Actual Memory Taken: %d\n",
                 ((uint32_t)((uint32_t)p_last - (uint32_t)p_ws_list)) + sizeof(ws_data_entry_t));

    return CLI_SUCCESS;
}

void cold_boot_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context)
{
    FPFW_UNUSED(p_completion_context);
    FPFW_UNUSED(request);
    BUG_ASSERT(false);
}

void subsys_boot_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context)
{
    FPFW_UNUSED(p_completion_context);
    printf("Request (%x)\n is completed", (uintptr_t)request);
    printf("Subsys Boot Completed");
}

void shutdown_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context)
{
    FPFW_UNUSED(p_completion_context);
    FPFW_UNUSED(request);
    BUG_ASSERT(false);
}

void warm_boot_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context)
{
    FPFW_UNUSED(p_completion_context);
    printf("Request (%x)\n is completed", (uintptr_t)request);
    printf("AP Warm Boot Completed");
}

/**
 *  CLI command for performing a warm reset
 *
 *  @param argc
 *      Number of arguments provided
 *
 *  @param argv
 *      pointer array for the argument strings
 *
 *  @return
 *      If reset is available on platform, this will never return
 */
static FPFW_CLI_STATUS ws_reset(int argc, const char** argv)
{
    // send synchronous and asynchronous startup stage start requests
    static startup_shutdown_request_t shutdown_request;
    DfwkAsyncRequestInitialize((void*)&shutdown_request.header, sizeof(shutdown_request));

    WS_LOG_INFO("Attempting to perform %s reset", argv[1]);

    /*NOTE: These CLIs will currently only print a cli message
    as shutdown_handler is targeted to complete by 1.0_extended
    https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1824038
    */

    // Detect which type of reset to perform
    if (argc == 2)
    {
        if (strcmp(argv[1], "cold") == 0)
        {
            sos_shutdown((void*)&sos_interface, &shutdown_request, COLD_RESET, cold_boot_completion, NULL);
        }
        else if (strcmp(argv[1], "subsys") == 0)
        {
            sos_shutdown((void*)&sos_interface, &shutdown_request, MSCP_SUBSYS_RESET, subsys_boot_completion, NULL);
        }
        else if (strcmp(argv[1], "shutdown") == 0)
        {
            sos_shutdown((void*)&sos_interface, &shutdown_request, SHUTDOWN, shutdown_completion, NULL);
        }
        else if (strcmp(argv[1], "warm") == 0)
        {
            sos_shutdown((void*)&sos_interface, &shutdown_request, AP_WARM_RESET, warm_boot_completion, NULL);
        }
        else
        {
            WS_LOG_INFO("Invalid parameter %s\n", argv[1]);
            return CLI_SUCCESS;
        }
    }
    else
    {
        WS_LOG_INFO("Invalid Args, Check usage\n");
        return CLI_SUCCESS;
    }

    return CLI_SUCCESS;
}

void warm_start_cli_init(psos_device_t p_device)
{
    sos_interface_init(p_device, &sos_interface, true);

    FpFwCliRegisterTable(warm_start_cli_list, FPFW_ARRAY_SIZE(warm_start_cli_list));

    FpFwCliPrint("Warm Start CLI Init done\n");
}
