//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file systick_update.h
 * This contains the systick_update specific headers and macros.
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include <stdint.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/**
 * @brief Initializes the SysTick timer for the M7 core after decoding platform configuration.
 * 
 * This function sets the SysTick timer's reload value based on the EMCPU clock frequency
 * and the desired timer tick rate. It configures the SysTick timer to generate interrupts
 * at the specified rate.
 * 
 * @return
 * - KNG_SUCCESS: if the SysTick timer was successfully initialized.
 * - KNG_E_FAIL: if there was an error during initialization.
 * 
 * This function should be called during the system initialization phase to ensure
 * the SysTick timer is set up correctly before any other components rely on it.
 * 
 * @note The SysTick timer is a 24-bit timer, and the reload value is calculated
 * based on the EMCPU clock frequency and the desired timer tick rate.
 * 
 * @warning Ensure that the EMCPU clock frequency is correctly configured before
 * calling this function. An incorrect clock frequency may lead to
 * unexpected behavior or timer overflows.
 * 
 */
int32_t systick_update_init(void);

/**
 * @brief   Gets the SysTick timer's reload value.
 * 
 * This function retrieves the current reload value of the SysTick timer.
 *  
 * @return  uint32_t
 *          The current reload value of the SysTick timer.
 */
uint32_t systick_get_reload_val(void);


/**
 * @brief   Gets the EMCPU clock frequency.
 * 
 * This function retrieves the current EMCPU clock frequency (in Hz).
 *  
 * @return  uint32_t
 *          The current EMCPU clock frequency.
 */
uint32_t systick_get_emcpu_clock(void);