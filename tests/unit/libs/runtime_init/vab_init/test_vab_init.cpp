//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implement unit tests for vab initialization
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>     // for CmockaWrapperTest, TEST_FUNCTION, che...
#include <fpfw_init.h>         // for fpfw_init_component_t
#include <kng_soc_constants.h> // for D0_VAB0_RPSS0, D0_VAB1_RPSS1, D0_VAB2...
#include <stdint.h>            // for uint16_t

extern "C" {
#include <fpfw_init.h>
#include <idsw.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_vab;

/*------------- Functions ----------------*/
PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(PLAT_ID);
}

int __wrap_vab_init(uint16_t vab_instances_to_init)
{
    check_expected(vab_instances_to_init);
    function_called();

    return 0;
}

TEST_FUNCTION(test_vab_init_silicon, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    expect_value(__wrap_vab_init,
                 vab_instances_to_init,
                 ((1 << D0_VAB0_RPSS0) | (1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) | (1 << D0_VAB3_RPSS3)));
    expect_function_call(__wrap_vab_init);
    _fpfw_component_vab.init_fn();
}

TEST_FUNCTION(test_vab_init_fpga, nullptr, nullptr)
{
    expect_value_count(__wrap_vab_init, vab_instances_to_init, ((1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2)), 3);

    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    expect_function_call(__wrap_vab_init);
    _fpfw_component_vab.init_fn();

    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE);
    expect_function_call(__wrap_vab_init);
    _fpfw_component_vab.init_fn();

    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE_RVP);
    expect_function_call(__wrap_vab_init);
    _fpfw_component_vab.init_fn();
}

TEST_FUNCTION(test_vab_init_svp, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    expect_value(__wrap_vab_init, vab_instances_to_init, 0);
    expect_function_call(__wrap_vab_init);
    _fpfw_component_vab.init_fn();
}

TEST_FUNCTION(test_vab_init_invalid_plat, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_platform_sdv, 0x100);
    expect_value(__wrap_vab_init, vab_instances_to_init, 0);
    expect_function_call(__wrap_vab_init);
    _fpfw_component_vab.init_fn();
}
}
