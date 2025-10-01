/**
 * @file dma_init.c
 * Instantiates DMA driver and interface
 */

/*------------- Includes -----------------*/
#include <DfwkThreadXHost.h> // for DFWK_THREADX_HOST
#include <dma_dfwk.h>        // for dma_device_init, dma_interface_init
#include <dmac.h>            // for dma_init
#include <fpfw_init.h>       // for FPFW_INIT_COMPONENT
#include <interrupts.h>      // HW_INT_GPIO_CTRL_4_INT, HW_INT_GPIO_CTRL_6_INT
#include <silibs_status.h>   // for SILIBS_SUCCESS

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

/**
 * @brief Initialize DMA library and controller hardware
 *
 */
FPFW_INIT_COMPONENT(dma_all, FPFW_INIT_DEPENDENCIES("atu_svc", "dfwk", "mpu"))
{
    static dma_device_t dma_dev = {};
    static dma_interface_t driver_interface = {};

    static dma_config_t dma_config = {
        .base_address = 0,
        .config_type = DMA_CONFIG_TYPE_POLLING,
    };

    DFWK_THREADX_HOST* dfwk_host = (DFWK_THREADX_HOST*)fpfw_init_get_handle("dfwk");

    dma_device_init(&dma_dev, &dma_config, &dfwk_host->Schedule);
    dma_interface_init(&dma_dev, &driver_interface);
    dma_lib_init(&dma_dev);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &driver_interface};
}