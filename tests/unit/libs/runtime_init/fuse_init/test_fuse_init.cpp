//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_ap_core_init.cpp
 * Tests the init of the power service
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {
#include <DfwkDriver.h> // for DFWK_SCHEDULE
#include <FpFwCli.h>    // for _FPFW_CLI_CONFIG, CLI_SUCCESS, PFPFW_CLI_...
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <boot_status.h>
#include <fpfw_init.h> // for fpfw_init_component_t
#include <fuse_client.h>
#include <fuse_init.h>
#include <idsw_kng.h>
#include <kng_soc_constants.h> // for NUM_AP_CORES_PER_DIE
#include <stdint.h>
#include <string.h> // for memcpy

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern fpfw_init_component_t _fpfw_component_fuse_pre_mesh;
extern fpfw_init_component_t _fpfw_component_fuse_post_mesh;
extern fpfw_init_component_t _fpfw_component_cli_fuse;
extern fpfw_init_component_t _fpfw_component_fuse_en;

/*------------- Functions ----------------*/
//
// Mocks
//
fpfw_icc_base_ctx_t* __wrap_fpfw_init_get_handle(const char* name)
{
    check_expected_ptr(name);
    function_called();
    return mock_ptr_type(fpfw_icc_base_ctx_t*);
}

void __wrap_fuse_init(fpfw_icc_base_ctx_t* icc_base_ctx)
{
    check_expected_ptr(icc_base_ctx);
}

int __wrap_platform_fuse_override(void)
{
    return mock_type(int);
}

int __wrap_platform_fuse_distribution(int stage)
{
    check_expected(stage);
    return mock_type(int);
}

FPFW_CLI_STATUS __wrap_platform_fuse_init_cli(void)
{
    function_called();
    return mock_type(FPFW_CLI_STATUS);
}

void __wrap_fuse_feature_enable(const bool enable)
{
    check_expected(enable);
}

void __wrap_fuse_post_mesh_init(fpfw_icc_base_ctx_t* icc_die2die_ctx)
{
    FPFW_UNUSED(icc_die2die_ctx);
    function_called();
}

bool __wrap_idhw_is_single_die_boot_en()
{
    return mock_type(bool);
}

void __wrap_boot_status_notify_extd(boot_status_req_t* p_req_mem, uint32_t boot_status, uint32_t boot_status_ex)
{
    check_expected(boot_status);
    assert_non_null(p_req_mem);
    check_expected(boot_status_ex);

    function_called();
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

/* Tests */

TEST_FUNCTION(test_fuse_pre_mesh, NULL, NULL)
{
    fpfw_icc_base_ctx_t* dummy_icc_hspmbx_ctx = reinterpret_cast<fpfw_icc_base_ctx_t*>(1);
    // Mock ICC context initialization
    expect_string(__wrap_fpfw_init_get_handle, name, "icc_hspmbx");
    will_return(__wrap_fpfw_init_get_handle, dummy_icc_hspmbx_ctx);
    expect_function_call(__wrap_fpfw_init_get_handle);
    expect_value(__wrap_fuse_init, icc_base_ctx, dummy_icc_hspmbx_ctx);
    will_return(__wrap_platform_fuse_override, CLI_SUCCESS);
    expect_value(__wrap_platform_fuse_distribution, stage, FUSE_DISTRIBUTION_STAGE_POST_HSP);
    will_return(__wrap_platform_fuse_distribution, 0);
    expect_value(__wrap_platform_fuse_distribution, stage, FUSE_DISTRIBUTION_STAGE_POST_HSP_MESH_INIT);
    will_return(__wrap_platform_fuse_distribution, 0);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_PRE_FUSE_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    _fpfw_component_fuse_pre_mesh.init_fn();
}

TEST_FUNCTION(test_fuse_post_mesh, NULL, NULL)
{
    fpfw_icc_base_ctx_t* dummy_icc_d2dmbx_ctx = reinterpret_cast<fpfw_icc_base_ctx_t*>(2);

    // Mock ICC context initialization
    expect_value(__wrap_platform_fuse_distribution, stage, FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT);
    will_return(__wrap_platform_fuse_distribution, 0);
    expect_value(__wrap_platform_fuse_distribution, stage, FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT_BRIDGE_INIT);
    will_return(__wrap_platform_fuse_distribution, 0);

    expect_function_call(__wrap_fpfw_init_get_handle);
    expect_string(__wrap_fpfw_init_get_handle, name, "icc_die2die");
    will_return(__wrap_fpfw_init_get_handle, dummy_icc_d2dmbx_ctx);
    will_return_always(__wrap_idhw_is_single_die_boot_en, true);

    expect_function_call(__wrap_fuse_post_mesh_init);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_POST_FUSE_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    _fpfw_component_fuse_post_mesh.init_fn();
}

TEST_FUNCTION(test_cli_fuse_init, NULL, NULL)
{
    expect_function_call(__wrap_platform_fuse_init_cli);
    will_return(__wrap_platform_fuse_init_cli, CLI_SUCCESS);
    _fpfw_component_cli_fuse.init_fn();
}

TEST_FUNCTION(test_fuse_en, NULL, NULL)
{
    // Mock fuse feature enable
    expect_value(__wrap_fuse_feature_enable, enable, true);
    _fpfw_component_fuse_en.init_fn();
}
}