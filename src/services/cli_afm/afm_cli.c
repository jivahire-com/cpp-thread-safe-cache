//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file CLI commands for AFM and Die config on local and remote die (MCP)
 */

/*-------------------------------- Includes ---------------------------------*/
#include <FpFwCli.h>              // for FPFW_CLI_COMMAND, FpFwCliRegisterTable
#include <bug_check.h>            // for BUG_CHECK, BUG_ASSERT
#include <fpfw_init.h>            // for fpfw_init_get_handle
#include <gpio.h>                 // for gpio_request_t
#include <icc_mhu.h>              // for icc_mhu_header_t
#include <icc_platform_defines.h> // for D2D_MBOX_FIFO_DEPTH, HSP_MBOX_FIFO_DEPTH
#include <idsw_kng.h>             // for idsw_get_platform_sdv, PLATFORM_KINGS...
#include <stdlib.h>               // for atoi
#include <string.h>               // for memset
#include <utils.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

#define ARGS_COUNT_CHECK(n)                                                 \
    if (argc != (n))                                                        \
    {                                                                       \
        FpFwCliPrint("FAIL: Invalid Args (Expected %d vs. %d)\n", argc, n); \
        return CLI_ERROR;                                                   \
    }

#define GPIO_REMOTE_ARG_INVALID 0xFF

/*-------------------------------- Typedefs ---------------------------------*/
/* Enum for GPIO CLI Commands supported on the remote die */
typedef enum
{
    REMOTE_GPIO_CLI_CMD_AFM_UPDATE_REQ = 0x1,
    REMOTE_GPIO_CLI_CMD_AFM_UPDATE_RSP = 0x2,

    /* Die update is uniform on both dies, parameters are sanitized on the local die.
    Confirmation from remote die not necessary.
    Configs will be out of sync only if registers are updated manually with T32 */
    REMOTE_GPIO_CLI_CMD_DIE_UPDATE_REQ = 0x3,
    REMOTE_GPIO_CLI_CMD_MAX
} remote_gpio_cli_cmd_t;

/* Remote GPIO CLI Request Structure. The remote gpio cli supports up to 4 arguments,
 * enough for afm update as well as die update for Kingsgate. This fits inside the D2D mailbox */
typedef struct
{
    icc_mhu_header_t header;
    remote_gpio_cli_cmd_t gpio_cli_cmd;
    FPFW_CLI_STATUS cmd_status;
    uint8_t args[4];
} remote_gpio_cli_req_t, *p_remote_gpio_cli_req_t;

/* Check for the request size, must fit into die2die mbox*/
static_assert(sizeof(remote_gpio_cli_req_t) >= 16, "size(remote_gpio_cli_req_t) must be >= 16B");
static_assert(sizeof(remote_gpio_cli_req_t) <= 64, "size(remote_gpio_cli_req_t) must be <= 64B");

/*--------------------------- Function Prototypes ---------------------------*/
static FPFW_CLI_STATUS gpio_cli_set_uart_afm(int argc, const char** pp_argv);
static FPFW_CLI_STATUS gpio_cli_set_uart_die_config(int argc, const char** pp_argv);
static FPFW_CLI_STATUS gpio_cli_get_uart_ownership(int argc, const char** pp_argv);
static FPFW_CLI_STATUS gpio_cli_get_uart_afm(int argc, const char** pp_argv);
static FPFW_CLI_STATUS print_uart_afm_mux_table(int argc, const char** pp_argv);

static FPFW_CLI_STATUS d2d_gpio_cli_msg_send(p_remote_gpio_cli_req_t p_payload);
static void gpio_cli_d2d_recv_cb(void* context, size_t output_size_bytes, fpfw_status_t status);

