//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Unit tests for the common VAB initialization library.
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for will_return, CmockaWrapperTest, TEST_FUNC...
#include <atu_lib.h>
#include <cstddef> // for NULL
#include <kng_soc_constants.h>
#include <silibs_ap_top_regs.h>
#include <silibs_platform.h> // IWYU pragma: keep
#include <silibs_status.h>   // for SILIBS_SUCCESS, SILIBS_E_INIT
#include <stdint.h>          // for uint32_t
#include <vab_cded_ioss_top_regs.h>
#include <vab_regs.h>
#include <vab_rpss_top_regs.h>
#include <vab_sdm_top_regs.h>

extern "C" {
#include <error_handler.h>
#include <vab.h> // for vab_init
}

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static uint64_t expected_base_addresses[MAX_VAB_INSTANCES] = {
    (AP_TOP_D0_VAB_RPSS0_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS),
    (AP_TOP_D0_VAB_RPSS1_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS),
    (AP_TOP_D0_VAB_RPSS2_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS),
    (AP_TOP_D0_VAB_RPSS3_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS),
    (AP_TOP_D1_VAB_RPSS0_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS),
    (AP_TOP_D1_VAB_RPSS1_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS),
    (AP_TOP_D1_VAB_RPSS2_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS),
    (AP_TOP_D1_VAB_RPSS3_ADDRESS + VAB_RPSS_TOP_VAB_ADDRESS),
    (AP_TOP_D0_VAB_SDM_ADDRESS + VAB_SDM_TOP_VAB_ADDRESS),
    (AP_TOP_D1_VAB_SDM_ADDRESS + VAB_SDM_TOP_VAB_ADDRESS),
    (AP_TOP_D0_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_VAB_ADDRESS),
    (AP_TOP_D1_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_VAB_ADDRESS)};

/*------------- Functions ----------------*/
TEST_FUNCTION(test_svp_skip, NULL, NULL)
{

    int status;
    status = vab_init(0);
    assert_int_equal(status, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_successful_init_all_vabs, NULL, NULL)
{

    int status;
    uint16_t vabs_to_init = 0;

    /* Setup expectations for all VABs */
    expect_value_count(__wrap_atu_map, atu_id, ATU_ID_MSCP, MAX_VAB_INSTANCES);
    expect_value_count(__wrap_tower_configure_vab_apu, tower_base_addr, VAB_VAB_TOWER_ADDRESS, MAX_VAB_INSTANCES);
    expect_value_count(__wrap_configure_vab_system_addr_map, tower_base_addr, VAB_VAB_TOWER_ADDRESS, MAX_VAB_INSTANCES);
    expect_value_count(__wrap_atu_unmap, atu_id, ATU_ID_MSCP, MAX_VAB_INSTANCES);
    for (uint8_t i = 0; i < MAX_VAB_INSTANCES; i++)
    {
        vabs_to_init |= (1 << i);
        will_return(__wrap_atu_map, SILIBS_SUCCESS);
        expect_value(__wrap_tower_configure_vab_apu, vab_base_addr, expected_base_addresses[i]);
        expect_value(__wrap_configure_vab_system_addr_map, vab_base_addr, expected_base_addresses[i]);
        expect_value(__wrap_deassert_pcr_reset, vab_pcr_base_addr, VAB_VAB_PCR_TOP_ADDRESS);
        expect_value(__wrap_vab_pcr_init, vab_pcr_base_addr, VAB_VAB_PCR_TOP_ADDRESS);
        will_return(__wrap_vab_pcr_init, SILIBS_SUCCESS);
        will_return(__wrap_atu_unmap, SILIBS_SUCCESS);
    }
    status = vab_init(vabs_to_init);
    assert_int_equal(status, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_atu_map_fail, NULL, NULL)
{
    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, SILIBS_E_INIT);
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        vab_init(1);
    }
}

TEST_FUNCTION(test_pcr_init_fail, NULL, NULL)
{
    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);

    expect_value(__wrap_tower_configure_vab_apu, vab_base_addr, expected_base_addresses[0]);
    expect_value(__wrap_tower_configure_vab_apu, tower_base_addr, VAB_VAB_TOWER_ADDRESS);

    expect_value(__wrap_configure_vab_system_addr_map, vab_base_addr, expected_base_addresses[0]);
    expect_value(__wrap_configure_vab_system_addr_map, tower_base_addr, VAB_VAB_TOWER_ADDRESS);

    expect_value(__wrap_deassert_pcr_reset, vab_pcr_base_addr, VAB_VAB_PCR_TOP_ADDRESS);

    expect_value(__wrap_vab_pcr_init, vab_pcr_base_addr, VAB_VAB_PCR_TOP_ADDRESS);
    will_return(__wrap_vab_pcr_init, SILIBS_E_PARAM);

    expect_value(__wrap_atu_unmap, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        vab_init(1);
    }
}

TEST_FUNCTION(test_atu_unmap_fail, NULL, NULL)
{
    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);

    expect_value(__wrap_tower_configure_vab_apu, vab_base_addr, expected_base_addresses[0]);
    expect_value(__wrap_tower_configure_vab_apu, tower_base_addr, VAB_VAB_TOWER_ADDRESS);

    expect_value(__wrap_configure_vab_system_addr_map, vab_base_addr, expected_base_addresses[0]);
    expect_value(__wrap_configure_vab_system_addr_map, tower_base_addr, VAB_VAB_TOWER_ADDRESS);

    expect_value(__wrap_deassert_pcr_reset, vab_pcr_base_addr, VAB_VAB_PCR_TOP_ADDRESS);

    expect_value(__wrap_vab_pcr_init, vab_pcr_base_addr, VAB_VAB_PCR_TOP_ADDRESS);
    will_return(__wrap_vab_pcr_init, SILIBS_SUCCESS);

    expect_value(__wrap_atu_unmap, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_unmap, SILIBS_E_INIT);

    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        vab_init(1);
    }
}
