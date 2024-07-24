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

#include <DfwkDriver.h>
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

void __wrap_scp_avs_cli_initialize(pscp_avs_interface_t interface_array[])
{
    check_expected_ptr(interface_array);
}

//
// Tests
//
TEST_FUNCTION(test_avs_scp_cli, nullptr, nullptr)
{
    // Set up expectations
    scp_avs_interface_t* avs_test_array[4] = {0};
    scp_avs_interface_t avs0_test_host = {};
    scp_avs_interface_t avs1_test_host = {};
    scp_avs_interface_t avs2_test_host = {};
    scp_avs_interface_t avs3_test_host = {};

    will_return(__wrap_fpfw_init_get_handle, &avs0_test_host);
    will_return(__wrap_fpfw_init_get_handle, &avs1_test_host);
    will_return(__wrap_fpfw_init_get_handle, &avs2_test_host);
    will_return(__wrap_fpfw_init_get_handle, &avs3_test_host);

    avs_test_array[0] = &avs0_test_host;
    avs_test_array[1] = &avs1_test_host;
    avs_test_array[2] = &avs2_test_host;
    avs_test_array[3] = &avs3_test_host;

    expect_any(__wrap_fpfw_init_get_handle, id);
    expect_any(__wrap_fpfw_init_get_handle, id);
    expect_any(__wrap_fpfw_init_get_handle, id);
    expect_any(__wrap_fpfw_init_get_handle, id);

    expect_memory(__wrap_scp_avs_cli_initialize, interface_array, avs_test_array, sizeof(avs_test_array));

    // Call API under test
    _fpfw_component_avs_cli.init_fn();
}
}