/*------------------- Declarations (Statics and globals) --------------------*/
/* Retaining all commands in the gpio menu to support any legacy code. To confirm with FSE before moving to an `afm` menu */
/* These are commands that can be run on any config - single die or dual die */
static FPFW_CLI_COMMAND s_afm_cmd_list[] = {
    {NULL_LIST_ENTRY, "afm", "uart_afm", gpio_cli_set_uart_afm, "Set UART AFM", "uart_afm <die_num> <afm_u0> <afm_u1> <afm_u2> <afm_u3>"},
    {NULL_LIST_ENTRY, "afm", "get_uart_ownership", gpio_cli_get_uart_ownership, "Get UART ownership", "get_uart_ownership"},
    {NULL_LIST_ENTRY, "afm", "get_uart_afm", gpio_cli_get_uart_afm, "Get UART AFM", "get_uart_afm"},
    {NULL_LIST_ENTRY, "afm", "get_mux_table", print_uart_afm_mux_table, "Print UART Mux Table for this Die", "print_mux_table"}};

/* These are commands that require dual die config to run */
static FPFW_CLI_COMMAND s_afm_dual_die_cmd_list[] = {
    {NULL_LIST_ENTRY, "afm", "uart_die", gpio_cli_set_uart_die_config, "Set UART die config", "uart_die <die_id_u1> <die_id_u2>"}};

static bool d2d_send_in_progress = false;

static fpfw_icc_base_ctx_t* icc_base_rmss_d2d_mbx_ctx = NULL; //! D2D mailbox context       //! Remote GPIO CLI recv message

const uint16_t d2d_gpio_cli_msg_size = sizeof(remote_gpio_cli_req_t) - sizeof(icc_mhu_header_t);

static uint8_t this_die_id;
static uint8_t other_die_id;
/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/* ------------------------ */
/* GPIO UART AFM Management */
/* ------------------------ */

typedef enum
{
    E_CORE_HSP = 0,
    E_CORE_MCP = 1,
    E_CORE_SCP = 2,
    E_CORE_SDM = 3,
    E_CORE_CDED = 4,
    E_CORE_KMP = 5,
    E_CORE_APS = 6,
    E_CORE_MCTP = 7,
    E_CORE_NA = 8,
    E_CORE_NUM
} e_cores_t;

static const char* core_name[E_CORE_NUM] = {
    "HSP",  // 0
    "MCP",  // 1
    "SCP",  // 2
    "SDM",  // 3
    "CDED", // 4
    "KMP",  // 5
    "APS",  // 6
    "MCTP", // 7
    "NA",   // 8
};

static const e_cores_t uart_afm_d0_map_table[4][4] = {
    {E_CORE_HSP, E_CORE_MCTP, E_CORE_CDED, E_CORE_KMP}, // UART0
    {E_CORE_SCP, E_CORE_SDM, E_CORE_CDED, E_CORE_KMP},  // UART1
    {E_CORE_MCTP, E_CORE_SCP, E_CORE_SDM, E_CORE_APS},  // UART2
    {E_CORE_MCP, E_CORE_APS, E_CORE_NA, E_CORE_NA}      // UART3
};
static const e_cores_t uart_afm_d1_map_table[4][4] = {
    {E_CORE_HSP, E_CORE_MCP, E_CORE_CDED, E_CORE_KMP}, // UART5
    {E_CORE_SCP, E_CORE_SDM, E_CORE_CDED, E_CORE_KMP}, // UART1
    {E_CORE_MCP, E_CORE_SCP, E_CORE_SDM, E_CORE_NA},   // UART2
    {E_CORE_NA, E_CORE_NA, E_CORE_NA, E_CORE_NA}       // UART3
};

static PLACED_CODE void print_uart_afm_config(const uart_afm_cfg_t* afm_knobs, uint8_t die_id, const char* note)
{
    FpFwCliPrint("\nUART AFM Config Die %d", die_id);
    if (note != NULL)
    {
        FpFwCliPrint(" (%s) ", note);
    }
    FpFwCliPrint("\n");

    for (unsigned int i = 0; i < FPFW_ARRAY_SIZE(uart_afm_d0_map_table[0]); i++)
    {
        if (afm_knobs->uart_afm[i] == GPIO_REMOTE_ARG_INVALID)
        {
            FpFwCliPrint("UART%d: No Change\n", i);
            continue;
        }

        FpFwCliPrint("UART%d: %d(%s)\n",
                     i,
                     afm_knobs->uart_afm[i],
                     (die_id == 0 ? core_name[uart_afm_d0_map_table[i][afm_knobs->uart_afm[i]]]
                                  : core_name[uart_afm_d1_map_table[i][afm_knobs->uart_afm[i]]]));
    }
}

