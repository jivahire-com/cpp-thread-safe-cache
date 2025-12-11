//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file spi_ctrl_bridge.h
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <silibs_platform.h>
#include <spi_bridge.h>
#include <spi_ctrl_bridge.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*------------- Functions ----------------*/
int32_t spi_controller_bridge_init(uintptr_t spi_ctrl_addr, uintptr_t spi_bridge_addr, uint16_t clkDiv)
{
    int32_t status = SILIBS_SUCCESS;
    /**
     * @note The SPI bridge acts as a translator or interface between the SPI bus and the internal system bus AHB.
     * Initializing the bridge first ensures that any SPI transactions can be correctly routed and interpreted by the system.
     * It sets up the path for communication between the SPI controller and the rest of the SoC
     */
    spi_bridge_init(spi_bridge_addr, clkDiv);
    if (spi_bridge_check_errors(spi_bridge_addr) != SILIBS_SUCCESS)
    {
        //! Clear any latched errors, this is important to ensure that the bridge is in a clean state before proceeding with further operations.
        spi_bridge_clear_error_interrupts(spi_bridge_addr);
    }

    /**
     * @brief Once the bridge is ready, the SPI controller can be initialized.
     * The controller manages the actual SPI protocol operations (clocking, chip select, data transfer).
     * It relies on the bridge to communicate with memory or peripherals mapped on the system bus.
     */
    spi_controller_init(spi_ctrl_addr, clkDiv);

    //! Now check for errors post all the initializations
    status = spi_bridge_check_errors(spi_bridge_addr);
    if (status != SILIBS_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("SPI Bridge Init Error. Addr[0x%x]\n", spi_bridge_addr);
    }

    status = spi_controller_check_errors(spi_ctrl_addr);
    if (status != SILIBS_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("SPI Controller Init Error. Addr[0x%x]\n", spi_ctrl_addr);
    }

    return status;
}
