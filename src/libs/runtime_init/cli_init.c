//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_init.c
 * Initialize & start cli textio interface
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FpFwCli.h>      // for FpFwCliInitialize, FpFwCliStart, FPFW_CLI_...
#include <fpfw_init.h>    // for fpfw_init_get_handle, FPFW_INIT_COMPONENT
#include <stdint.h>       // for uint8_t
#include <stdio.h>        // for printf, NULL, stdout
#include <textio_pl011.h> // for textio_pl011_device_interface_initialize

/*------- Symbolic Constant Macros (defines) ----------*/
#define CLI_COMMAND_LENGTH       (256)
#define CLI_COMMAND_HISTORY_LEN  (4)
#define CLI_COMMAND_HISTORY_SIZE (CLI_COMMAND_LENGTH * CLI_COMMAND_HISTORY_LEN)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(cli, FPFW_INIT_DEPENDENCIES("uart", "std_io", "debug_print"))
{
    fpfw_init_component_id_t uart_id = "uart";
    static uint8_t cli_cmd_history[CLI_COMMAND_HISTORY_SIZE];
    static FPFW_CLI_CONFIG cli_config = {.CommandHistory = cli_cmd_history,
                                         .CommandHistorySize = CLI_COMMAND_HISTORY_SIZE,
                                         .MaxCommandLength = CLI_COMMAND_LENGTH};

    //! Pl011 cli interfaces
    static textio_pl011_interface_t pl011_interface_cli = {0};

    //! Initialize a pl011 interface for cli
    textio_pl011_device_interface_initialize(fpfw_init_get_handle(uart_id), &pl011_interface_cli);

    //! Initialize cli lib
    cli_config.InputStream = &pl011_interface_cli.header;
    cli_config.OutputStream = stdout;
    FPFW_CLI_STATUS cli_result = FpFwCliInitialize(&cli_config);

    //! Start cli lib
    if (CLI_SUCCESS == cli_result)
    {
        FpFwCliStart();
        FPFW_DBGPRINT_INFO("CLI Started\n");
    }
    return (fpfw_init_result_t){cli_result, NULL};
}