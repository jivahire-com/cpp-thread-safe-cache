//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_mocks.cpp
 * Mock functions for DDR Manager unit tests
 */

/*------------- Includes -----------------*/
#include "ddr_mocks.h"

#include <CMockaWrapper.h>
#include <FpFwUtils.h>
#include <ddrss_lib.h>
#include <idsw_kng.h>
#include <silibs_status.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

//
// Mocks
//

extern "C" {

uint32_t __wrap_mmio_read32(void* addr)
{
    FPFW_UNUSED(addr);
    if (!in_setup_teardown)
    {
        function_called();
        return mock_type(uint32_t);
    }
    else
    {
        return 0;
    }
}

void __wrap_mmio_write32(void* addr, uint32_t data)
{
    if (!in_setup_teardown)
    {
        function_called();
    }

    FPFW_UNUSED(addr);
    FPFW_UNUSED(data);
    return;
}

bool __wrap_ddr_manager_platform_is_polling_supported()
{
    return true;
}

void __wrap_prod_ddrss_lib_init(KNG_DIE_ID die_num)
{
    FPFW_UNUSED(die_num);
    return;
}
} // extern "C"