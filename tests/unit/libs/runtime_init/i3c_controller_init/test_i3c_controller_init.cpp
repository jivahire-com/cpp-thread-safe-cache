//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_i3c_controller_init.cpp
 * i3c_controller init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {

#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_i3c_controller;

/*------------- Functions ----------------*/

//
// Mocks
//

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

void __wrap_i3c_controller(uint8_t die_num)
{
    check_expected(die_num);
}

//
// Tests
//

TEST_FUNCTION(test_i3c_controller_init, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_die_id, test_die);
    expect_value(__wrap_i3c_controller, die_num, test_die);

    // Call API under test
    _fpfw_component_i3c_controller.init_fn();
}
}
