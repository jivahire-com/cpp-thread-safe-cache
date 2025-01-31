//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Unit tests for PCIe component initialization layer
 */

/*------------- Includes -----------------*/

#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include <FpFwUtils.h>
#include <cmocka.h>
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <kng_soc_constants.h>
#include <pcie_dfwk.h>
#include <scp_pcie_manager.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_pcie;
extern fpfw_init_component_t _fpfw_component_pcie_cli;

/*------------- Functions ----------------*/
void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    uintptr_t host_ptr = 0xDEADBEEF;
    return (void*)(host_ptr);
}

idsw_plat_id_t __wrap_idsw_get_platform_sdv()
{
    return mock_type(idsw_plat_id_t);
}

void __wrap_pcie_cli_init(pciess_device_t* pcie_dev_handles)
{
    assert_non_null(pcie_dev_handles);
}

void __wrap_scp_pcie_initialize(PDFWK_SCHEDULE schedule, uint16_t rpss_to_init, KNG_DIE_ID die_id)
{
    assert_non_null(schedule);
    check_expected(rpss_to_init);
    check_expected(die_id);
}

KNG_DIE_ID __wrap_idsw_get_die_id(void)
{
    return mock_type(KNG_DIE_ID);
}
}

TEST_FUNCTION(test_die0_svp_init, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, ((1 << RPSS0) | (1 << RPSS1) | (1 << RPSS2) | (1 << RPSS3)));
    expect_value(__wrap_scp_pcie_initialize, die_id, DIE_0);
    _fpfw_component_pcie.init_fn();
}

TEST_FUNCTION(test_die1_svp_init, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return(__wrap_idsw_get_die_id, DIE_1);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, ((1 << RPSS4) | (1 << RPSS5) | (1 << RPSS6) | (1 << RPSS7)));
    expect_value(__wrap_scp_pcie_initialize, die_id, DIE_1);
    _fpfw_component_pcie.init_fn();
}

TEST_FUNCTION(test_die0_emu_init, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_EMU_1D);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, ((1 << RPSS0) | (1 << RPSS1) | (1 << RPSS2) | (1 << RPSS3)));
    expect_value(__wrap_scp_pcie_initialize, die_id, DIE_0);
    _fpfw_component_pcie.init_fn();
}

TEST_FUNCTION(test_die1_emu_init, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_EMU_2D);
    will_return(__wrap_idsw_get_die_id, DIE_1);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, ((1 << RPSS4) | (1 << RPSS5) | (1 << RPSS6) | (1 << RPSS7)));
    expect_value(__wrap_scp_pcie_initialize, die_id, DIE_1);
    _fpfw_component_pcie.init_fn();
}

TEST_FUNCTION(test_die0_fpga_init, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, ((1 << RPSS1) | (1 << RPSS2)));
    expect_value(__wrap_scp_pcie_initialize, die_id, DIE_0);
    _fpfw_component_pcie.init_fn();
}

TEST_FUNCTION(test_die1_fpga_init, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    will_return(__wrap_idsw_get_die_id, DIE_1);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, (1 << RPSS5));
    expect_value(__wrap_scp_pcie_initialize, die_id, DIE_1);
    _fpfw_component_pcie.init_fn();
}

TEST_FUNCTION(test_die0_silicon_init, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, ((1 << RPSS0) | (1 << RPSS1) | (1 << RPSS2) | (1 << RPSS3)));
    expect_value(__wrap_scp_pcie_initialize, die_id, DIE_0);
    _fpfw_component_pcie.init_fn();
}

TEST_FUNCTION(test_die1_silicon_init, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return(__wrap_idsw_get_die_id, DIE_1);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, ((1 << RPSS4) | (1 << RPSS5) | (1 << RPSS6) | (1 << RPSS7)));
    expect_value(__wrap_scp_pcie_initialize, die_id, DIE_1);
    _fpfw_component_pcie.init_fn();
}

TEST_FUNCTION(test_die0_unknown_plat_init, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, 0xFF);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, 0);
    expect_value(__wrap_scp_pcie_initialize, die_id, DIE_0);
    _fpfw_component_pcie.init_fn();
}

TEST_FUNCTION(test_cli_init, NULL, NULL)
{
    _fpfw_component_pcie_cli.init_fn();
}
