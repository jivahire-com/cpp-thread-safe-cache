//
// Copyright (c) Microsoft Corporation.
//

/**
 * @file spi_ctrl_bridge.h
 * 
 */

#pragma once

/*----------- Nested includes ------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief 
 *
 * \b Description: API to configure & verify spi controller & bridge
 *
 * @param[in] spi_ctrl_addr   Address of the SPI controller
 * @param[in] spi_bridge_addr  Address of the SPI bridge
 * @param[in] clkDiv  Clock divider value for SPI communication
 *
 * @return SILIBS_SUCCESS on success, error code otherwise
 */
int32_t spi_controller_bridge_init(uintptr_t spi_ctrl_addr, uintptr_t spi_bridge_addr, uint16_t clkDiv);

