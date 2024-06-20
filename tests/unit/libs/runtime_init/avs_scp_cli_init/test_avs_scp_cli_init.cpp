//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_avs_scp_init.cpp
 * AVS Init test
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include "debug.h" // for UNUSED

#include <DfwkDriver.h>
#include <DfwkThreadXHost.h>
#include <fpfw_init.h>
#include <scp_avs_driver.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_avs_cli;

/*------------- Functions ----------------*/
//
// Mocks
//
void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    check_expected_ptr(id);
    return mock_type(void*);
}

void __wrap_scp_avs_cli_initialize(PDFWK_INTERFACE_HEADER Interface)
{
    check_expected_ptr(Interface);
}

//
// Tests
//
TEST_FUNCTION(test_avs0_scp_cli, nullptr, nullptr)
{
    // Set up expectations
    scp_avs_interface_t* avs0_test_host = {};

    // Set up expectations
    will_return(__wrap_fpfw_init_get_handle, &avs0_test_host);
    expect_any(__wrap_fpfw_init_get_handle, id);
    expect_any(__wrap_scp_avs_cli_initialize, Interface);

    // Call API under test
    _fpfw_component_avs_cli.init_fn();
}
}