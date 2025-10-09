//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file GPIO cli commands
 */

/*-------------------------------- Includes ---------------------------------*/
#include <DfwkClient.h> // for DfwkAsyncRequestInitialize, DfwkAs...
#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <FpFwCli.h>    // for FPFW_CLI_COMMAND, FpFwCliRegisterTable
#include <gpio.h>       // for gpio_request_t
#include <gpio_lib.h>   // for gpio_set_int_enable, gpio_get_int...
#include <kng_error.h>  // for KNG_FAILED, KNG_SUCCESS
#include <stdlib.h>     // for atoi
#include <utils.h>      // for PLACED_CODE

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/
typedef struct
{
    pgpio_interface_t gpio_iface;
    bool isr_registered;
} gpio_cli_isr_context_t, *pgpio_cli_isr_context_t;

/*--------------------------- Function Prototypes ---------------------------*/
static FPFW_CLI_STATUS gpio_cli_set_int_enable(int argc, const char** pp_argv);
static FPFW_CLI_STATUS gpio_cli_get_int_enable(int argc, const char** pp_argv);
static FPFW_CLI_STATUS gpio_cli_set_dir(int argc, const char** pp_argv);
static FPFW_CLI_STATUS gpio_cli_get_dir(int argc, const char** pp_argv);
static FPFW_CLI_STATUS gpio_cli_set_pin(int argc, const char** pp_argv);
static FPFW_CLI_STATUS gpio_cli_get_pin(int argc, const char** pp_argv);
static FPFW_CLI_STATUS gpio_cli_register_isr(int argc, const char** pp_argv);
static FPFW_CLI_STATUS gpio_cli_restore(int argc, const char** pp_argv);

/*------------------- Declarations (Statics and globals) --------------------*/
static FPFW_CLI_COMMAND s_gpio_cmd_list[] = {
    {NULL_LIST_ENTRY, "gpio", "set_int_enable", gpio_cli_set_int_enable, "Set interrupt enable state", "Usage: set_int_enable <ctrl_id> <pin_id> <enable [1:0]>"},
    {NULL_LIST_ENTRY, "gpio", "get_int_enable", gpio_cli_get_int_enable, "Get current interrupt enable state", "Usage: get_int_enable <ctrl_id> <pin_id>"},
    {NULL_LIST_ENTRY, "gpio", "set_dir", gpio_cli_set_dir, "Set GPIO Direction", "Usage: set_dir <ctrl_id> <pin_id> <dir [0|1]>"},
    {NULL_LIST_ENTRY, "gpio", "get_dir", gpio_cli_get_dir, "Get current GPIO Direction", "Usage: get_dir <ctrl_id> <pin_id>"},
    {NULL_LIST_ENTRY, "gpio", "set_pin", gpio_cli_set_pin, "Set GPIO pin output", "Usage: set_pin <ctrl_id> <pin_id> <level [0|1]>"},
    {NULL_LIST_ENTRY, "gpio", "get_pin", gpio_cli_get_pin, "Get GPIO pin input", "Usage: get_pin <ctrl_id> <pin_id>"},
    {NULL_LIST_ENTRY, "gpio", "register_isr", gpio_cli_register_isr, "Register single GPIO interrupt callback", "Usage: register_isr <ctrl_id> <pin_id>"},
    {NULL_LIST_ENTRY, "gpio", "restore", gpio_cli_restore, "Restore initial GPIO config", "Usage: restore"}};

// Cache the GPIO configuration table to restore the GPIO configuration
static pgpio_interface_t s_gpio_iface = NULL;

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/
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

/*----------------------------- Global Functions ----------------------------*/

FPFW_CLI_STATUS gpio_cli_init(pgpio_interface_t gpio_iface)
{
    if (gpio_iface == NULL)
    {
        FpFwCliPrint("Failed GPIO CLI Init, GPIO interface is NULL\n");
        FPFW_RUNTIME_ASSERT(false);
        return CLI_ERROR;
    }
    s_gpio_iface = gpio_iface;

    //! register the GPIO commands for the local die
    FpFwCliRegisterTable(s_gpio_cmd_list, FPFW_ARRAY_SIZE(s_gpio_cmd_list));

    return CLI_SUCCESS;
}
