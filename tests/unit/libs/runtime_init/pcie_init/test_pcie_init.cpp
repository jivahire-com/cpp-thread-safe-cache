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
#include <scp_pcie_manager.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_pcie;

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

void __wrap_scp_pcie_initialize(PDFWK_SCHEDULE schedule, uint16_t rpss_to_init)
{
    assert_non_null(schedule);
    check_expected(rpss_to_init);
}
}

TEST_FUNCTION(test_die0_svp_init, NULL, NULL)
{
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, ((1 << RPSS0) | (1 << RPSS1) | (1 << RPSS2) | (1 << RPSS3)));
    _fpfw_component_pcie.init_fn();
}

TEST_FUNCTION(test_die0_fpga_init, NULL, NULL)
{
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, ((1 << RPSS1) | (1 << RPSS2)));
    _fpfw_component_pcie.init_fn();
}

TEST_FUNCTION(test_di0_silicon_init, NULL, NULL)
{
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, ((1 << RPSS0) | (1 << RPSS1) | (1 << RPSS2) | (1 << RPSS3)));
    _fpfw_component_pcie.init_fn();
}

TEST_FUNCTION(test_die0_unknown_plat_init, NULL, NULL)
{
    will_return(__wrap_idsw_get_platform_sdv, 0xFF);
    expect_value(__wrap_scp_pcie_initialize, rpss_to_init, 0);
    _fpfw_component_pcie.init_fn();
}
