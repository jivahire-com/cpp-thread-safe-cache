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
#include <health_monitor.h>
#include <mscp_error_domain.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_hm_register_error_domain(uint16_t error_domain_idx,
                                     const guid_t* error_domain_guid,
                                     const char* error_domain_name,
                                     hm_error_injection_cb_t err_inject_cb,
                                     void* err_inject_ctx)
{
    assert_true(error_domain_idx == ACPI_ERROR_DOMAIN_SCP_PROC);
    assert_true(err_inject_cb != NULL);
    FPFW_UNUSED(error_domain_guid);
    FPFW_UNUSED(error_domain_name);
    FPFW_UNUSED(err_inject_ctx);

    ras_einj_info_t_temp einj_payload;
    err_inject_cb(&einj_payload, NULL);
    function_called();
}
}

//
// Tests
//

TEST_FUNCTION(test_register_scp_error_domain, nullptr, nullptr)
{
    expect_function_call(__wrap_hm_register_error_domain);
    register_scp_error_domain();
}