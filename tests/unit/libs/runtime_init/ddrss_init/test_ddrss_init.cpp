//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_ddrss_init.cpp
 * DDRSS init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {

#include <fpfw_init.h>
#include <idsw.h> // for DIE_ID, PLAT_ID, _PLAT_ID

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_ddr;

/*------------- Functions ----------------*/

//
// Mocks
//

DIE_ID __wrap_idhw_get_die_id()
{
    return mock_type(DIE_ID);
}

void __wrap_ddrss_lib_init(uint8_t die_num)
{
    check_expected(die_num);
}

//
// Tests
//

TEST_FUNCTION(test_ddr_init, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (DIE_ID)0;
    will_return(__wrap_idhw_get_die_id, test_die);
    expect_value(__wrap_ddrss_lib_init, die_num, test_die);

    // Call API under test
    _fpfw_component_ddr.init_fn();
}
}
