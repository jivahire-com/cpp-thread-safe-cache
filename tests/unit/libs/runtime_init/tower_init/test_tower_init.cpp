//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_tower_init.cpp
 * Implement unit tests for tower init
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for CmockaWrapperTest, TEST_FUNCTION, che...
#include <stdint.h>        // for uint16_t

extern "C" {
#include <fpfw_init.h>
#include <idsw.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_tower_cfg;

/*------------- Functions ----------------*/

int __wrap_tower_init(uint8_t die_num)
{
    check_expected(die_num);

    return SILIBS_SUCCESS;
}

DIE_ID __wrap_idhw_get_die_id()
{
    return mock_type(DIE_ID);
}

TEST_FUNCTION(test_tower_init, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (DIE_ID)1;
    will_return_always(__wrap_idhw_get_die_id, test_die);
    expect_value(__wrap_tower_init, die_num, test_die);
    _fpfw_component_tower_cfg.init_fn();
}
}
