//
// Copyright (c) Microsoft Corporation.
//

/**
 * @file bandgap.h
 * header for Voltage Reference Control Register manipulation
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Configure bandgap trim settings
 *
 * \b Description:
 *      Programs the REF_MACRO_TRIM register with specified slope and offset trim values.
 *      These values are typically derived from fuse reads or calibration data and are used
 *      to fine-tune the bandgap reference circuitry for voltage accuracy.
 *
 * @param[in] slope_trim   Trim value for slope adjustment
 * @param[in] offset_trim  Trim value for offset adjustment
 *
 * @return None
 */
void configure_bandgap_trim(uint32_t slope_trim, uint32_t offset_trim);

/**
 * @brief Enable or disable the bandgap circuitry
 *
 * \b Description:
 *      Sets the enable bit in the REF_MACRO_CTRL register to turn the bandgap reference
 *      circuitry on or off. This is typically called during early boot or power domain
 *      initialization.
 *
 * @param[in] enable   Boolean flag to enable (true) or disable (false) the bandgap circuit
 *
 * @return None
 */
void enable_bandgap_circuitry(bool enable);

