//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_err_domain_scp_init.cpp
 * Tests the init of the scp error domain
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {

#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_init.h> // for fpfw_init_result_t, fpfw_init_component_t
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_scp_ras;
/*------------- Functions ----------------*/
//
// Mocks
//

void __wrap_register_scp_error_domain()
{
    function_called();
}

void __wrap_register_smmu_error_domain()
{
    function_called();
}

void __wrap_register_gic_error_domain()
{
    function_called();
}

void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

void __wrap_mcp_error_injection_setup_listener(void* mhu_handle)
{
    FPFW_UNUSED(mhu_handle);
    function_called();
}
}

//
// Tests
//

TEST_FUNCTION(scp_ras, nullptr, nullptr)
{
    expect_function_call(__wrap_mcp_error_injection_setup_listener);
    will_return_always(__wrap_fpfw_init_get_handle, (void*)1234);
    expect_function_call(__wrap_register_scp_error_domain);
    expect_function_call(__wrap_register_smmu_error_domain);
    expect_function_call(__wrap_register_gic_error_domain);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_scp_ras.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
}
