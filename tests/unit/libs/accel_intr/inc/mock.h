//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mock.h
 * Mock helper functions used in accel_intr
 */

/*-------------------------------- Includes ---------------------------------*/
#include <stdbool.h>           // for bool
#include "fpfw_timer_types.h" // for _fpfw_timer_variant_t, fpfw_timer_call...

#include <accelip_id.h> // for ACCEL_ID, NUM_VALID_ACCEL_ID

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/
/**
 * @brief set_int_status - Function used by test to set expected interrupt status
 *
 *  @param[in]
 *      bool
 *
 *  @return
 *      void
 */
void set_int_status(bool new_int_status);

/**
 * @brief fpfw_mock_get_timer_cb - Get timer callback set during create
 * 
 * @param accel_type - ACCEL_ID for which the timer callback is requested
 * @return - timer callback function
 */
fpfw_timer_callback fpfw_mock_get_timer_cb(ACCEL_ID accel_type);

/**
 * @brief fpfw_mock_get_timer_ctx - Get timer context set during create
 *
 * @param accel_type - ACCEL_ID for which the timer context is requested
 * @return - timer context
 */
void* fpfw_mock_get_timer_ctx(ACCEL_ID accel_type);

/**
 * @brief fpfw_mock_set_active_accel_type - Set the active accelerator type
 *
 * @param accel_type - ACCEL_ID to set as active
 */
void fpfw_mock_set_active_accel_type(ACCEL_ID accel_type);
