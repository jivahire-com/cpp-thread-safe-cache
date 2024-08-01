//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file gpio_lib_mock.cpp
 *
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <silibs_common.h>
#include <silibs_status.h> // for SILIBS_SUCCESS
#include <stdint.h>        // for uint32_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
int __wrap_gpio_get_interrupt_status(uint32_t gpio_ctrl_pin_id, uint32_t* intr)
{
    check_expected(gpio_ctrl_pin_id);
    *intr = mock_type(uint32_t);
    function_called();

    return SILIBS_SUCCESS;
}

int __wrap_gpio_get_input(uint32_t gpio_ctrl_pin_id, uint32_t* level)
{
    check_expected(gpio_ctrl_pin_id);
    *level = mock_type(uint32_t);
    function_called();

    return SILIBS_SUCCESS;
}

int __wrap_gpio_clear_interrupt_status(uint32_t gpio_ctrl_pin_id)
{
    check_expected(gpio_ctrl_pin_id);
    function_called();

    return SILIBS_SUCCESS;
}

//
// Tests
//

} // extern "C"