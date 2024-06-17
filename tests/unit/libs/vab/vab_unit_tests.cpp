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
#include <silibs_platform.h> // IWYU pragma: keep
#include <silibs_status.h>   // for SILIBS_SUCCESS, SILIBS_E_INIT
#include <stdint.h>          // for uint32_t

extern "C" {
#include <error_handler.h>
#include <vab.h> // for vab_init
}

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
TEST_FUNCTION(test_skip_all_vabs, NULL, NULL)
{

    int status;
    status = vab_common_init(0);
    assert_int_equal(status, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_successful_init_all_vabs, NULL, NULL)
{

    // int status;
    uint16_t vabs_to_init = 0xfff;

    // Setup expectations for all VABs
    for (uint16_t vab_id = 0; vab_id < MAX_VAB_INSTANCES; vab_id++)
    {
        if ((vabs_to_init >> vab_id) & 0x1)
        {
            expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
            will_return(__wrap_atu_map, SILIBS_SUCCESS);

            expect_value(__wrap_vab_init, vab_init_params->security_state, SECURITY_STATE_NON_SECURE);
            expect_value(__wrap_vab_init, vab_init_params->vab_smmu_gbpa_cfg->sh_cfg, 1);
            expect_value(__wrap_vab_init, vab_init_params->system_counter_delay, 0);
            expect_value(__wrap_vab_init, vab_init_params->vab_resolved_base_addr, 0xffffffff);

            expect_value(__wrap_atu_unmap, atu_id, ATU_ID_MSCP);
            will_return(__wrap_atu_unmap, SILIBS_SUCCESS);

            expect_function_call(__wrap_vab_init);
        }
    }
    vab_common_init(vabs_to_init);
}

TEST_FUNCTION(test_atu_map_fail, NULL, NULL)
{
    uint16_t vabs_to_init = 0xfff;
    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, SILIBS_E_INIT);
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        vab_common_init(vabs_to_init);
    }
}

TEST_FUNCTION(test_atu_unmap_fail, NULL, NULL)
{
    uint16_t vabs_to_init = 0x1;

    for (uint16_t vab_id = 0; vab_id < MAX_VAB_INSTANCES; vab_id++)
    {
        if ((vabs_to_init >> vab_id) & 0x1)
        {
            expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
            will_return(__wrap_atu_map, SILIBS_SUCCESS);

            expect_value(__wrap_vab_init, vab_init_params->security_state, SECURITY_STATE_NON_SECURE);
            expect_value(__wrap_vab_init, vab_init_params->vab_smmu_gbpa_cfg->sh_cfg, 1);
            expect_value(__wrap_vab_init, vab_init_params->system_counter_delay, 0);
            expect_value(__wrap_vab_init, vab_init_params->vab_resolved_base_addr, 0xffffffff);

            expect_value(__wrap_atu_unmap, atu_id, ATU_ID_MSCP);
            will_return(__wrap_atu_unmap, SILIBS_E_INIT);

            expect_function_call(__wrap_vab_init);
        }
    }

    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        vab_common_init(vabs_to_init);
    }
}