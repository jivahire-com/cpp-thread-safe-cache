//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_power_log.c
 * Source file with implementations of power log CLI commands.
 */

/*------------- Includes -----------------*/
#include "DfwkCommon.h" // for PDFWK_ASYNC_REQUEST_HEADER, _DFWK_ASYNC_REQU...
#include "power_dfwk.h" // for (anonymous), ppower_service_cli_req...
#include "power_init.h"
#include "power_log.h"

#include <FpFwUtils.h>           // for FPFW_UNUSED
#include <cli_power_common.h>    // for ST_COUNT
#include <cli_power_interface.h> // for power_cli_sub_command_dictionary_el...
#include <cli_power_log.h>
#include <corebits.h>
#include <ddrss_reserved_regions.h>
#include <inttypes.h>
#include <power_runconfig.h>
#include <stdio.h> // for printf

/*-- Symbolic Constant Macros (defines) --*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
static void power_log_list(ppower_service_cli_request_t p_cli_request);
static void power_log_ddr(ppower_service_cli_request_t p_cli_request);
static void power_log_mask(ppower_service_cli_request_t p_cli_request);
/*-- Declarations (Statics and globals) --*/
// clang-format off
const power_cli_sub_command_dictionary_element_t power_cli_log_sub_command_dictionary[] = {
    {"list",                NULL,               POWER_IF_CMD_LOG_DUMP},
    {"ddr",              NULL,             POWER_IF_CMD_LOG_DDR},
    {"mask",             NULL,            POWER_IF_CMD_LOG_MASK},

};
const uint32_t length_log_commands_dictionary =
    sizeof(power_cli_log_sub_command_dictionary) / sizeof(power_cli_sub_command_dictionary_element_t);
/*-------------- Functions ---------------*/
power_if_cmd_t cli_power_log_get_cmd_id(char* sub_command)
{
    if (sub_command == NULL)
    {
        return POWER_IF_CMD_UNKNOWN;
    }

    /* Parse dictionary for subcommand and fetch corresponding power_ext_if_cmd_id */
    for (uint32_t index = 0; index < length_log_commands_dictionary; index++)
    {
        if (strcmp(sub_command, power_cli_log_sub_command_dictionary[index].sub_command) == 0)
        {
            return power_cli_log_sub_command_dictionary[index].power_ext_if_cmd_id;
        }
    }

    /* If sub-command is not found in the dictionary */
    return POWER_IF_CMD_UNKNOWN;
}
typedef void (*print_power_entry_t)(unsigned entry_idx, power_log_entry_t *entry);
static void print_power_entry(unsigned entry_idx, power_log_entry_t *entry)
{
    printf(" %03d [0x%08" PRIx32 "%08" PRIx32 "] ", 
       entry_idx, 
       (uint32_t)(entry->timestamp >> 32),  // high level shift 32 bit
       (uint32_t)(entry->timestamp));      // low level  32 bit
    if (power_log_has_cores(entry->type)) {
        printf(COREBITS_FMT_STR, COREBITS_FMT_DATA(entry->payload.cores));
    } else {
        printf(COREBITS_FMT_STR, COREBITS_FMT_DATA(ALLCORES));
    }
    printf(" - %s\n", power_log_string(entry->type, &entry->payload));
}

static void print_log_entry(unsigned entry_idx, print_power_entry_t print_func)
{
    power_log_entry_t *entry = NULL;
    power_log_data_t *log = get_instance();
    entry                    = &(log->entries[entry_idx]);
    if (entry->type == POWER_LOG_INVALID_TYPE) {
        return;
    }
    print_func(entry_idx, entry);
}

static void power_log_base(print_power_entry_t print_func)
{
    power_log_data_t *log = get_instance();

    unsigned entry       = log->oldest_entry;
    unsigned max_entries = log->max_entries;
    unsigned last_entry  = log->last_entry;
    while (entry != last_entry) {
        print_log_entry(entry, print_func);
        entry += 1;
        entry %= max_entries;
    }
    print_log_entry(entry, print_func);
}

// "pwr log list" command
static void power_log_list(ppower_service_cli_request_t p_cli_request)
{
    UNUSED(p_cli_request);
    power_log_base(print_power_entry);
}

// "pwr log useddr" command
void power_log_ddr(ppower_service_cli_request_t p_cli_request)
{

    uint16_t value = p_cli_request->pwrset_sub_command_args.minupdate_val;
    power_log_use_ddr((value != 0));

    printf("  log use ddr: %s\n", (value != 0) ? "true" : "false");
    uint64_t start_addr = POWER_LOG_RESERVATION_BASE;
    uint64_t end_addr   = POWER_LOG_RESERVATION_END - 1;
    printf("   DDR range: 0x%" PRIx32 "%08" PRIx32 "--0x%" PRIx32 "%08" PRIx32 "\n",
       (uint32_t)(start_addr >> 32),
       (uint32_t)start_addr,
       (uint32_t)(end_addr >> 32),
       (uint32_t)end_addr);
}

// "pwr log mask" command
void power_log_mask(ppower_service_cli_request_t p_cli_request)
{
    power_log_data_t *log = get_instance();

        log->mask = p_cli_request->pwrset_sub_command_args.minupdate_val;

   printf("  mask: %08" PRIx32 "\n", log->mask);
}
void cli_power_log_async_print(PDFWK_ASYNC_REQUEST_HEADER p_request, void* completion_context)
{
    FPFW_UNUSED(p_request);
    FPFW_UNUSED(completion_context);
    ppower_service_cli_request_t p_cli_request = (ppower_service_cli_request_t)p_request;
    p_cli_request->header = *p_request;

    switch (p_cli_request->power_ext_if_cmd_id)
    {
        case POWER_IF_CMD_LOG_DUMP:
            power_log_list(p_cli_request);        
            break;
            
        case POWER_IF_CMD_LOG_DDR: 
            power_log_ddr(p_cli_request);                        
            break;
            
        case POWER_IF_CMD_LOG_MASK:
            power_log_mask(p_cli_request);                            
            break;

        default:
           break;
    }    
    printf("Power Logging yet to be implemented\n");
}
