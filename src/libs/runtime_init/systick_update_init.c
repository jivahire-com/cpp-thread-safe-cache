//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file systick_update_init.c
 * Updates the SysTick timer for the M7 core after decoding platform configuration
 */

/*-------------------------------- Includes ---------------------------------*/
#include <fpfw_init.h>      // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT...
#include <stdint.h>         // for uint32_t, uintptr_t
#include <systick_update.h> // for systick_update_init

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/

FPFW_INIT_COMPONENT(systick_upd, FPFW_INIT_DEPENDENCIES("hw_ver"))
{
    return (fpfw_init_result_t){systick_update_init(), NULL};
}