static PLACED_CODE FPFW_CLI_STATUS print_uart_afm_mux_table(int argc, const char** pp_argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(pp_argv);

    const char* uart_afm_mux_table = "|Die0|UART0|UART1|UART2|UART3|\n"
                                     "|----|-----|-----|-----|-----|\n"
                                     "|HSP |  0  |     |     |     |\n"
                                     "|SCP |     |  0  |  1  |     |\n"
                                     "|MCTP|  1  |     |  0  |     |\n"
                                     "|MCP |     |     |     |  0  |\n"
                                     "|SDM |     |  1  |  2  |     |\n"
                                     "|CDED|  2  |  2  |     |     |\n"
                                     "|KMP |  3  |  3  |     |     |\n"
                                     "|APS |     |     |  3  |  1  |\n"
                                     "------------------------------\n"
                                     "\nAPNS/WinDbg Die 0 is persistently mapped to UART4\n\n"
                                     "|Die1|UART5|UART1|UART2|\n"
                                     "|----|-----|-----|-----|\n"
                                     "|HSP |  0  |     |     |\n"
                                     "|SCP |     |  0  |  1  |\n"
                                     "|MCP |  1  |     |  0  |\n"
                                     "|SDM |     |  1  |  2  |\n"
                                     "|CDED|  2  |  2  |     |\n"
                                     "|KMP |  3  |  3  |     |\n"
                                     "------------------------\n";

    FpFwCliPrint("\nUART AFM Mux Table:\n");
    FpFwCliPrint("%s\n", uart_afm_mux_table);

    return CLI_SUCCESS;
}

/* This command retrieves the current UART AFM settings for the particular Die */
static PLACED_CODE FPFW_CLI_STATUS gpio_cli_get_uart_afm(int argc, const char** pp_argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(pp_argv);

    uart_afm_cfg_t afm_knobs = {0};
    uint32_t status = gpio_get_uart_afmsel(this_die_id, &afm_knobs);

    if (KNG_FAILED(status))
    {
        FpFwCliPrint("Fail: UART AFM Read (0x%08x)\n", status);
        return CLI_ERROR;
    }

    print_uart_afm_config(&afm_knobs, this_die_id, NULL);

    return CLI_SUCCESS;
}

/* This command updates the UART AFM Config for a given die (0 or 1).
 * It supports both complete configuration (when all AFM mux values are entered),
 * as well as partial configuration (when only specific AFM mux values are updated).
 * To use partial configuration, use the x mark to indicate that there is no change.
 * For example, uart_afm x x 3 x will update only UART3. The other UARTs will remain unchanged.
 * Meanwhile, the command uart_afm 1 0 1 0 will set UART0 to 1, UART1 to 0, UART2 to 1, and UART3 to 0.
 * Can be run from either MCP Die 0 or Die 1, takes into account any overrides performed via by T32)
 */
