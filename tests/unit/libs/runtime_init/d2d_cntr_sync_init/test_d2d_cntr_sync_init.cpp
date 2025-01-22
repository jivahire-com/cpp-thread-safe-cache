//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_d2d_cntr_sync_init.cpp
 * d2d_cntr_sync init tests
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
extern fpfw_init_component_t _fpfw_component_d2d_cntr_sync;

/*------------- Functions ----------------*/

//
// Mocks
//

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

void __wrap_d2d_cntr_sync_init(uint8_t die_num)
{
    check_expected(die_num);
}

KNG_PLAT_ID __wrap_idsw_get_platform_sdv()
{
    return mock_type(KNG_PLAT_ID);
}

//
// Tests
//

TEST_FUNCTION(test_d2d_cntr_sync_init, nullptr, nullptr)
{
    // Set up expectations
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    expect_value(__wrap_d2d_cntr_sync_init, die_num, DIE_0);

    // Call API under test
    _fpfw_component_d2d_cntr_sync.init_fn();
}
}
