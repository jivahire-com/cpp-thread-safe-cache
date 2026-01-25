//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file gpio_init.c
 * Instantiates GPIO driver and interface
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <DfwkThreadXHost.h> // for DFWK_THREADX_HOST

#ifdef MCP_RUNTIME_INIT
    #include <afm_cli.h>
#endif

#include <boot_status.h>
#include <bug_check.h>
#include <fpfw_cfg_mgr.h>
#include <fpfw_init.h> // for FPFW_INIT_COMPONENT
#include <gpio.h>      // for gpio_device_init, gpio_interface_init
#include <gpio_cli.h>  // for gpio_cli_init
#include <gpio_lib.h>  // for gpio_init
#include <gpio_struct_defaults.h>
#include <idsw.h> // for idsw_get_platform_sdv, idsw...
#include <idsw_kng.h>
#include <interrupts.h>        // HW_INT_GPIO_CTRL_4_INT, HW_INT_GPIO_CTRL_6_INT
#include <kng_soc_constants.h> // for SOC_D0, D1
#include <silibs_status.h>     // for SILIBS_SUCCESS
#include <stdio.h>
#include <variable_services.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

// Define GPIO configuration table based onKingsgate RMSS HAS Chapter 15.11.2
// https://microsoft.sharepoint.com/teams/EchoFalls/Shared%20Documents/Kingsgate%20SOC/Architecture/HAS%201.0/RMSS/KingsGateRMSS%20HAS%20v0p1.4.docx?web=1
static const gpio_config_entry_t fpga_config_gpio_table[] = {
    // Override the default gpio config for the FPGA, since one some systems, On GPIO Bank 6,
    // Pin 6 and Pin 7 are looped back. Hence, we will use these pins to check if GPIO is connected.
    // Disable GPIO4_5 interrupt, since power code polls for VR_HOT_SOC
    {GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_4, 0xFF),
     {.grp = {.grp_level = 0x1C, .grp_dir = 0x1C, .grp_int_enable = 0x00, .grp_int_lvl_edge = 0x20}}},
    {GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_6, 0xFF),
     {.grp = {.grp_level = 0x0F, .grp_dir = 0x4D, .grp_int_enable = 0x10, .grp_int_lvl_edge = 0x00}}},
};

static const gpio_config_entry_t def_gpio_config_table_grp[] = {
    // Program a 8-pin group together, optimized for reducing register access.
    // Each value is a 8 bit vector, and each bit control one GPIO in the group.
    // Controller  pin_mask
    // level 1:high 0:low | dir 1:out 0:in  | int_en 1:enable 0:disable | int_sense 1:low_level 0:rising_edge
    // Disable GPIO4_5 interrupt, since power code polls for VR_HOT_SOC
    {GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_4, 0xFF),
     {.grp = {.grp_level = 0x1C, .grp_dir = 0x1C, .grp_int_enable = 0x00, .grp_int_lvl_edge = 0x20}}},
    {GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_6, 0xFF),
     {.grp = {.grp_level = 0x0F, .grp_dir = 0x0D, .grp_int_enable = 0x10, .grp_int_lvl_edge = 0x00}}},
};
/* AFM configuration for i3c scl and sda gpios */
static const gpio_afm_entry_t fpga_config_gpio_table_afm[] = {
    {PADRING_SE_AFM_I3C2_SCL,
     {{.pull_down = 0, .pull_up = 0, .drive_strength = 0, .slew_rate = 0, .schmitt_trigger = 0, .afmsel = 1}}},
    {PADRING_SE_AFM_I3C2_SDA,
     {{.pull_down = 0, .pull_up = 0, .drive_strength = 0, .slew_rate = 0, .schmitt_trigger = 0, .afmsel = 1}}},
};

/*------------- Functions ----------------*/
/**
 * @brief Initialize and configure GPIO registers.
 *
 */
