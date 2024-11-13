//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_cfg_mgr_init.cpp
 * Config Manger Init test
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
// #include "debug.h" // for UNUSED

#include <DfwkDriver.h>
#include <DfwkThreadXHost.h>
#include <fpfw_cfg_mgr.h>
#include <fpfw_cfg_mgr_init.h>
#include <fpfw_init.h>
#include <idsw_kng.h>
#include <mscp_exp_rmss_memory_map.h>
#include <variable_services.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_cfg_mgr;
extern fpfw_init_component_t _fpfw_component_cfg_mgr_cli;

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_cfg_mgr_init(fpfw_cfg_mgr_config_t* cfg_mgr_config, var_service_shared_mem_t* var_svc_mem_ctx)
{
    assert_non_null(cfg_mgr_config->read_knob_fn);
    assert_non_null(cfg_mgr_config->write_knob_fn);
    assert_true(var_svc_mem_ctx->max_payload_size == SCP_EXP_SCP_CFGMGR_VARIABLE_SERVICE_PAYLOAD_SIZE);
    assert_true(var_svc_mem_ctx->payload_base == (uintptr_t)SCP_EXP_SCP_CFGMGR_VARIABLE_SERVICE_PAYLOAD_BASE);
    function_called();
}

void __wrap_cfg_mgr_cli_init()
{
    function_called();
}


KNG_PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(KNG_PLAT_ID);
}

//
// Tests
//
TEST_FUNCTION(test_cfg_mgr_init, nullptr, nullptr)
{
    // Set up expectations
    expect_function_call(__wrap_cfg_mgr_init);
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);

    // Call API under test
    _fpfw_component_cfg_mgr.init_fn();
}

TEST_FUNCTION(test_cfg_mgr_cli_init, nullptr, nullptr)
{
    // Set up expectations
    expect_function_call(__wrap_cfg_mgr_cli_init);

    // Check dependencies
    assert_string_equal("cfg_mgr", _fpfw_component_cfg_mgr_cli.children[0]);
    assert_string_equal("cli", _fpfw_component_cfg_mgr_cli.children[1]);

    // Call API under test
    _fpfw_component_cfg_mgr_cli.init_fn();
}
}