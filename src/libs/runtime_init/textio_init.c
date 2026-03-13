//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file textio_init.c
 * Instantiates TextIO Interface from UART for MSCP
 */

/*------------- Includes -----------------*/

#include <DfwkThreadXHost.h> // for PDFWK_THREADX_HOST
#include <atu_api.h>         // for MSCP_ATU_AP_WINDOW_UART_X_BASE_ADDR
#include <boot_status.h>
#include <build_data.h>          // for BUILD_PC, BUILD_TIMESTAMP, GIT_BRANCH
#include <fpfw_cfg_mgr.h>        // for knobs
#include <fpfw_init.h>           // for FPFW_INIT_STATUS_SUCCESS, fpfw_init_get...
#include <idsw.h>                // for idsw_get_platform_sdv,
#include <idsw_kng.h>            // for PLATFORM_FPGA_LARGE
#include <interrupts.h>          // IWYU pragma: keep
#include <silibs_mcp_top_regs.h> // IWYU pragma: keep
#include <silibs_scp_top_regs.h> // IWYU pragma: keep
#include <stddef.h>              // for NULL
#include <stdio_textio.h>        // for stdio_textio_init
#include <textio_pl011.h>        // for textio_pl011_device_t, textio_pl011_dev...
#include <uart_pl011.h>          // for UART_PL011_PARITY_NONE, UART_PL011_STOP...
#include <utils.h>

/*------------- Typedefs -----------------*/
#define SVP_UART_APB_FREQUENCY  125000000U
#define FPGA_UART_APB_FREQUENCY 10000000U
#define SOC_UART_APB_FREQUENCY  250000000U

#define AP_NS_UART_APB_FREQUENCY_FPGA (10000000U)
#define AP_NS_UART_APB_FREQUENCY_SOC  (100000000U)

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
static uint32_t get_uart_apb_frequency(idsw_plat_id_t sdv_id)
{
    switch (sdv_id)
    {
    case PLATFORM_SVP_SIM:
    case PLATFORM_SVP_MIN_CONFIG_SIM:
        return SVP_UART_APB_FREQUENCY;
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        return FPGA_UART_APB_FREQUENCY;
    default:
        return SOC_UART_APB_FREQUENCY;
    }
}

static PLACED_CODE bool platform_supports_oob(idsw_plat_id_t sdv_id)
{
    switch (sdv_id)
    {
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
    case PLATFORM_RVP_EVT_SILICON:
        return true;
    default:
        return false;
    }
}

static PLACED_CODE void print_build_info()
{

    for (int i = 0; i < 64; i++)
    {
        CRITICAL_PRINT("-");
    }
    CRITICAL_PRINT("\n");
    CRITICAL_PRINT("Build Version: %d.%d.%d\n", (int)MAJOR_VERSION, (int)MINOR_VERSION, (int)PATCH_VERSION);
    CRITICAL_PRINT("Branch: %s\n", GIT_BRANCH);
    CRITICAL_PRINT("Commit: %s\n", GIT_COMMIT_SHA);
    CRITICAL_PRINT("Build Timestamp: %s\n", BUILD_TIMESTAMP);
    CRITICAL_PRINT("Build PC: %s\n", BUILD_PC);

    /* MSCP Build Info */
    CRITICAL_PRINT("MSCP tag " SCP_DESCRIBE "\n");
    CRITICAL_PRINT("MSCP Commit " GIT_COMMIT_SHA "\n");

    /* Silibs Info */
    CRITICAL_PRINT("Silibs tag " SILIBS_DESCRIBE "\n");
    CRITICAL_PRINT("Silibs Commit " SILIBS_LAST_COMMIT_DESCRIBE "\n");

    /* SDM Info */
    CRITICAL_PRINT("SDM tag " SDM_DESCRIBE "\n");
    CRITICAL_PRINT("SDM Commit " SDM_LAST_COMMIT_DESCRIBE "\n");

    for (int i = 0; i < 64; i++)
    {
        CRITICAL_PRINT("-");
    }
    CRITICAL_PRINT("\n");
}

// TODO: The UART initialization component is now dependent on the configuration manager. To resolve
//       early boot stage debugging via UART see ado: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2856417
PLACED_CODE FPFW_INIT_COMPONENT(uart, FPFW_INIT_DEPENDENCIES("dfwk", "nvic", "systick_upd", "atu_svc", "cfg_mgr", "boot_stat"))
{
    fpfw_init_component_id_t dfwk_id = "dfwk";
    static textio_pl011_config_t pl011_config = {
        .base_address = UART_BASE_ADDR,
        .vuart_base_address = VUART_BASE_ADDR, // Base address for virtual UART, if applicable
        .interrupt = UART_IRQ,
        .vuart_interrupt = VUART_IRQ, // Interrupt for virtual UART, if applicable
        .baud_rate = 115200,
        .wlen = UART_PL011_WLEN_8,
        .stop_bits = UART_PL011_STOP_BITS_1,
        .parity = UART_PL011_PARITY_NONE,
        .config_type = TEXTIO_PL011_CONFIG_TYPE_INTERRUPT,
        .is_vuart_enabled = true,
    };

    idsw_plat_id_t sdv_id = idsw_get_platform_sdv();

    // Set clock frequency from APB clock
    pl011_config.clk_freq = get_uart_apb_frequency(sdv_id);

    // Reassign MCP Die 0 CLI if:
    // - MCP UART reassignment config knob is enabled
    // - The core is the MCP
    // - The die is die 0
    // - The platform supports oob (and therefore needs this to be switched)
    if ((config_get_uart_mcp_reassign()) && (idsw_get_cpu_type() == CPU_MCP) &&
        (idsw_get_die_id() == DIE_0) && platform_supports_oob(sdv_id))
    {
        // Take the AP-NS UART instead
        pl011_config.base_address = MSCP_ATU_AP_WINDOW_UART_NS_BASE_ADDR;

        // Use polling, because we can't get interrupts from other subsystems
        pl011_config.config_type = TEXTIO_PL011_CONFIG_TYPE_POLLING;

        // Update the clock frequency to match the clock for the different UART.
        // Default to the FPGA rate, and update if on silicon
        pl011_config.clk_freq = AP_NS_UART_APB_FREQUENCY_FPGA;
        if (sdv_id == PLATFORM_RVP_EVT_SILICON)
        {
            pl011_config.clk_freq = AP_NS_UART_APB_FREQUENCY_SOC;
        }
    }

    static textio_pl011_device_t pl011_device = {0};

    // get driver fwk threadx handle
    PDFWK_THREADX_HOST drvfwk = (PDFWK_THREADX_HOST)fpfw_init_get_handle(dfwk_id);

    // Initialize the pl011 uart device.
    textio_pl011_device_initialize(&pl011_device, &pl011_config, &drvfwk->Schedule);

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_UART_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_UART_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &pl011_device};
}

PLACED_CODE FPFW_INIT_COMPONENT(std_io, FPFW_INIT_DEPENDENCIES("uart", "boot_stat"))
{

    // Pl011 TextIo Interfaces
    static textio_pl011_interface_t pl011_interface_stdio = {0};
    fpfw_init_component_id_t uart_id = "uart";

    // get uart device handle
    textio_pl011_device_t* uart_handle = (textio_pl011_device_t*)fpfw_init_get_handle(uart_id);

    // Initialize a pl011 interface for stdio_textio
    textio_pl011_device_interface_initialize(uart_handle, &pl011_interface_stdio);
    stdio_textio_init(&pl011_interface_stdio.header);

    /* SCP Build Info */
    print_build_info();

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_STDIO_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_STDIO_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}