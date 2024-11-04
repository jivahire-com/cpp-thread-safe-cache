//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_ap_core_init.cpp
 * Tests the init of the power service
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {
#include <DfwkDriver.h> // for DFWK_SCHEDULE
#include <FpFwCli.h>    // for _FPFW_CLI_CONFIG, CLI_SUCCESS, PFPFW_CLI_...
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <fpfw_init.h>  // for fpfw_init_component_t
#include <fuse_client.h>
#include <fuse_init.h>
#include <kng_soc_constants.h> // for NUM_AP_CORES_PER_DIE
#include <stdint.h>
#include <string.h> // for memcpy

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_fuse_svc;
extern fpfw_init_component_t _fpfw_component_cli_fuse;
/*------------- Functions ----------------*/
//
// Mocks
//
fpfw_icc_base_ctx_t* __wrap_fpfw_init_get_handle(const char* name)
{
    check_expected_ptr(name);
    function_called();
    return mock_ptr_type(fpfw_icc_base_ctx_t*);
}

void __wrap_fuse_init(fpfw_icc_base_ctx_t* icc_base_ctx)
{
    check_expected_ptr(icc_base_ctx);
}

int __wrap_platform_fuse_override(void)
{
    return mock_type(int);
}

int __wrap_platform_fuse_distribution(int stage)
{
    check_expected(stage);
    return mock_type(int);
}

FPFW_CLI_STATUS __wrap_platform_fuse_init_cli(void)
{
    function_called();
    return mock_type(FPFW_CLI_STATUS);
}

/* Tests */

TEST_FUNCTION(test_fuse_svc_init, NULL, NULL)
{
    fpfw_icc_base_ctx_t* dummy_icc_hspmbx_ctx = reinterpret_cast<fpfw_icc_base_ctx_t*>(1);
    // Mock ICC context initialization
    expect_string(__wrap_fpfw_init_get_handle, name, "icc_hspmbx");
    will_return(__wrap_fpfw_init_get_handle, dummy_icc_hspmbx_ctx);
    expect_function_call(__wrap_fpfw_init_get_handle);
    expect_value(__wrap_fuse_init, icc_base_ctx, dummy_icc_hspmbx_ctx);
    will_return(__wrap_platform_fuse_override, CLI_SUCCESS);
    expect_value(__wrap_platform_fuse_distribution,stage,0);
    will_return(__wrap_platform_fuse_distribution, CLI_SUCCESS);
    _fpfw_component_fuse_svc.init_fn();
}

TEST_FUNCTION(test_cli_fuse_init, NULL, NULL)
{
    expect_function_call(__wrap_platform_fuse_init_cli);
    will_return(__wrap_platform_fuse_init_cli, CLI_SUCCESS);
    _fpfw_component_cli_fuse.init_fn();
}
}