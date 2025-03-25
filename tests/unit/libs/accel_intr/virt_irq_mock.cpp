//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file nvic_mock.cpp
 * NVIC driver mocks
 */

/*-------------------------------- Includes ---------------------------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep

extern "C" {
#include "accel_intr.h"  // for accel_intr_irq_init, ACCEL_INTR_RET_SUC...
#include "fpfw_status.h" // for FPFW_STATUS_SUCCESS

#include <FpFwUtils.h>
#include <nvic.h>   // for NVIC_STATUS_SUCCESS
#include <stdint.h> // for uint32_t
#include <virt_irq.h>

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/

/*****************************
 * MOCKS
 *****************************/

/**
 * @brief virt_irq_set_isr_with_param - Mock function
 */
nvic_status_t __wrap_virt_irq_set_isr_with_param(uint32_t irq_num, isr_callback_fn_with_params_t isr, void* parameter)
{
    irq_num = GET_PHYSICAL_IRQ_FROM_VIRTUAL_IRQ(irq_num);
    check_expected(irq_num);
    check_expected(isr);
    check_expected(parameter);
    return mock_type(nvic_status_t);
}

/**
 * @brief virt_irq_enable - Mock function
 */
nvic_status_t __wrap_virt_irq_enable(uint32_t irq_num)
{
    irq_num = GET_PHYSICAL_IRQ_FROM_VIRTUAL_IRQ(irq_num);
    check_expected(irq_num);
    return mock_type(nvic_status_t);
}

/**
 * @brief virt_irq_disable - Mock function
 */
nvic_status_t __wrap_virt_irq_disable(uint32_t irq_num)
{
    irq_num = GET_PHYSICAL_IRQ_FROM_VIRTUAL_IRQ(irq_num);
    check_expected(irq_num);
    return mock_type(nvic_status_t);
}

/**
 * @brief virt_irq_is_irq_enabled - Mock function
 */
nvic_status_t __wrap_virt_irq_is_irq_enabled(uint32_t virtual_irq_num, bool* enabled)
{
    FPFW_UNUSED(virtual_irq_num);

    *enabled = mock_type(bool);
    return mock_type(nvic_status_t);
}

} // extern "c"