//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_mcp_clock_init.cpp
 * MCP PIK clock Init Test
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT...
#include <pik_clock_lib.h>
#include <stddef.h> // for NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_pik_clock;

/*------------- Functions ----------------*/
//
// Mocks
//
int __wrap_pik_clock_init(int pik_dev_cnt, const pik_clock_dev_config_t* pik_cfg_table, pik_clock_dev_ctx_t* pik_dev_ctx_mem)
{
    // Must use default table.
    assert_int_equal(pik_dev_cnt, 0);
    assert_null(pik_cfg_table);
    assert_null(pik_dev_ctx_mem);

    function_called();

    return mock_type(int);
}

int __wrap_pik_clock_power_transition(uint32_t dev_id, MOD_PD_STATE state)
{
    check_expected(dev_id);
    assert_true(state == MOD_PD_STATE_ON); // Only ON state is expected.

    function_called();

    return mock_type(int);
}

//
// Tests
//
TEST_FUNCTION(test_mcp_clock_init, nullptr, nullptr)
{
    will_return(__wrap_pik_clock_init, 0);
    expect_function_call(__wrap_pik_clock_init);

    expect_value(__wrap_pik_clock_power_transition, dev_id, PIK_DEV_ID_MCP_CORECLK);
    will_return(__wrap_pik_clock_power_transition, 0);
    expect_function_call(__wrap_pik_clock_power_transition);

    expect_value(__wrap_pik_clock_power_transition, dev_id, PIK_DEV_ID_MCP_AXICLK);
    will_return(__wrap_pik_clock_power_transition, 0);
    expect_function_call(__wrap_pik_clock_power_transition);

    // Call API under test
    fpfw_init_result_t result = _fpfw_component_pik_clock.init_fn();

    // Check expected return value
    assert_int_equal(result.status, FPFW_INIT_STATUS_SUCCESS);
}
}
