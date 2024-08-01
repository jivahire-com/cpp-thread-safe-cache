/**
 * @file gpio_init.c
 * Instantiates GPIO driver and interface
 */

/*------------- Includes -----------------*/
#include <DfwkThreadXHost.h> // for DFWK_THREADX_HOST
#include <FpFwAssert.h>      // for FPFW_RUNTIME_ASSERT
#include <fpfw_init.h>       // for FPFW_INIT_COMPONENT
#include <gpio.h>            // for gpio_device_init, gpio_interface_init
#include <gpio_lib.h>        // for gpio_init
#include <interrupts.h>      // HW_INT_GPIO_CTRL_4_INT, HW_INT_GPIO_CTRL_6_INT
#include <silibs_status.h>   // for SILIBS_SUCCESS

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
/**
 * @brief Initialize and configure GPIO registers.
 *
 */
FPFW_INIT_COMPONENT(gpio_lib, FPFW_INIT_DEPENDENCIES("mpu"))
{
    int status = SILIBS_SUCCESS;

    // Initialize GPIO
    status = gpio_init(NULL, 0);
    FPFW_RUNTIME_ASSERT(status == SILIBS_SUCCESS);

    status = gpio_afm_init(NULL, 0);
    FPFW_RUNTIME_ASSERT(status == SILIBS_SUCCESS);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

/**
 * @brief Initialize GPIO device instance.
 *
 */
FPFW_INIT_COMPONENT(gpio_dev, FPFW_INIT_DEPENDENCIES("gpio_lib", "dfwk"))
{
    static gpio_irq_config_t gpio_irq_config[] = {
        {.nvic_irq = HW_INT_GPIO_CTRL_4_INT, .gpio_ctrl_id = MSCP_EXP_GPIO_4},
        {.nvic_irq = HW_INT_GPIO_CTRL_6_INT, .gpio_ctrl_id = MSCP_EXP_GPIO_6},
    };
    static gpio_device_t gpio_device;
    gpio_device.IrqConfig = gpio_irq_config;
    gpio_device.IrqConfigCount = sizeof(gpio_irq_config) / sizeof(gpio_irq_config[0]);

    DFWK_THREADX_HOST* dfwk_host = (DFWK_THREADX_HOST*)fpfw_init_get_handle("dfwk");
    gpio_device_init(&gpio_device, &(dfwk_host->Schedule));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &gpio_device};
}

/**
 * @brief Initialize GPIO interface instance.
 *
 */
FPFW_INIT_COMPONENT(gpio_int, FPFW_INIT_DEPENDENCIES("gpio_dev", "nvic"))
{
    static gpio_interface_t gpio_interface;

    gpio_interface_init(&gpio_interface, fpfw_init_get_handle("gpio_dev"));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &gpio_interface};
}
