//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_icc_cli_init.cpp
 * Test stub for ICC CLI initialization
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:icc_cli_init

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <fpfw_init.h>
#include <icc_cli.h>
#include <idhw.h>
#include <idsw.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_icc_cli;

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_icc_cli_init(icc_cli_init_params_t *params)
{
    FPFW_UNUSED(params);
    function_called();
}

KNG_PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(KNG_PLAT_ID);
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type()
{
    return mock_type(idsw_cpu_type_t);
}

bool __wrap_idhw_is_single_die_boot_en(void)
{
    return mock_type(bool);
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

//
// Tests
//
TEST_FUNCTION(test_icc_cli_init, nullptr, nullptr)
{
    // Set up expectations
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return(__wrap_idhw_is_single_die_boot_en, true);
    expect_function_call(__wrap_icc_cli_init);

    // Call API under test
    _fpfw_component_icc_cli.init_fn();
}
}