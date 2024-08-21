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
 * @brief mmio_set_mock_data - Set MMIO_READ32 data
 *
 * @param[in] addr: Address to set data
 * @param[in] data: Data to set for address
 *
 *  @return
 *      void
 */
void mmio_set_mock_data(uint32_t addr, uint32_t data);

/**
 * @brief fpfw_mock_get_timer_cb - Get timer callback set during create
 *
 *  @return
 *      timer callback function
 */
fpfw_timer_callback fpfw_mock_get_timer_cb(void);
