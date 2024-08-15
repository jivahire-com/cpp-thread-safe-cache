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
#include <idsw_kng.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_vab;
extern fpfw_init_component_t _fpfw_component_accel_vab_irq;

/*------------- Functions ----------------*/
KNG_PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(KNG_PLAT_ID);
}

KNG_DIE_ID __wrap_idsw_get_die_id(void)
{
    return mock_type(KNG_DIE_ID);
}

int __wrap_vab_common_init(uint16_t vab_instances_to_init)
{
    check_expected(vab_instances_to_init);
    // function_called();

    return 0;
}

void __wrap_enable_vab_isrs(uint16_t vab_instances_to_init)
{
    check_expected(vab_instances_to_init);
}

TEST_FUNCTION(test_vab_init_silicon_die0, nullptr, nullptr)
{
    const auto test_die = (KNG_DIE_ID)0;
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return_always(__wrap_idsw_get_die_id, test_die);
    expect_value(__wrap_vab_common_init,
                 vab_instances_to_init,
                 ((1 << D0_VAB0_RPSS0) | (1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) | (1 << D0_VAB3_RPSS3) |
                  (1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS)));
    _fpfw_component_vab.init_fn();
}

TEST_FUNCTION(test_vab_init_silicon_die1, nullptr, nullptr)
{
    const auto test_die = (KNG_DIE_ID)1;
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return_always(__wrap_idsw_get_die_id, test_die);
    expect_value(__wrap_vab_common_init,
                 vab_instances_to_init,
                 ((1 << D1_VAB0_RPSS0) | (1 << D1_VAB1_RPSS1) | (1 << D1_VAB2_RPSS2) | (1 << D1_VAB3_RPSS3) |
                  (1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS)));
    _fpfw_component_vab.init_fn();
}

TEST_FUNCTION(test_vab_init_fpga_die0, nullptr, nullptr)
{
    const auto test_die = (KNG_DIE_ID)0;
    will_return_always(__wrap_idsw_get_die_id, test_die);
    expect_value(__wrap_vab_common_init,
                 vab_instances_to_init,
                 ((1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) | (1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS)));

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    _fpfw_component_vab.init_fn();
}

TEST_FUNCTION(test_vab_init_fpga_large_die0, nullptr, nullptr)
{
    const auto test_die = (KNG_DIE_ID)0;
    will_return_always(__wrap_idsw_get_die_id, test_die);
    expect_value(__wrap_vab_common_init,
                 vab_instances_to_init,
                 ((1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) | (1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS)));
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE);
    _fpfw_component_vab.init_fn();
}

TEST_FUNCTION(test_vab_init_fpga_large_rvp_die0, nullptr, nullptr)
{
    const auto test_die = (KNG_DIE_ID)0;
    will_return_always(__wrap_idsw_get_die_id, test_die);
    expect_value(__wrap_vab_common_init,
                 vab_instances_to_init,
                 ((1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) | (1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS)));
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE_RVP);
    _fpfw_component_vab.init_fn();
}

TEST_FUNCTION(test_vab_init_fpga_die1, nullptr, nullptr)
{
    const auto test_die = (KNG_DIE_ID)1;
    will_return_always(__wrap_idsw_get_die_id, test_die);
    expect_value(__wrap_vab_common_init,
                 vab_instances_to_init,
                 ((1 << D1_VAB1_RPSS1) | (1 << D1_VAB2_RPSS2) | (1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS)));

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    _fpfw_component_vab.init_fn();
}

TEST_FUNCTION(test_vab_init_fpga_large_die1, nullptr, nullptr)
{
    const auto test_die = (KNG_DIE_ID)1;
    will_return_always(__wrap_idsw_get_die_id, test_die);
    expect_value(__wrap_vab_common_init,
                 vab_instances_to_init,
                 ((1 << D1_VAB1_RPSS1) | (1 << D1_VAB2_RPSS2) | (1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS)));
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE);
    _fpfw_component_vab.init_fn();
}

TEST_FUNCTION(test_vab_init_fpga_large_rvp_die1, nullptr, nullptr)
{
    const auto test_die = (KNG_DIE_ID)1;
    will_return_always(__wrap_idsw_get_die_id, test_die);
    expect_value(__wrap_vab_common_init,
                 vab_instances_to_init,
                 ((1 << D1_VAB1_RPSS1) | (1 << D1_VAB2_RPSS2) | (1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS)));
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE_RVP);
    _fpfw_component_vab.init_fn();
}

TEST_FUNCTION(test_vab_init_svp_die0, nullptr, nullptr)
{
    const auto test_die = (KNG_DIE_ID)0;
    will_return_always(__wrap_idsw_get_die_id, test_die);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    expect_value(__wrap_vab_common_init,
                 vab_instances_to_init,
                 ((1 << D0_VAB0_RPSS0) | (1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) | (1 << D0_VAB3_RPSS3) |
                  (1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS)));
    _fpfw_component_vab.init_fn();
}

TEST_FUNCTION(test_vab_init_svp_die1, nullptr, nullptr)
{
    const auto test_die = (KNG_DIE_ID)1;
    will_return_always(__wrap_idsw_get_die_id, test_die);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    expect_value(__wrap_vab_common_init, vab_instances_to_init, ((1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS)));
    _fpfw_component_vab.init_fn();
}

TEST_FUNCTION(test_vab_init_invalid_plat, nullptr, nullptr)
{
    const auto test_die = (KNG_DIE_ID)1;
    will_return_always(__wrap_idsw_get_die_id, test_die);
    will_return_always(__wrap_idsw_get_platform_sdv, 0x100);
    expect_value(__wrap_vab_common_init, vab_instances_to_init, 0);
    _fpfw_component_vab.init_fn();
}

TEST_FUNCTION(test_non_pcie_vab_isr_init_silicon_die0, nullptr, nullptr)
{
    will_return_always(__wrap_idsw_get_die_id, 0);
    expect_value(__wrap_enable_vab_isrs, vab_instances_to_init, ((1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS)));
    _fpfw_component_accel_vab_irq.init_fn();
}

TEST_FUNCTION(test_non_pcie_vab_isr_init_silicon_die1, nullptr, nullptr)
{
    will_return_always(__wrap_idsw_get_die_id, 1);
    expect_value(__wrap_enable_vab_isrs, vab_instances_to_init, ((1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS)));
    _fpfw_component_accel_vab_irq.init_fn();
}
}
