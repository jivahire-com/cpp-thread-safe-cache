/**
 * @file uart_scp_init.c
 * Instantiates UART for the SCP
 */

/*------------- Includes -----------------*/
#include <FpFwCli.h>      // for FPFW_CLI_CONFIG, FpFwCliInitialize, FPFW_C...
#include <fpfw_init.h>    // for fpfw_init_get_handle, FPFW_INIT_COMPONENT
#include <stddef.h>       // for NULL
#include <stdint.h>       // for uint8_t
#include <stdio.h>        // for printf
#include <textio_pl011.h> // for textio_pl011_device_interface_initialize

/*------- Symbolic Constant Macros (defines) ----------*/
#define CLI_COMMAND_LENGTH       (128)
#define CLI_COMMAND_HISTORY_LEN  (4)
#define CLI_COMMAND_HISTORY_SIZE (CLI_COMMAND_LENGTH * CLI_COMMAND_HISTORY_LEN)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(cli, FPFW_INIT_DEPENDENCIES("uart", "std_io"))
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
        printf("CLI Initialized & Started\n");
    }
    return (fpfw_init_result_t){cli_result, NULL};
}