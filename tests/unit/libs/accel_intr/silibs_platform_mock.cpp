//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file silibs_platform_mocks.c
 * silibs_platform MOCKS for unit testing. Only built for win32
 */

/*-------------------------------- Includes ---------------------------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {

#include <FpFwCMocka.h>
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <silibs_platform.h>
#include <stdint.h> // for uint32_t
#include <stdio.h>
#include <stdlib.h>
#include <utils.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/
typedef struct
{
    uint32_t addr;
    uint32_t value;
    void* p_next;
} mmio_rw_data_t;

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/
mmio_rw_data_t* head = NULL;

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/
void mmio_set_mock_data(uint32_t addr, uint32_t data)
{
    // Check if this address is part of current pool
    mmio_rw_data_t* prev = NULL;
    mmio_rw_data_t* cur = head;

    while (cur != NULL)
    {
        if (cur->addr == addr)
        {
            cur->value = data;
            return;
        }

        prev = cur;
        cur = (mmio_rw_data_t*)cur->p_next;
    }

    mmio_rw_data_t* p_data;
    p_data = (mmio_rw_data_t*)malloc(sizeof(mmio_rw_data_t));

    p_data->addr = addr;
    p_data->value = data;
    p_data->p_next = NULL;

    if (prev == NULL)
    {
        head = p_data;
    }
    else
    {
        prev->p_next = (void*)p_data;
    }
}

/**
 * @brief mmio_read32 - Mock function
 * Reads value if written by mmio_set_mock_data
 * Else returns 0x0
 */
uint32_t __wrap_mmio_read32(volatile uint32_t* addr)
{
    check_expected_ptr(addr);

    // Check if this address is part of current pool
    mmio_rw_data_t* cur = head;

    while (cur != NULL)
    {
        if (cur->addr == (uint32_t)addr)
        {
            return cur->value;
        }

        cur = (mmio_rw_data_t*)cur->p_next;
    }

    return 0x0;
}

/**
 * @brief mmio_write32 - Mock function
 */
void __wrap_mmio_write32(volatile uint32_t* addr, uint32_t data)
{
    check_expected_ptr(addr);
    check_expected(data);
}

/**
 * @brief mmio_update32 - Mock function
 */
void __wrap_mmio_update32(volatile uint32_t* addr, uint32_t data, uint32_t mask)
{
    check_expected_ptr(addr);
    check_expected(data);
    check_expected(mask);
}

} // extern "c"