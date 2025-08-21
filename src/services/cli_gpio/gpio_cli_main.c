//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file GPIO cli commands
 */

/*------------- Includes -----------------*/
#include <DfwkClient.h> // for DfwkAsyncRequestInitialize, DfwkAs...
#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <FpFwCli.h>    // for FPFW_CLI_COMMAND, FpFwCliRegisterTable
#include <FpFwUtils.h>  // for FPFW_ARRAY_SIZE, FPFW_UNUSED
#include <fpfw_cfg_mgr.h>
#include <gpio.h>     // for gpio_request_t
#include <gpio_lib.h> // for gpio_set_int_enable, gpio_get_int...
#include <idsw_kng.h>
#include <kng_error.h> // for KNG_FAILED, KNG_SUCCESS
#include <stdlib.h>    // for atoi
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef struct
{
    pgpio_interface_t gpio_iface;
    bool isr_registered;
} gpio_cli_isr_context_t, *pgpio_cli_isr_context_t;

/*--------- Function Prototypes ----------*/
static FPFW_CLI_STATUS gpio_cli_set_int_enable(int argc, const char** pp_argv);
static FPFW_CLI_STATUS gpio_cli_get_int_enable(int argc, const char** pp_argv);
static FPFW_CLI_STATUS gpio_cli_set_dir(int argc, const char** pp_argv);
static FPFW_CLI_STATUS gpio_cli_get_dir(int argc, const char** pp_argv);
static FPFW_CLI_STATUS gpio_cli_set_pin(int argc, const char** pp_argv);
static FPFW_CLI_STATUS gpio_cli_get_pin(int argc, const char** pp_argv);
static FPFW_CLI_STATUS gpio_cli_register_isr(int argc, const char** pp_argv);
static FPFW_CLI_STATUS gpio_cli_restore(int argc, const char** pp_argv);
static FPFW_CLI_STATUS gpio_cli_set_uart_afm(int argc, const char** pp_argv);
static FPFW_CLI_STATUS gpio_cli_set_uart_die_config(int argc, const char** pp_argv);
static FPFW_CLI_STATUS gpio_cli_get_uart_ownership(int argc, const char** pp_argv);
static FPFW_CLI_STATUS gpio_cli_get_uart_afm(int argc, const char** pp_argv);

static FPFW_CLI_STATUS print_uart_afm_mux_table(int argc, const char** pp_argv);

/*-- Declarations (Statics and globals) --*/
static FPFW_CLI_COMMAND s_gpio_cmd_list[] = {
    {NULL_LIST_ENTRY, "gpio", "set_int_enable", gpio_cli_set_int_enable, "Set interrupt enable state", "Usage: set_int_enable <ctrl_id> <pin_id> <enable [1:0]>"},
    {NULL_LIST_ENTRY, "gpio", "get_int_enable", gpio_cli_get_int_enable, "Get current interrupt enable state", "Usage: get_int_enable <ctrl_id> <pin_id>"},
    {NULL_LIST_ENTRY, "gpio", "set_dir", gpio_cli_set_dir, "Set GPIO Direction", "Usage: set_dir <ctrl_id> <pin_id> <dir [0|1]>"},
    {NULL_LIST_ENTRY, "gpio", "get_dir", gpio_cli_get_dir, "Get current GPIO Direction", "Usage: get_dir <ctrl_id> <pin_id>"},
    {NULL_LIST_ENTRY, "gpio", "set_pin", gpio_cli_set_pin, "Set GPIO pin output", "Usage: set_pin <ctrl_id> <pin_id> <level [0|1]>"},
    {NULL_LIST_ENTRY, "gpio", "get_pin", gpio_cli_get_pin, "Get GPIO pin input", "Usage: get_pin <ctrl_id> <pin_id>"},
    {NULL_LIST_ENTRY, "gpio", "register_isr", gpio_cli_register_isr, "Register single GPIO interrupt callback", "Usage: register_isr <ctrl_id> <pin_id>"},
    {NULL_LIST_ENTRY, "gpio", "restore", gpio_cli_restore, "Restore initial GPIO config", "Usage: restore"},
    {NULL_LIST_ENTRY, "gpio", "uart_afm", gpio_cli_set_uart_afm, "Set UART AFM", "Usage: uart_afm <afm_u0> <afm_u1> <afm_u2> <afm_u3>"},
    {NULL_LIST_ENTRY, "gpio", "uart_die", gpio_cli_set_uart_die_config, "Set UART die configuration", "Usage: uart_die <die_id_u1> <die_id_u2>"},
    {NULL_LIST_ENTRY, "gpio", "get_uart_ownership", gpio_cli_get_uart_ownership, "Get UART ownership", "Usage: get_uart_ownership"},
    {NULL_LIST_ENTRY, "gpio", "get_uart_afm", gpio_cli_get_uart_afm, "Get UART AFM", "Usage: get_uart_afm"},
    {NULL_LIST_ENTRY, "gpio", "get_mux_table", print_uart_afm_mux_table, "Print UART Mux Table for this Die", "Usage: print_mux_table"} // End of command list
};

