//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file die_to_die_exchange_i.h
 * This file contains the private interface for the data exchange
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdint.h> // for uint8_t

/*-- Symbolic Constant Macros (defines) --*/


/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize the die to die exchange component.
 * This function initializes the die to die exchange component for a specific die.
 *
 * @param[in] die_id The ID of the die to initialize.
 */
void die_2_die_exchange_init(uint8_t die_id);

/**
 * @brief Get the ID of the current die.
 * This function returns the ID of the die that is currently being processed.
 *
 * @return The ID of the current die.
 */
uint8_t die_2_die_exchange_get_this_die_id(void);

/**
 * @brief Write the maximum die temperature to the die to die exchange.
 * This function writes the maximum die temperature in deci-Celsius to the exchange. It writes the location for the the
 * die specified by `this_die_id`, which should be initialized using `die_2_die_exchange_init`.
 *
 * @param[in] max_die_temperature_dC The maximum die temperature in deci-Celsius.
 */
void die_2_die_exchange_write_max_die_temp(uint16_t max_die_temperature_dC);

/**
 * @brief Read the maximum die temperature from the die to die exchange.
 * This function reads the maximum die temperature in deci-Celsius for a specific die.
 *
 * @param[in] die_id The ID of the die to read the temperature from.
 * @return The maximum die temperature in deci-Celsius.
 */
uint16_t die_2_die_exchange_read_max_die_temp_dC(uint8_t die_id);