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
#include <spi_ctrl_bridge.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/
#define D2D_MBOX_SPI_CTRL_BASE_ADDR   (MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_SPI_CTRL_ADDRESS)
#define D2D_MBOX_SPI_BRIDGE_BASE_ADDR (MSCP_TOP_MSCP_EXP_ADDRESS + MSCP_EXP_TOP_SPI_BRIDGE_ADDRESS)
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(spi_bridge, FPFW_INIT_DEPENDENCIES("hw_ver"))
{
#if defined(SCP_RUNTIME_INIT)
    int32_t status = spi_controller_bridge_init(D2D_MBOX_SPI_CTRL_BASE_ADDR, D2D_MBOX_SPI_BRIDGE_BASE_ADDR, 20);
    if (status != 0)
    {
        return (fpfw_init_result_t){status, NULL};
    }
#endif

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