FPFW_INIT_COMPONENT(gpio_lib, FPFW_INIT_DEPENDENCIES("mpu", "hw_ver", "debug_print", "boot_stat"))
{
    int status = SILIBS_SUCCESS;
    static gpio_init_config_t gpio_init_config;
    static DEFAULT_INSTANCE_UART_AFM_CFG_T(uart_afm_knobs);
    DIE_INSTANCE die_id = idsw_get_die_id();

    // Initialize only on SCP
    if (idsw_get_cpu_type() == CPU_MCP)
    {
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }
    // Initialize GPIO
    status = gpio_afm_init(NULL, 0);
    BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, 0);

    int i3c_config_table_size = ARRAY_SIZE(fpga_config_gpio_table_afm);
    // update i3c AFM gpio setting.
    status = gpio_afm_init(fpga_config_gpio_table_afm, i3c_config_table_size);
    BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, 0);

    if (idsw_get_platform_sdv() == PLATFORM_FPGA_LARGE || idsw_get_platform_sdv() == PLATFORM_FPGA_LARGE_RVP)
    {
        FPFW_DBGPRINT_INFO("FPGA GPIO Config (Loopback pins 6,7on bank 6)\n");
        status = gpio_init(fpga_config_gpio_table, ARRAY_SIZE(fpga_config_gpio_table));
        gpio_init_config.gpio_config_table = fpga_config_gpio_table;
        gpio_init_config.table_size = ARRAY_SIZE(fpga_config_gpio_table);
    }
    else
    {
        status = gpio_init(def_gpio_config_table_grp, ARRAY_SIZE(def_gpio_config_table_grp));
        gpio_init_config.gpio_config_table = def_gpio_config_table_grp;
        gpio_init_config.table_size = ARRAY_SIZE(def_gpio_config_table_grp);
    }

    BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, 0);

    if (die_id == SOC_D0)
    {
        uart_afm_cfg_prod_t uart_afm_knobs_prod = config_get_uart_afm_cfg_die0();
        uart_afm_knobs.uart_afm[0] = uart_afm_knobs_prod.uart0_afm;
        uart_afm_knobs.uart_afm[1] = uart_afm_knobs_prod.uart1_afm;
        uart_afm_knobs.uart_afm[2] = uart_afm_knobs_prod.uart2_afm;
        uart_afm_knobs.uart_afm[3] = uart_afm_knobs_prod.uart3_afm;
    }
    else
    {
        uart_afm_cfg_d1_prod_t uart_afm_knobs_prod = config_get_uart_afm_cfg_die1();
        uart_afm_knobs.uart_afm[0] = uart_afm_knobs_prod.uart0_afm;
        uart_afm_knobs.uart_afm[1] = uart_afm_knobs_prod.uart1_afm;
        uart_afm_knobs.uart_afm[2] = uart_afm_knobs_prod.uart2_afm;
        uart_afm_knobs.uart_afm[3] = uart_afm_knobs_prod.uart3_afm;
    }

    status = gpio_override_uart_afmsel(die_id, &uart_afm_knobs);

    if (idsw_get_platform_sdv() == PLATFORM_RVP_EVT_SILICON)
    {
        // Override AFM for shared UART pins in case of PLATFORM_RVP_EVT_SILICON
        gpio_configure_shared_uart(die_id,
                                   config_get_uart_die_cfg().uart_die_sel[0],
                                   config_get_uart_die_cfg().uart_die_sel[1]);
    }

    BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, 0);

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_GPIO_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_GPIO_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &gpio_init_config};
}

/**
 * @brief Initialize GPIO device instance.
 *
 */
FPFW_INIT_COMPONENT(gpio_dev, FPFW_INIT_DEPENDENCIES("gpio_lib", "dfwk", "nvic"))
{
    static gpio_irq_config_t gpio_irq_config[] = {
#if defined(SCP_RUNTIME_INIT)
        {.nvic_irq = HW_INT_GPIO_CTRL_4_INT, .gpio_ctrl_id = MSCP_EXP_GPIO_4},
#elif defined(MCP_RUNTIME_INIT)
        {.nvic_irq = HW_INT_GPIO_CTRL_6_INT, .gpio_ctrl_id = MSCP_EXP_GPIO_6},
#endif
    };
    static gpio_device_t gpio_device;
    gpio_device.IrqConfig = gpio_irq_config;
    gpio_device.IrqConfigCount = ARRAY_SIZE(gpio_irq_config);
    gpio_device.ConfigTable = (gpio_init_config_t*)fpfw_init_get_handle("gpio_lib");

    DFWK_THREADX_HOST* dfwk_host = (DFWK_THREADX_HOST*)fpfw_init_get_handle("dfwk");
    gpio_device_init(&gpio_device, &(dfwk_host->Schedule));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &gpio_device};
}

/**
 * @brief Initialize GPIO CLI.
 *
 */
FPFW_INIT_COMPONENT(gpio_cli, FPFW_INIT_DEPENDENCIES("gpio_dev", "cli"))
{
    // Create and initialize GPIO interface for CLI.
    static gpio_interface_t gpio_interface;
    gpio_interface_init(&gpio_interface);

    gpio_cli_init(&gpio_interface);

#ifdef MCP_RUNTIME_INIT
    /**
     * Initialize the remote GPIO CLI for MCP.
     * This is performed only on MCP because:
     * 1. MCP Die 0 has a persistent connection on UART3 to the debug PC, unmuxed with any other core.
     * 2. Saves precious code space on SCP
     */
    afm_cli_init();
#endif

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
