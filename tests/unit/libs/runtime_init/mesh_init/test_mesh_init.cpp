//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_mesh_init.cpp
 * Implement unit tests for mesh init
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for CmockaWrapperTest, TEST_FUNCTION, che...

extern "C" {
#include <FpFwUtils.h>     // for FPFW_UNUSED
#include <boot_status.h>   // for post_led_status, boot_status_notify_extd
#include <fpfw_icc_base.h> // for fpfw_icc_base_init, fpfw_icc_base_deinit
#include <fpfw_init.h>
#include <idhw.h>
#include <mesh.h>
#include <stdint.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_mesh_stg_1;
extern fpfw_init_component_t _fpfw_component_mesh_stg_2;
static uint32_t dummy_icc_ctx = 0;

/*------------- Functions ----------------*/
void* __wrap_fpfw_init_get_handle(const char* id)
{
    FPFW_UNUSED(id);
    return &dummy_icc_ctx;
}

int __wrap_mesh_init(uint8_t die_num, fpfw_icc_base_ctx_t* icc_ctx)
{
    check_expected(die_num);
    assert_non_null(icc_ctx);

    return SILIBS_SUCCESS;
}

int __wrap_d2d_init(uint8_t die_num, fpfw_icc_base_ctx_t* icc_ctx)
{
    check_expected(die_num);
    assert_non_null(icc_ctx);

    return SILIBS_SUCCESS;
}

KNG_DIE_ID __wrap_idhw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

void __wrap_post_led_status(boot_status_req_t* p_req_mem, led_status_codes_t status)
{
    assert_non_null(p_req_mem);
    check_expected(status);

    function_called();
}

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

void __wrap_boot_status_notify_extd(boot_status_req_t* p_req_mem, uint32_t boot_status, uint32_t boot_status_ex)
{
    check_expected(boot_status);
    assert_non_null(p_req_mem);
    check_expected(boot_status_ex);

    function_called();
}

uint8_t __wrap_idsw_get_cpu_type()
{
    return mock_type(uint8_t);
}

/*------------- Test Cases ----------------*/
TEST_FUNCTION(test_mesh_init, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return_always(__wrap_idhw_get_die_id, test_die);

    expect_value(__wrap_post_led_status, status, LED_STATUS_CODE_SCP_MESH_INIT_START);
    expect_function_call(__wrap_post_led_status);

    expect_value(__wrap_mesh_init, die_num, test_die);

    _fpfw_component_mesh_stg_1.init_fn();
}

TEST_FUNCTION(test_d2d_init, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return_always(__wrap_idhw_get_die_id, test_die);
    expect_value(__wrap_d2d_init, die_num, test_die);

    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_MESH_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    _fpfw_component_mesh_stg_2.init_fn();
}
}
