//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cded_interrupts_mock.c
 * CDED interrupt MOCKS for unit testing. Only built for win32
 */

/*-------------------------------- Includes ---------------------------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwCMocka.h>
#include <cded_interrupts.h>
#include <stddef.h>
#include <utils.h> // for UNUSED

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/

/**
 * @brief cded_int_enable - Mock function
 *
 *  @param[in]
 *      coproc_apb_addr - CDED CFG base address
 *      vector
 *
 *  @return
 *      int
 */
int __wrap_cded_int_enable(uintptr_t coproc_apb_addr, cded_int_t vector)
{
    UNUSED(coproc_apb_addr);
    UNUSED(vector);
    return mock_type(int);
}

int __wrap_cded_int_disable(uintptr_t coproc_apb_addr, cded_int_t vector)
{
    UNUSED(coproc_apb_addr);
    UNUSED(vector);
    return mock_type(int);
}

/**
 * @brief cded_int_status_clear - Mock function
 *
 *  @param[in]
 *      coproc_apb_addr - CDED CFG base address
 *      vector
 *
 *  @return
 *      int
 */
int __wrap_cded_int_status_clear(uintptr_t coproc_apb_addr, cded_int_t vector)
{
    UNUSED(coproc_apb_addr);
    UNUSED(vector);
    return mock_type(int);
}

int __wrap_cded_int_mask_enable(uintptr_t coproc_apb_addr, CDED_INT_CATEGORY category_id, uint32_t interrupt_mask)
{
    UNUSED(coproc_apb_addr);
    UNUSED(category_id);
    UNUSED(interrupt_mask);

    return mock_type(int);
}

int __wrap_cded_int_mask_disable(uintptr_t coproc_apb_addr, CDED_INT_CATEGORY category_id, uint32_t interrupt_mask)
{
    UNUSED(coproc_apb_addr);
    UNUSED(category_id);
    UNUSED(interrupt_mask);

    return mock_type(int);
}

int __wrap_cded_clear_trigger_int_mask(uintptr_t coproc_apb_addr, CDED_INT_CATEGORY category_id, uint32_t interrupt_mask)
{
    UNUSED(coproc_apb_addr);
    UNUSED(category_id);
    UNUSED(interrupt_mask);

    return mock_type(int);
}

int __wrap_cded_clear_trigger_int(uintptr_t coproc_apb_addr, cded_int_t vector)
{
    UNUSED(coproc_apb_addr);
    UNUSED(vector);

    return mock_type(int);
}

int __wrap_cded_int_mask_status_clear(uintptr_t coproc_apb_addr, CDED_INT_CATEGORY category_id, uint32_t interrupt_mask)
{
    UNUSED(coproc_apb_addr);
    UNUSED(category_id);
    UNUSED(interrupt_mask);

    return mock_type(int);
}

uint32_t __wrap_cded_int_get_category_status_reg_addr(CDED_INT_CATEGORY category_id, uint32_t coproc_apb_addr)
{
    UNUSED(coproc_apb_addr);
    UNUSED(category_id);

    return mock_type(uint32_t);
}

} // extern "c"