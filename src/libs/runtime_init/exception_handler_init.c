//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file exception_handler_init.c
 *  Sets up the exception handler for the Cortex-M7
 */

/*------------- Includes -----------------*/
#include <FpFwAssert.h>         // for FPFW_RUNTIME_ASSERT
#include <exception_handler.h>  // for exception_handler_init
#include <fpfw_init.h>          // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <kng_error.h>          // for KNG_SUCCESS
#include <stddef.h>             // for NULL
#include <stdint.h>             // for int32_t

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
/**
 * @brief Register main exception handler for fault ISR and debug monitor and thread stack error.
 * 
 */
FPFW_INIT_COMPONENT(excp_hnd, FPFW_INIT_DEPENDENCIES("cd_init", "nvic"))
{
    // Initialize the exception handler.
    int32_t status = exception_handler_init();
    FPFW_RUNTIME_ASSERT(status == KNG_SUCCESS);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
