//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file gtimer_init.c
 * Initialization of the gtimer library
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <silibs_scp_exp_top_regs.h>
#include <silibs_scp_top_regs.h>
#include <spi_bridge.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/
#define D2D_MBOX_SPI_CTRL_BASE_ADDR   (MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_SPI_CTRL_ADDRESS)
#define D2D_MBOX_SPI_BRIDGE_BASE_ADDR (MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_SPI_BRIDGE_ADDRESS)
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
/**
 * @brief API to configure & verify spi controller & bridge
 *
 * @param spi_master_reg base addr of spi controller
 * @param clkDiv
 * @return int32_t
 */
#if defined(SCP_RUNTIME_INIT)
static int32_t SpiControllerBridgeInit(uintptr_t spi_ctrl_addr, uintptr_t spi_bridge_addr, uint16_t clkDiv)
{
    int32_t status = 0;
    //! SPI bridge initialization
    spi_bridge_init(spi_bridge_addr, clkDiv);
    status = spi_bridge_check_errors(spi_bridge_addr);
    if (status != SILIBS_SUCCESS)
    {
        printf("SPI bridge Init Error. Addr[0x%x]\n", spi_bridge_addr);
        return status;
    }
    spi_bridge_clear_error_interrupts(spi_bridge_addr);

    //! SPI Controller initialization
    spi_controller_init(spi_ctrl_addr, clkDiv);
    status = spi_controller_check_errors(spi_ctrl_addr);
    if (status != SILIBS_SUCCESS)
    {
        printf("SPI Controller Init Error. Addr[0x%x]\n", spi_ctrl_addr);
    }
    return status;
}
#endif

FPFW_INIT_COMPONENT(spi_bridge, FPFW_INIT_DEPENDENCIES("hw_ver"))
{
#if defined(SCP_RUNTIME_INIT)
    int32_t status = SpiControllerBridgeInit(D2D_MBOX_SPI_CTRL_BASE_ADDR, D2D_MBOX_SPI_BRIDGE_BASE_ADDR, 10);
    if (status != 0)
    {
        return (fpfw_init_result_t){status, NULL};
    }
#endif

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
