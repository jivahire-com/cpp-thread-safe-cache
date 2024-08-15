//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_tower_init.cpp
 * Implement unit tests for tower init
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for CmockaWrapperTest, TEST_FUNCTION, che...

extern "C" {
#include <fpfw_icc_base.h>
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <silibs_status.h>
#include <stdint.h> // for uint16_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_tower_cfg;
static fpfw_icc_base_ctx_t* icc_ctx = nullptr;
static uint32_t dummy_icc_ctx = 0;

/*------------- Functions ----------------*/

int __wrap_tower_init(uint8_t die_num)
{
    check_expected(die_num);

    return SILIBS_SUCCESS;
}

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

void* __wrap_fpfw_init_get_handle(const char* id)
{
    check_expected_ptr(id);
    function_called();

    return mock_ptr_type(void*);
}

TEST_FUNCTION(test_tower_init, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;
    
    // Setting up the ICC flow
    icc_ctx = (fpfw_icc_base_ctx_t*)&dummy_icc_ctx;

    expect_string(__wrap_fpfw_init_get_handle, id, "icc_hspmbx");
    expect_function_call(__wrap_fpfw_init_get_handle);
    will_return(__wrap_fpfw_init_get_handle, &icc_ctx);
    will_return_always(__wrap_idsw_get_die_id, test_die);
    expect_value(__wrap_tower_init, die_num, test_die);
    _fpfw_component_tower_cfg.init_fn();
}
}
