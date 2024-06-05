//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_etc_init.cpp
 * ETC Init test
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <event_trace_collector.h> // for etc_service_config_t, etc_service...
#include <fpfw_init.h>             // for fpfw_init_component_t
#include <stddef.h>                // for NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern fpfw_init_component_t _fpfw_component_etc_init;

/*------------- Functions ----------------*/

//
// Mocks
//
void __wrap_etc_initialize(etc_service_context_t* p_service, const etc_service_config_t* p_config)
{
    check_expected(p_service);
    check_expected(p_config);
}

//
// Tests
//
TEST_FUNCTION(test_etc_init, nullptr, nullptr)
{
    // Set up expectations
    expect_not_value(__wrap_etc_initialize, p_service, NULL);
    expect_not_value(__wrap_etc_initialize, p_config, NULL);

    // Call API under test
    _fpfw_component_etc_init.init_fn();
}
}