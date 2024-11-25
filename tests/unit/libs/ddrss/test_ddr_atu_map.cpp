//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_atu_map.cpp
 * Tests the DDR ATU mapping
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_non_null, CmockaWrapperTest, TEST_...

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <ddr_atu_map.h>
#include <stddef.h> // for NULL, size_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
uintptr_t __real_ddrss_atu_map(uint32_t die_num);
void __real_ddrss_atu_unmap(uint32_t die_num);

/*------------- Functions ----------------*/
//
// Tests
//
TEST_FUNCTION(test_ddr_atu_map, NULL, NULL)
{
    // Set up pre-conditions / expectations
    will_return(__wrap_atu_map, 0x12345678);
    will_return(__wrap_atu_map, 0);

    // Execute the function under test
    uintptr_t mapped_addr = __real_ddrss_atu_map(0);

    // Verify the results
    assert_int_equal(mapped_addr, 0x12345678);
}

TEST_FUNCTION(test_ddr_atu_unmap, NULL, NULL)
{
    // Set up pre-conditions / expectations
    will_return(__wrap_atu_unmap, 0);

    // Execute the function under test
    __real_ddrss_atu_unmap(0);
}

} // extern "C"