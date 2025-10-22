//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file SEL cli commands
 */

/*------------- Includes -----------------*/
#include <FpFwCli.h>   // for FPFW_CLI_COMMAND, FpFwCliRegisterTable
#include <FpFwUtils.h> // for FPFW_ARRAY_SIZE, FPFW_UNUSED
#include <kng_error.h> // for KNG_SUCCESS
#include <sel.h>       // for sel_event_record_t
#include <stdlib.h>    // for strtoul
#include <utils.h>     // for PLACED_CODE

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
static FPFW_CLI_STATUS sel_log_sel_event(int argc, const char** pp_argv);

/*-- Declarations (Statics and globals) --*/
static FPFW_CLI_COMMAND s_sel_cmd_list[] = {
    {NULL_LIST_ENTRY, "sel", "log", sel_log_sel_event, "Logs a SEL event", "log <id> <type> <generator_id> <rev> <sensor_type> <sensor_number> <event_dir_type> <event_data1> <event_data2> <event_data3>"},
};

/*------------- Functions ----------------*/
PLACED_CODE void sel_cli_init(void)
{
    //! register the sel commands
    FpFwCliRegisterTable(s_sel_cmd_list, FPFW_ARRAY_SIZE(s_sel_cmd_list));
}

static PLACED_CODE FPFW_CLI_STATUS sel_log_sel_event(int argc, const char** pp_argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(pp_argv);
    sel_event_record_t event_record;

    if (argc != 11)
    {
        FpFwCliPrint("Invalid usage\n");
        return CLI_ERROR;
    }

    event_record.record_id = (uint16_t)strtoul(pp_argv[1], NULL, 0);     // 1-2: SEL Record ID
    event_record.record_type = (uint8_t)strtoul(pp_argv[2], NULL, 0);    // 3: SEL Record Type
    event_record.timestamp = 0;                                          // 4-7: Timestamp - ToDo: Fill UTC
    event_record.generator_id = (uint16_t)strtoul(pp_argv[3], NULL, 0);  // 8-9: Generator ID
    event_record.evm_rev = (uint8_t)strtoul(pp_argv[4], NULL, 0);        // 10: EVM Revision
    event_record.sensor_type = (uint8_t)strtoul(pp_argv[5], NULL, 0);    // 11: Sensor Type
    event_record.sensor_number = (uint8_t)strtoul(pp_argv[6], NULL, 0);  // 12: Sensor Number
    event_record.event_dir_type = (uint8_t)strtoul(pp_argv[7], NULL, 0); // 13: Event Direction Type
    event_record.event_data[0] = (uint8_t)strtoul(pp_argv[8], NULL, 0);  // 14: Event Data 1
    event_record.event_data[1] = (uint8_t)strtoul(pp_argv[9], NULL, 0);  // 15: Event Data 2
    event_record.event_data[2] = (uint8_t)strtoul(pp_argv[10], NULL, 0); // 16: Event Data 3

    log_sel_event(&event_record);

    return CLI_SUCCESS;
}
