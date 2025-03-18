//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_startup_shutdown_init.cpp
 * Tests the init of the sos service
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {

#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_init.h> // for fpfw_init_result_t, fpfw_init_component_t
#include <health_monitor.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_mcp_ras;

/*------------- Functions ----------------*/
//
// Mocks
//
void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

void __wrap_start_mcp_error_injection_listener()
{
    function_called();
}

void __wrap_register_mcp_error_domain()
{
    function_called();
}
}

//
// Tests
//

TEST_FUNCTION(mcp_ras, nullptr, nullptr)
{
    will_return_always(__wrap_fpfw_init_get_handle, (void*)1234);
    expect_function_call(__wrap_register_mcp_error_domain);
    expect_function_call(__wrap_start_mcp_error_injection_listener);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_mcp_ras.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}