// Cache the GPIO configuration table to restore the GPIO configuration
static pgpio_interface_t s_gpio_iface = NULL;

/*------------- Functions ----------------*/
void gpio_cli_init(pgpio_interface_t gpio_iface)
{
    if (gpio_iface == NULL)
    {
        FpFwCliPrint("Failed GPIO CLI Init, GPIO interface is NULL\n");
        FPFW_RUNTIME_ASSERT(false);
        return;
    }
    s_gpio_iface = gpio_iface;

    //! register the GPIO commands
    FpFwCliRegisterTable(s_gpio_cmd_list, FPFW_ARRAY_SIZE(s_gpio_cmd_list));
}

static PLACED_CODE FPFW_CLI_STATUS gpio_cli_restore(int argc, const char** pp_argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(pp_argv);

    int slibs_ret = gpio_init(s_gpio_iface->Device->ConfigTable->gpio_config_table,
                              s_gpio_iface->Device->ConfigTable->table_size);
    if (SILIBS_SUCCESS == slibs_ret)
    {
        FpFwCliPrint("GPIO config restored\n");
    }
    else
    {
        FpFwCliPrint("Failed to restore GPIO config - 0x%08x\n", slibs_ret);
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS gpio_cli_set_int_enable(int argc, const char** pp_argv)
{
    uint32_t gpio_ctrl_id = 0;
    uint32_t gpio_pin_id = 0;
    uint32_t enable = 0;

    if (argc == 4)
    {
        gpio_ctrl_id = atoi(pp_argv[1]);
        gpio_pin_id = atoi(pp_argv[2]);
        enable = atoi(pp_argv[3]) == 0 ? 0 : 1;
    }
    else
    {
        FpFwCliPrint("Failed: Invalid Args\n");
        return CLI_ERROR;
    }

    int slibs_ret = gpio_set_interrupt_enable(GPIO_CTRL_PIN_ID(gpio_ctrl_id, gpio_pin_id), enable);
    if (SILIBS_SUCCESS == slibs_ret)
    {
        FpFwCliPrint("Set %d / %d GPIO interrupt: %s\n", gpio_ctrl_id, gpio_pin_id, enable ? "Enable" : "Disable");
    }
    else
    {
        FpFwCliPrint("Failed to set interrupt enable - 0x%08x\n", slibs_ret);
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS gpio_cli_get_int_enable(int argc, const char** pp_argv)
{
    uint32_t gpio_ctrl_id = 0;
    uint32_t gpio_pin_id = 0;
    uint32_t enable = 0;

    if (argc == 3)
    {
        gpio_ctrl_id = atoi(pp_argv[1]);
        gpio_pin_id = atoi(pp_argv[2]);
    }
    else
    {
        FpFwCliPrint("Failed: Invalid Args\n");
        return CLI_ERROR;
    }

    int slibs_ret = gpio_get_interrupt_enable(GPIO_CTRL_PIN_ID(gpio_ctrl_id, gpio_pin_id), &enable);
    if (SILIBS_SUCCESS == slibs_ret)
    {
        FpFwCliPrint("Get %d / %d GPIO interrupt: %s\n", gpio_ctrl_id, gpio_pin_id, enable ? "Enable" : "Disable");
    }
    else
    {
        FpFwCliPrint("Failed to set interrupt enable - 0x%08x\n", slibs_ret);
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS gpio_cli_set_dir(int argc, const char** pp_argv)
{
    uint32_t gpio_ctrl_id = 0;
    uint32_t gpio_pin_id = 0;
    uint32_t dir = 0;

    if (argc == 4)
    {
        gpio_ctrl_id = atoi(pp_argv[1]);
        gpio_pin_id = atoi(pp_argv[2]);
        dir = atoi(pp_argv[3]);

        switch (dir)
        {
        case 0:
            dir = GPIO_DIR_IN;
            break;
        case 1:
            dir = GPIO_DIR_OUT;
            break;
        default:
            FpFwCliPrint("Failed: Invalid Direction\n");
            return CLI_ERROR;
        }
    }
    else
    {
        FpFwCliPrint("Failed: Invalid Args\n");
        return CLI_ERROR;
    }

    int slibs_ret = gpio_set_dir(GPIO_CTRL_PIN_ID(gpio_ctrl_id, gpio_pin_id), dir);
    if (SILIBS_SUCCESS == slibs_ret)
    {
        FpFwCliPrint("Set %d / %d GPIO Direction: %s\n", gpio_ctrl_id, gpio_pin_id, dir == 0 ? "GPIO_DIR_IN" : "GPIO_DIR_OUT");
    }
    else
    {
        FpFwCliPrint("Failed to set GPIO direction - 0x%08x\n", slibs_ret);
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS gpio_cli_get_dir(int argc, const char** pp_argv)
{
    uint32_t gpio_ctrl_id = 0;
    uint32_t gpio_pin_id = 0;
    uint32_t dir = 0;

    if (argc == 3)
    {
        gpio_ctrl_id = atoi(pp_argv[1]);
        gpio_pin_id = atoi(pp_argv[2]);
    }
    else
    {
        FpFwCliPrint("Failed: Invalid Args\n");
        return CLI_ERROR;
    }

    int slibs_ret = gpio_get_dir(GPIO_CTRL_PIN_ID(gpio_ctrl_id, gpio_pin_id), &dir);
    if (SILIBS_SUCCESS == slibs_ret)
    {
        FpFwCliPrint("Get %d / %d GPIO Direction: %s\n", gpio_ctrl_id, gpio_pin_id, dir == 0 ? "GPIO_DIR_IN" : "GPIO_DIR_OUT");
    }
    else
    {
        FpFwCliPrint("Failed to get GPIO direction - 0x%08x\n", slibs_ret);
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS gpio_cli_set_pin(int argc, const char** pp_argv)
{
    uint32_t gpio_ctrl_id = 0;
    uint32_t gpio_pin_id = 0;
    uint32_t level = 0;

    if (argc == 4)
    {
        gpio_ctrl_id = atoi(pp_argv[1]);
        gpio_pin_id = atoi(pp_argv[2]);
        level = atoi(pp_argv[3]) == 0 ? 0 : 1;
    }
    else
    {
        FpFwCliPrint("Failed: Invalid Args\n");
        return CLI_ERROR;
    }

    int slibs_ret = gpio_set_output(GPIO_CTRL_PIN_ID(gpio_ctrl_id, gpio_pin_id), level);
    if (SILIBS_SUCCESS == slibs_ret)
    {
        FpFwCliPrint("Set %d / %d GPIO Output: %d\n", gpio_ctrl_id, gpio_pin_id, level);
    }
    else
    {
        FpFwCliPrint("Failed to set GPIO output - 0x%08x\n", slibs_ret);
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS gpio_cli_get_pin(int argc, const char** pp_argv)
{
    uint32_t gpio_ctrl_id = 0;
    uint32_t gpio_pin_id = 0;
    uint32_t level = 0;

    if (argc == 3)
    {
        gpio_ctrl_id = atoi(pp_argv[1]);
        gpio_pin_id = atoi(pp_argv[2]);
    }
    else
    {
        FpFwCliPrint("Failed: Invalid Args\n");
        return CLI_ERROR;
    }

    int slibs_ret = gpio_get_input(GPIO_CTRL_PIN_ID(gpio_ctrl_id, gpio_pin_id), &level);
    if (SILIBS_SUCCESS == slibs_ret)
    {
        FpFwCliPrint("Get %d / %d GPIO Input: %d\n", gpio_ctrl_id, gpio_pin_id, level);
    }
    else
    {
        FpFwCliPrint("Failed to get GPIO input - 0x%08x\n", slibs_ret);
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

static void gpio_cli_isr_callback(PDFWK_ASYNC_REQUEST_HEADER request, void* completion_context)
{
    pgpio_cli_isr_context_t isr_context = (pgpio_cli_isr_context_t)completion_context;

    pgpio_request_t gpio_request = (pgpio_request_t)request;

    DfwkClientInterfaceClose(&isr_context->gpio_iface->Header);
    isr_context->isr_registered = false;

    FpFwCliPrint("GPIO ISR Callback: Status: 0x%08x, CtrlID: %d, PinID: %d, Level: %d\n",
                 gpio_request->status,
                 GET_GPIO_CTRL_ID(gpio_request->gpio_pin_id),
                 GET_GPIO_PIN_ID(gpio_request->gpio_pin_id),
                 gpio_request->level);
}

static PLACED_CODE FPFW_CLI_STATUS gpio_cli_register_isr(int argc, const char** pp_argv)
{
    static gpio_cli_isr_context_t isr_context = {.isr_registered = false};
    uint32_t gpio_ctrl_id = 0;
    uint32_t gpio_pin_id = 0;

    if (argc == 3)
    {
        gpio_ctrl_id = atoi(pp_argv[1]);
        gpio_pin_id = atoi(pp_argv[2]);
    }
    else
    {
        FpFwCliPrint("Failed: Invalid Args\n");
        return CLI_ERROR;
    }

    if (isr_context.isr_registered)
    {
        FpFwCliPrint("Failed: ISR already registered\n");
        return CLI_ERROR;
    }

    static gpio_request_t isr_request = {0};

    DfwkClientInterfaceOpen(&s_gpio_iface->Header);
    isr_context.gpio_iface = s_gpio_iface;

    uint32_t status = gpio_register_deferred_isr(s_gpio_iface, // GPIO interface
                                                 &isr_request, // GPIO request to register callback
                                                 GPIO_CTRL_PIN_ID(gpio_ctrl_id, gpio_pin_id), // GPIO controller and pin ID
                                                 gpio_cli_isr_callback, // Callback function
                                                 &isr_context);         // Callback Context

    if (KNG_FAILED(status))
    {
        FpFwCliPrint("Failed - 0x%08x\n", status);
        return CLI_ERROR;
    }

    isr_context.isr_registered = true;

    return CLI_SUCCESS;
}

/* ------------------------ */
/* GPIO UART AFM Management */
/* ------------------------ */
static const char* uart_afm_map_table[4][4] = {
    {"HSP", "MCP", "CDED", "KMP"}, // UART0 for Die 0, UART 5 for Die 1
    {"SCP", "SDM", "CDED", "KMP"}, // UART1
    {"MCP", "SCP", "SDM", "APS"},  // UART2
    {"APNS", "APS", "NA", "NA"}    // UART3
};

static void print_uart_afm_config(const uart_afm_cfg_t* afm_knobs, const char* note)
{
    FpFwCliPrint("\nUART AFM Config Die %d", idsw_get_die_id());
    if (note != NULL)
    {
        FpFwCliPrint(" (%s) ", note);
    }
    FpFwCliPrint("\n");

    for (int i = 0; i < 4; i++)
    {
        FpFwCliPrint("UART%d: %d(%s)\n", i, afm_knobs->uart_afm[i], uart_afm_map_table[i][afm_knobs->uart_afm[i]]);
    }
}

static FPFW_CLI_STATUS print_uart_afm_mux_table(int argc, const char** pp_argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(pp_argv);

    const char* uart_afm_mux_table =
        "|      | UART0/5 | UART1 | UART2 | UART3 |\n"
        "|------|---------|-------|-------|-------|\n"
        "|HSP   |   0     |       |       |       |\n"
        "|SCP   |         |   0   |   1   |       |\n"
        "|MCP   |   1     |       |   0   |       |\n"
        "|SDM   |         |   1   |   2   |       |\n"
        "|CDED  |   2     |   2   |       |       |\n"
        "|KMP   |   3     |   3   |       |       |\n"
        "|APNS  |         |       |       |   0   |\n"
        "|APS   |         |       |   3   |   1   |\n"
        "------------------------------------------\n"
        "Note: UART0 is for Die 0, the same mapping is applicable for UART5 for Die 1\n"
        "Note: APNS and APS are only available for Die 0\n";

    FpFwCliPrint("\nUART AFM Mux Table:\n");
    FpFwCliPrint("%s\n", uart_afm_mux_table);

    return CLI_SUCCESS;
}

/* This command retrieves the current UART AFM settings for the particular Die */
static PLACED_CODE FPFW_CLI_STATUS gpio_cli_get_uart_afm(int argc, const char** pp_argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(pp_argv);

    uint8_t die_id = idsw_get_die_id();
    uart_afm_cfg_t afm_knobs = {0};
    uint32_t status = gpio_get_uart_afmsel(die_id, &afm_knobs);

    if (KNG_FAILED(status))
    {
        FpFwCliPrint("Fail: UART AFM Read (0x%08x)\n", status);
        return CLI_ERROR;
    }

    print_uart_afm_config(&afm_knobs, NULL);

    return CLI_SUCCESS;
}

/* This command updates the UART AFM Config for that particular die.
 * It supports both complete configuration (when all AFM mux values are entered),
 * as well as partial configuration (when only specific AFM mux values are updated).
 * To use partial configuration, use the x mark to indicate that there is no change.
 * For example, uart_afm x x 3 x will update only UART3. The other UARTs will remain unchanged.
 * Meanwhile, the command uart_afm 1 0 1 0 will set UART0 to 1, UART1 to 0, UART2 to 1, and UART3 to 0.
 * Can be run from both MCP and SCP, takes into account any overrides performed via by T32)
 */
static PLACED_CODE FPFW_CLI_STATUS gpio_cli_set_uart_afm(int argc, const char** pp_argv)
{
    uart_afm_cfg_t afm_knobs = {0};
    uint8_t die_id = idsw_get_die_id();

    uint32_t status = gpio_get_uart_afmsel(die_id, &afm_knobs);
    if (KNG_FAILED(status))
    {
        FpFwCliPrint("Fail: UART AFM Read (0x%08x)\n", status);
        return CLI_ERROR;
    }

    print_uart_afm_config(&afm_knobs, "Current Map");

    if (argc != 5)
    {
        FpFwCliPrint("Fail: Invalid Args\n");
        return CLI_ERROR;
    }

    uint8_t uart_afm = 0;
    for (int i = 1; i <= argc - 1; i++)
    {
        if (pp_argv[i][0] >= '0' && pp_argv[i][0] <= '3')
        {
            uart_afm = atoi(pp_argv[i]);
            afm_knobs.uart_afm[i - 1] = uart_afm;
        }
    }

    print_uart_afm_config(&afm_knobs, "Requested Update");

    status = gpio_override_uart_afmsel(die_id, &afm_knobs);
    if (KNG_FAILED(status))
    {
        FpFwCliPrint("Fail: AFM Override (0x%08x)\n", status);
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

/* ----------------------------------------------------- */
/* GPIO UART Die Management - For UART 1 and UART 2 Only */
/* ----------------------------------------------------- */

/**
 * This command returns the UART die ownership for the current die.
 * It will indicate whether UART1 and UART2 are owned by this die.
 */
static PLACED_CODE FPFW_CLI_STATUS gpio_cli_get_uart_ownership(int argc, const char** pp_argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(pp_argv);
    bool uart_ownership[2] = {false, false};

    int status = gpio_get_shared_uart_ownership(idsw_get_die_id(), &uart_ownership[0], &uart_ownership[1]);
    if (status != SILIBS_SUCCESS)
    {
        FpFwCliPrint("Fail: UART Owner Read (0x%08x)\n", status);
        return CLI_ERROR;
    }

    for (int uart_num = 1; uart_num <= 2; uart_num++)
    {
        FpFwCliPrint("Die %d owns UART %d? %s\n", idsw_get_die_id(), uart_num, uart_ownership[uart_num - 1] ? "Yes" : "No");
    }

    return CLI_SUCCESS;
}

/*
 * This command sets the UART die configuration UARt 1 and 2 on the current Die.
 * For example, the command uart_die 0 1 will set the Die ID for UART1 to 0 and for UART2 to 1.
 *
 * Important Note: This command will need another corresponding command on the other die as well to set IE OE
 * bits. For the above example, the user will have to run the command uart_die 0 1 on the other die as well.
 *
 * This command also supports partial configuration. For example, the command
 * uart_die 0 x will set the Die ID for UART1 to 0 and leave the Die ID for UART2 unchanged.
 */
static PLACED_CODE FPFW_CLI_STATUS gpio_cli_set_uart_die_config(int argc, const char** pp_argv)
{
    uint8_t this_die_id = idsw_get_die_id();
    uint8_t other_die_id = (this_die_id == 0) ? 1 : 0;

    bool uart_ownership[2] = {false, false};

    uint32_t status = gpio_get_shared_uart_ownership(idsw_get_die_id(), &uart_ownership[0], &uart_ownership[1]);
    if (status != SILIBS_SUCCESS)
    {
        FpFwCliPrint("Fail: UART Owner Read (0x%08x)\n", status);
        return CLI_ERROR;
    }

    uart_die_cfg_t die_cfg_knobs;
    die_cfg_knobs.uart1_die_id = (uart_ownership[0] ? this_die_id : other_die_id);
    die_cfg_knobs.uart2_die_id = (uart_ownership[1] ? this_die_id : other_die_id);

    FpFwCliPrint("\nCurrent UART1 Owner: Die%d, UART2 Owner: Die%d\n",
                 die_cfg_knobs.uart1_die_id,
                 die_cfg_knobs.uart2_die_id);

    if (argc != 3)
    {
        FpFwCliPrint("Fail: Invalid Args\n");
        return CLI_ERROR;
    }

    if (pp_argv[1][0] == '0' || pp_argv[1][0] == '1')
    {
        die_cfg_knobs.uart1_die_id = atoi(pp_argv[1]);
    }

    if (pp_argv[2][0] == '0' || pp_argv[2][0] == '1')
    {
        die_cfg_knobs.uart2_die_id = atoi(pp_argv[2]);
    }

    status = gpio_configure_shared_uart(idsw_get_die_id(), die_cfg_knobs.uart1_die_id, die_cfg_knobs.uart2_die_id);
    if (status != SILIBS_SUCCESS)
    {
        FpFwCliPrint("Fail: UART Die Cfg Update (0x%08x)\n", status);
        return CLI_ERROR;
    }

    status = gpio_get_shared_uart_ownership(idsw_get_die_id(), &uart_ownership[0], &uart_ownership[1]);
    if (status != SILIBS_SUCCESS)
    {
        FpFwCliPrint("Fail: UART Owner Read (0x%08x)\n", status);
        return CLI_ERROR;
    }

    die_cfg_knobs.uart1_die_id = (uart_ownership[0] ? this_die_id : other_die_id);
    die_cfg_knobs.uart2_die_id = (uart_ownership[1] ? this_die_id : other_die_id);

    FpFwCliPrint("Updated UART1 Die ID: %d, UART2 Die ID: %d\n", die_cfg_knobs.uart1_die_id, die_cfg_knobs.uart2_die_id);

    FpFwCliPrint("\nFor the update to be complete, please run the same command on the other die as well\n");

    // TODO: Send a message to the other Die to update the UART configuration
    return CLI_SUCCESS;
}
