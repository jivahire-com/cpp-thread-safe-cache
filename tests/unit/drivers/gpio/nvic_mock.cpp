//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file nvic_mock.cpp
 *
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <nvic.h>      // for nvic_irq_clear_pending, nvic_irq_enable, nvic...
#include <stdint.h>    // for uint32_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
nvic_status_t __wrap_nvic_irq_set_isr_with_param(uint32_t irq_num, isr_callback_fn_with_params_t isr, void* parameter)
{
    FPFW_UNUSED(irq_num);
    FPFW_UNUSED(isr);
    FPFW_UNUSED(parameter);

    function_called();

    return NVIC_STATUS_SUCCESS;
}
nvic_status_t __wrap_nvic_irq_clear_pending(uint32_t irq_num)
{
    FPFW_UNUSED(irq_num);

    function_called();

    return NVIC_STATUS_SUCCESS;
}

nvic_status_t __wrap_nvic_irq_enable(uint32_t irq_num)
{
    FPFW_UNUSED(irq_num);

    function_called();

    return NVIC_STATUS_SUCCESS;
}

nvic_status_t __wrap_nvic_get_current_irq(uint32_t* irq_num)
{
    *irq_num = mock_type(uint32_t);
    function_called();

    return NVIC_STATUS_SUCCESS;
}

//
// Tests
//

} // extern "C"