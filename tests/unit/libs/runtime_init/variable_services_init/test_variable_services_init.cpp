//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_variable_services.cpp
 * Implement unit tests for variable services initialization
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for CmockaWrapperTest, TEST_FUNCTION, che...

extern "C" {
#include <FpFwUtils.h>     // for FPFW_UNUSED
#include <fpfw_icc_base.h> // for fpfw_icc_base_init, fpfw_icc_ba...
#include <fpfw_init.h>
#include <idhw.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_var_serv;
static uint32_t dummy_icc_ctx = 0;

/*------------- Functions ----------------*/
void* __wrap_fpfw_init_get_handle(const char* id)
{
    FPFW_UNUSED(id);
    return &dummy_icc_ctx;
}

KNG_DIE_ID __wrap_idhw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

TEST_FUNCTION(test_variable_service_init, nullptr, nullptr)
{
    // Set up expectations
    will_return_always(__wrap_idhw_get_die_id, (KNG_DIE_ID)DIE_0);
    _fpfw_component_var_serv.init_fn();
}
}