static PLACED_CODE FPFW_CLI_STATUS gpio_set_uart_afm_local(uart_afm_cfg_t* p_afm_update_req)
{
    uart_afm_cfg_t afm_knobs = {0};

    uint32_t status = gpio_get_uart_afmsel(this_die_id, &afm_knobs);
    if (KNG_FAILED(status))
    {
        FpFwCliPrint("Fail: AFM Read: 0x%08x\n", status);
        return CLI_ERROR;
    }

    print_uart_afm_config(&afm_knobs, this_die_id, "Current Map");

    for (unsigned int i = 0; i < (FPFW_ARRAY_SIZE(p_afm_update_req->uart_afm)); i++)
    {
        if (p_afm_update_req->uart_afm[i] <= 3)
        {
            afm_knobs.uart_afm[i] = p_afm_update_req->uart_afm[i];
        }
    }

    print_uart_afm_config(&afm_knobs, this_die_id, "New Map");

    status = gpio_override_uart_afmsel(this_die_id, &afm_knobs);
    if (KNG_FAILED(status))
    {
        FpFwCliPrint("Fail: AFM Update: 0x%08x\n", status);
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS gpio_set_uart_afm_remote(uart_afm_cfg_t* p_afm_update_req)
{

    remote_gpio_cli_req_t d2d_request = {.header = {.msg_header = {.command = RMSS_D2D_MAILBOX_MSG_GPIO_CLI_CMD,
                                                                   .payload_size = d2d_gpio_cli_msg_size}},
                                         .gpio_cli_cmd = REMOTE_GPIO_CLI_CMD_AFM_UPDATE_REQ};

    uart_afm_cfg_t afm_knobs = {0};

    for (unsigned int i = 0; i < (FPFW_ARRAY_SIZE(p_afm_update_req->uart_afm)); i++)
    {
        afm_knobs.uart_afm[i] = p_afm_update_req->uart_afm[i];
        d2d_request.args[i] = p_afm_update_req->uart_afm[i];
    }

    print_uart_afm_config(&afm_knobs, other_die_id, "Requested Map");

    return d2d_gpio_cli_msg_send(&d2d_request);
}

static PLACED_CODE FPFW_CLI_STATUS gpio_cli_set_uart_afm(int argc, const char** pp_argv)
{
    /* Check that there are at 6 input arguments */
    ARGS_COUNT_CHECK(6);

    /* Check for a valid die index in pp_argv[1] */
    uint8_t die_id;
    if (pp_argv[1][0] == '0')
    {
        die_id = 0;
    }
    else if (pp_argv[1][0] == '1')
    {
        /* Check if die2die mailbox is available */
        if (icc_base_rmss_d2d_mbx_ctx == NULL)
        {
            FpFwCliPrint("Die1 not supported, single die config\n");
            return CLI_ERROR;
        }
        die_id = 1;
    }
    else
    {
        FpFwCliPrint("Invalid Die ID: %s\n", pp_argv[1]);
        return CLI_ERROR;
    }

    /* Read for UART AFM values */
    uart_afm_cfg_t afm_knobs = {0};
    for (int i = 2; i < argc; i++)
    {
        if (pp_argv[i][0] >= '0' && pp_argv[i][0] <= '3')
        {
            uint8_t uart_afm = atoi(pp_argv[i]);
            afm_knobs.uart_afm[i - 2] = uart_afm;
        }
        else
        {
            afm_knobs.uart_afm[i - 2] = GPIO_REMOTE_ARG_INVALID;
        }
    }

    if (die_id == this_die_id)
    {
        return gpio_set_uart_afm_local(&afm_knobs);
    }

    return gpio_set_uart_afm_remote(&afm_knobs);
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
 * This command also supports partial configuration. For example, the command
 * uart_die 0 x will set the Die ID for UART1 to 0 and leave the Die ID for UART2 unchanged.
 */
static PLACED_CODE FPFW_CLI_STATUS gpio_cli_set_uart_die_config(int argc, const char** pp_argv)
{
    bool uart_ownership[2] = {false, false};

    uint32_t status = gpio_get_shared_uart_ownership(idsw_get_die_id(), &uart_ownership[0], &uart_ownership[1]);
    if (status != SILIBS_SUCCESS)
    {
        FpFwCliPrint("Fail: UART Owner Read (0x%08x)\n", status);
        return CLI_ERROR;
    }

    uart_die_cfg_t die_cfg;
    die_cfg.uart1_die_id = (uart_ownership[0] ? this_die_id : other_die_id);
    die_cfg.uart2_die_id = (uart_ownership[1] ? this_die_id : other_die_id);

    FpFwCliPrint("\nCurrent Owners UART1: Die%d, UART2: Die%d\n", die_cfg.uart1_die_id, die_cfg.uart2_die_id);

    if (argc != 3)
    {
        FpFwCliPrint("Fail: Invalid Args\n");
        return CLI_ERROR;
    }

    remote_gpio_cli_req_t d2d_request = {.header = {.msg_header = {.command = RMSS_D2D_MAILBOX_MSG_GPIO_CLI_CMD,
                                                                   .payload_size = d2d_gpio_cli_msg_size}},
                                         .gpio_cli_cmd = REMOTE_GPIO_CLI_CMD_DIE_UPDATE_REQ,
                                         .args[0] = GPIO_REMOTE_ARG_INVALID,
                                         .args[1] = GPIO_REMOTE_ARG_INVALID};

    if (pp_argv[1][0] == '0' || pp_argv[1][0] == '1')
    {
        die_cfg.uart1_die_id = atoi(pp_argv[1]);
    }

    if (pp_argv[2][0] == '0' || pp_argv[2][0] == '1')
    {
        die_cfg.uart2_die_id = atoi(pp_argv[2]);
    }

    status = gpio_configure_shared_uart(idsw_get_die_id(), die_cfg.uart1_die_id, die_cfg.uart2_die_id);
    if (status != SILIBS_SUCCESS)
    {
        FpFwCliPrint("Fail: UART Die Cfg Update (0x%08x)\n", status);
        return CLI_ERROR;
    }

    FpFwCliPrint("Updated local die config. Updating remote die config\n");
    d2d_request.args[0] = die_cfg.uart1_die_id;
    d2d_request.args[1] = die_cfg.uart2_die_id;
    d2d_gpio_cli_msg_send(&d2d_request);

    status = gpio_get_shared_uart_ownership(idsw_get_die_id(), &uart_ownership[0], &uart_ownership[1]);
    if (status != SILIBS_SUCCESS)
    {
        FpFwCliPrint("Fail: UART Owner Read (0x%08x)\n", status);
        return CLI_ERROR;
    }

    die_cfg.uart1_die_id = (uart_ownership[0] ? this_die_id : other_die_id);
    die_cfg.uart2_die_id = (uart_ownership[1] ? this_die_id : other_die_id);

    FpFwCliPrint("Updated Owners UART1: Die%d, UART2: Die%d\n", die_cfg.uart1_die_id, die_cfg.uart2_die_id);

    return CLI_SUCCESS;
}

/* ------------------------------------------------------- */
/* Setup Die2Die infra needed for remote GPIO UART CLI     */
/* ------------------------------------------------------- */
static PLACED_CODE void d2d_gpio_cli_msg_send_rsp(FPFW_CLI_STATUS cmd_status)
{
    remote_gpio_cli_req_t d2d_response = {.header = {.msg_header = {.command = RMSS_D2D_MAILBOX_MSG_GPIO_CLI_CMD,
                                                                    .payload_size = d2d_gpio_cli_msg_size}},
                                          .gpio_cli_cmd = REMOTE_GPIO_CLI_CMD_AFM_UPDATE_RSP};
    d2d_response.cmd_status = cmd_status;
    d2d_gpio_cli_msg_send(&d2d_response);
}

static PLACED_CODE void gpio_cli_d2d_mbox_recv_subscribe()
{
    static remote_gpio_cli_req_t d2d_cli_recv_msg = {0};
    static fpfw_icc_base_recv_req_t d2d_recv_params = {0};

    //! Prepare recv request
    memset(&d2d_cli_recv_msg, 0, sizeof(d2d_cli_recv_msg));
    d2d_recv_params.payload_buffer = &d2d_cli_recv_msg;
    d2d_recv_params.buffer_size = sizeof(d2d_cli_recv_msg);
    d2d_recv_params.recv_cmd_code = RMSS_D2D_MAILBOX_MSG_GPIO_CLI_CMD;
    d2d_recv_params.cb = gpio_cli_d2d_recv_cb;
    d2d_recv_params.cb_ctx = &d2d_cli_recv_msg;

    // //! Register for recv thru icc base
    fpfw_status_t status = fpfw_icc_base_recv(icc_base_rmss_d2d_mbx_ctx, &d2d_recv_params);
    BUG_ASSERT_PARAM(status == FPFW_STATUS_SUCCESS, status, 0);
}

static PLACED_CODE void gpio_cli_d2d_recv_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);
    BUG_ASSERT_PARAM(status == FPFW_STATUS_SUCCESS, status, 0);

    p_remote_gpio_cli_req_t p_rx_gpio_cli_req = (p_remote_gpio_cli_req_t)(context);

    switch (p_rx_gpio_cli_req->gpio_cli_cmd)
    {
    case REMOTE_GPIO_CLI_CMD_AFM_UPDATE_REQ:
        /* Parameters are already sanitized while processing the CLI command.
        No additional checks/sanitization needed on the remote size */
        uart_afm_cfg_t afm_config = {0};
        for (unsigned int i = 0; i < FPFW_ARRAY_SIZE(afm_config.uart_afm); i++)
        {
            afm_config.uart_afm[i] = p_rx_gpio_cli_req->args[i];
        }

        FPFW_CLI_STATUS afm_status = gpio_set_uart_afm_local(&afm_config);
        d2d_gpio_cli_msg_send_rsp(afm_status);
        break;

    case REMOTE_GPIO_CLI_CMD_AFM_UPDATE_RSP:
        /* Print the result, mimic the print from ExecuteCommand */
        char* prnt_res_str = (p_rx_gpio_cli_req->cmd_status == CLI_SUCCESS) ? "Remote - Ok" : "Remote ERROR - Check AFM Config";
        FpFwCliPrint("%s\n", prnt_res_str);
        break;

    case REMOTE_GPIO_CLI_CMD_DIE_UPDATE_REQ:
        /* Parameters are already sanitized while processing the CLI command.
        No additional checks/sanitization needed on the remote side */
        gpio_configure_shared_uart(idsw_get_die_id(), p_rx_gpio_cli_req->args[0], p_rx_gpio_cli_req->args[1]);
        break;

    default:
        FpFwCliPrint("ERROR: Unknown GPIO CLI Cmd: %d\n", p_rx_gpio_cli_req->gpio_cli_cmd);
        break;
    }

    // Re-subscribe for next recv
    gpio_cli_d2d_mbox_recv_subscribe();
}

static PLACED_CODE void gpio_cli_remote_cmd_send_cb(void* context, fpfw_status_t status)
{
    FPFW_UNUSED(context);

    BUG_ASSERT_PARAM(status == FPFW_STATUS_SUCCESS, status, 0);

    d2d_send_in_progress = false;
}

static FPFW_CLI_STATUS d2d_gpio_cli_msg_send(p_remote_gpio_cli_req_t p_payload)
{
    if (d2d_send_in_progress)
    {
        FpFwCliPrint("[GPIO CLI] Busy, another D2D request in progress\n");
        return CLI_ERROR;
    }
    d2d_send_in_progress = true;

    /* Build the payload */
    static remote_gpio_cli_req_t tx_remote_gpio_cli_req_payload = {0};
    tx_remote_gpio_cli_req_payload = *p_payload;

    /* Build the ICC send request */
    static fpfw_icc_base_send_req_t remote_gpio_icc_send_req = {0};
    remote_gpio_icc_send_req.payload_buffer = &tx_remote_gpio_cli_req_payload;
    remote_gpio_icc_send_req.buffer_size = sizeof(tx_remote_gpio_cli_req_payload);
    remote_gpio_icc_send_req.cb = gpio_cli_remote_cmd_send_cb;
    remote_gpio_icc_send_req.cb_ctx = NULL;

    /* Send the ICC request */
    fpfw_status_t status = fpfw_icc_base_send(icc_base_rmss_d2d_mbx_ctx, &remote_gpio_icc_send_req);
    if (status != FPFW_ICC_BASE_STATUS_SUCCESS)
    {
        FpFwCliPrint("[GPIO CLI] Err: D2D send fail: 0x%08x\n", status);
        return (CLI_ERROR);
    }

    return CLI_SUCCESS;
}

/*----------------------------- Global Functions ----------------------------*/

PLACED_CODE void afm_cli_init(void)
{
    // Cache the die IDs
    this_die_id = idsw_get_die_id();
    other_die_id = (this_die_id == DIE_0) ? 1 : 0;

    //! register the common AFM commands
    FpFwCliRegisterTable(s_afm_cmd_list, FPFW_ARRAY_SIZE(s_afm_cmd_list));

    // Initialise the D2D interface
    icc_base_rmss_d2d_mbx_ctx = (fpfw_icc_base_ctx_t*)fpfw_init_get_handle("icc_die2die");
    if (icc_base_rmss_d2d_mbx_ctx == NULL)
    {
        FpFwCliPrint("[GPIO CLI] D2D Mbox not found. Skipping afm commands for dual die\n");
        return;
    }

    /* Register the commands for dual die */
    FpFwCliRegisterTable(s_afm_dual_die_cmd_list, FPFW_ARRAY_SIZE(s_afm_dual_die_cmd_list));

    gpio_cli_d2d_mbox_recv_subscribe();
}