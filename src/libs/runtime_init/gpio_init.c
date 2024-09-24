//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file gpio_init.c
 * Instantiates GPIO driver and interface
 */

/*------------- Includes -----------------*/
#include <DfwkThreadXHost.h> // for DFWK_THREADX_HOST
#include <FpFwAssert.h>      // for FPFW_RUNTIME_ASSERT
#include <fpfw_init.h>       // for FPFW_INIT_COMPONENT
#include <gpio.h>            // for gpio_device_init, gpio_interface_init
#include <gpio_cli.h>        // for gpio_cli_init
#include <gpio_lib.h>        // for gpio_init
#include <idsw.h>            // for idsw_get_platform_sdv, idsw...
#include <idsw_kng.h>
#include <interrupts.h>      // HW_INT_GPIO_CTRL_4_INT, HW_INT_GPIO_CTRL_6_INT
#include <silibs_status.h>   // for SILIBS_SUCCESS
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static const gpio_config_entry_t fpga_config_gpio_table[] = {
    // Override the default gpio configm for the FPGA, since one some systems, On GPIO Bank 6, 
    // Pin 6 and Pin 7 are looped back. Hence, we will use these pins to check if GPIO is connected.
    {GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_4, 0xFF),
    {.grp = {.grp_level = 0x1C, .grp_dir = 0x10, .grp_int_enable = 0x20, .grp_int_lvl_edge = 0x20}}},
    {GPIO_CTRL_PIN_MSK(MSCP_EXP_GPIO_6, 0xFF),
    {.grp = {.grp_level = 0x0F, .grp_dir = 0x4D, .grp_int_enable = 0x10, .grp_int_lvl_edge = 0x00}}},
};

/*------------- Functions ----------------*/
/**
 * @brief Initialize and configure GPIO registers.
 *
 */
FPFW_INIT_COMPONENT(gpio_lib, FPFW_INIT_DEPENDENCIES("mpu", "hw_ver"))
{
    int status = SILIBS_SUCCESS;
    static gpio_init_config_t gpio_init_config;

    // Initialize GPIO
    status = gpio_afm_init(NULL, 0);
    FPFW_RUNTIME_ASSERT(status == SILIBS_SUCCESS);

    if (idsw_get_platform_sdv() == PLATFORM_FPGA_LARGE)
    {
        printf("Programming FPGA specific GPIO Config table (Supporting loopback of pins 6 and 7 on GPIO Bank 6) . . .\n\r)");
        status = gpio_init(fpga_config_gpio_table, ARRAY_SIZE(fpga_config_gpio_table));
        gpio_init_config.gpio_config_table = fpga_config_gpio_table;
        gpio_init_config.table_size = ARRAY_SIZE(fpga_config_gpio_table);
    }
    else 
    {
        status = gpio_init(NULL, 0);
        gpio_init_config.gpio_config_table = NULL;
        gpio_init_config.table_size = 0;
    }
    FPFW_RUNTIME_ASSERT(status == SILIBS_SUCCESS);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &gpio_init_config};
}

/**
 * @brief Initialize GPIO device instance.
 *
 */
FPFW_INIT_COMPONENT(gpio_dev, FPFW_INIT_DEPENDENCIES("gpio_lib", "dfwk", "nvic"))
{
    static gpio_irq_config_t gpio_irq_config[] = {
        {.nvic_irq = HW_INT_GPIO_CTRL_4_INT, .gpio_ctrl_id = MSCP_EXP_GPIO_4},
        {.nvic_irq = HW_INT_GPIO_CTRL_6_INT, .gpio_ctrl_id = MSCP_EXP_GPIO_6},
    };
    static gpio_device_t gpio_device;
    gpio_device.IrqConfig = gpio_irq_config;
    gpio_device.IrqConfigCount = sizeof(gpio_irq_config) / sizeof(gpio_irq_config[0]);
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

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}