//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_etc_etd_init.cpp
 * ETC and ETD init test
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <event_trace_collector.h> // for etc_service_config_t, etc_service...
#include <event_trace_decoder.h>   // for etd_service_config_t, etd_service...
#include <fpfw_init.h>             // for fpfw_init_component_t
#include <stddef.h>                // for NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern fpfw_init_component_t _fpfw_component_etc;
extern fpfw_init_component_t _fpfw_component_etd;

/*------------- Functions ----------------*/

//
// Mocks
//
void __wrap_etc_initialize(etc_service_context_t* p_service, const etc_service_config_t* p_config)
{
    check_expected(p_service);
    check_expected(p_config);
}

void __wrap_etd_initialize(etd_service_context_t* p_service, const etd_service_config_t* p_config)
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
    _fpfw_component_etc.init_fn();
}

TEST_FUNCTION(test_etd_init, nullptr, nullptr)
{
    // Set up expectations
    expect_not_value(__wrap_etd_initialize, p_service, NULL);
    expect_not_value(__wrap_etd_initialize, p_config, NULL);

    // Call API under test
    _fpfw_component_etd.init_fn();
}
}