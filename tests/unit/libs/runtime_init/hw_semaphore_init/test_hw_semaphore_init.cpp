//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_hw_version_init.cpp
 * Tests the init of the hw version component
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <FpFwUtils.h>
#include <boot_status.h>
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <semaphore_lib.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_hw_sem;

/*------------- Functions ----------------*/
//
// Mocks
//
bool __wrap_idhw_is_single_die_boot_en(void)
{
    return mock_type(bool);
}

int __wrap_set_semaphore_group_base(SEMAPHORE_GROUP_ID grp_id, uintptr_t base)
{
    FPFW_UNUSED(base);
    check_expected(grp_id);

    return mock_type(int);
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type()
{
    return mock_type(idsw_cpu_type_t);
}

void __wrap_initialize_semaphore(SEMAPHORE_ID id)
{
    FPFW_UNUSED(id);

    function_called();
}

void __wrap_boot_status_notify_extd(boot_status_req_t* p_req_mem, uint32_t boot_status, uint32_t boot_status_ex)
{
    check_expected(boot_status);
    assert_non_null(p_req_mem);
    check_expected(boot_status_ex);

    function_called();
}

//
// Tests
//

TEST_FUNCTION(test_hw_semaphore_init_dual_die_scp, nullptr, nullptr)
{
    // Set up expectations
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    expect_value(__wrap_set_semaphore_group_base, grp_id, SEM_GRP_ID_DIE0_IOSS);
    will_return(__wrap_set_semaphore_group_base, 0);
    expect_value(__wrap_set_semaphore_group_base, grp_id, SEM_GRP_ID_DIE1_IOSS);
    will_return(__wrap_set_semaphore_group_base, 0);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    expect_function_calls(__wrap_initialize_semaphore, 16);

    // Check dependencies
    assert_string_equal("hw_ver", _fpfw_component_hw_sem.children[0]);
    assert_string_equal("atu_svc", _fpfw_component_hw_sem.children[1]);
    assert_string_equal("mesh_stg_2", _fpfw_component_hw_sem.children[2]);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_HW_SEM_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call API under test
    _fpfw_component_hw_sem.init_fn();
}

TEST_FUNCTION(test_hw_semaphore_init_single_die_scp, nullptr, nullptr)
{
    // Set up expectations
    will_return(__wrap_idhw_is_single_die_boot_en, true);
    expect_value(__wrap_set_semaphore_group_base, grp_id, SEM_GRP_ID_DIE0_IOSS);
    will_return(__wrap_set_semaphore_group_base, 0);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    expect_function_calls(__wrap_initialize_semaphore, 8);

    // Check dependencies
    assert_string_equal("hw_ver", _fpfw_component_hw_sem.children[0]);
    assert_string_equal("atu_svc", _fpfw_component_hw_sem.children[1]);
    assert_string_equal("mesh_stg_2", _fpfw_component_hw_sem.children[2]);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_HW_SEM_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call API under test
    _fpfw_component_hw_sem.init_fn();
}

TEST_FUNCTION(test_hw_semaphore_init_dual_die_mcp, nullptr, nullptr)
{
    // Set up expectations
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    expect_value(__wrap_set_semaphore_group_base, grp_id, SEM_GRP_ID_DIE0_IOSS);
    will_return(__wrap_set_semaphore_group_base, 0);
    expect_value(__wrap_set_semaphore_group_base, grp_id, SEM_GRP_ID_DIE1_IOSS);
    will_return(__wrap_set_semaphore_group_base, 0);
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);

    // Check dependencies
    assert_string_equal("hw_ver", _fpfw_component_hw_sem.children[0]);
    assert_string_equal("atu_svc", _fpfw_component_hw_sem.children[1]);
    assert_string_equal("mesh_stg_2", _fpfw_component_hw_sem.children[2]);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_MCP, MSCP_GENERIC, (test_die == DIE_0) ? MCP_PRIMARY : MCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_MCP_HW_SEM_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call API under test
    _fpfw_component_hw_sem.init_fn();
}
}
