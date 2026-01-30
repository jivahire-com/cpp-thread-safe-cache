//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_shared_mem_init.cpp
 * Shared Memory init tests
 */

/*------------- Includes -----------------*/
#include "idsw.h"

#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include <FpFwUtils.h>
#include <atu_api.h>
#include <atu_lib.h>
#include <boot_status.h>
#include <fpfw_init.h>
#include <idsw_kng.h>
#include <system_info.h>

/*-- Symbolic Constant Macros (defines) --*/
#define VOYAGER_RSM_SIZE (0x10000)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

void* __real_memset(void* __a, int __b, size_t __c);

/*-- Declarations (Statics and globals) --*/

extern fpfw_init_component_t _fpfw_component_shared_mem;
static bool s_test_enabled = false;
static uint32_t mapped_rsm_addr = 0x12345678;

/*------------- Functions ----------------*/

//
// Mocks
//

idsw_plat_id_t __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(idsw_plat_id_t);
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

bool __wrap_system_info_is_warm_start(void)
{
    return mock_type(bool);
}

void* __wrap_memset(void* __a, int __b, size_t __c)
{
    if (s_test_enabled)
    {
        check_expected(__a);
        check_expected(__b);
        check_expected(__c);
        return NULL;
    }

    return __real_memset(__a, __b, __c);
}

int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    check_expected(atu_id);
    assert_non_null(atu_map_entry);

    atu_map_entry->mscp_start_address = (uint32_t)mapped_rsm_addr;

    function_called();

    return 0;
}

int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    check_expected(atu_id);
    assert_non_null(atu_map_entry);
    function_called();

    return 0;
}

static int test_setup(void** ctx)
{
    FPFW_UNUSED(ctx);

    s_test_enabled = true;

    return 0;
}

static int test_teardown(void** ctx)
{
    FPFW_UNUSED(ctx);

    s_test_enabled = false;

    return 0;
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
}

//
// Tests
//
TEST_FUNCTION(test_shared_mem_init_warm_boot, test_setup, test_teardown)
{
    // Set up expectations
    will_return(__wrap_system_info_is_warm_start, true);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_SHARED_MEM_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_shared_mem.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_arsm_init_cold_boot_d0, test_setup, test_teardown)
{
    // Set up expectations
    will_return(__wrap_system_info_is_warm_start, false);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);

    // ARSM memset expectations
    expect_value(__wrap_memset, __a, (void*)MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR);
    expect_value(__wrap_memset, __b, 0);
    expect_value(__wrap_memset, __c, MSCP_ATU_AP_WINDOW_ARSM_DIE_0_SIZE);

    // RSM memset expectations
    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    expect_function_call(__wrap_atu_map);

    expect_value(__wrap_memset, __a, (void*)mapped_rsm_addr);
    expect_value(__wrap_memset, __b, 0);
    expect_value(__wrap_memset, __c, VOYAGER_RSM_SIZE);

    expect_value(__wrap_atu_unmap, atu_id, ATU_ID_MSCP);
    expect_function_call(__wrap_atu_unmap);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_SHARED_MEM_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_shared_mem.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_arsm_init_cold_boot_d1, test_setup, test_teardown)
{
    // Set up expectations
    will_return(__wrap_system_info_is_warm_start, false);
    will_return_always(__wrap_idsw_get_die_id, DIE_1);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    expect_value(__wrap_memset, __a, (void*)MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR);
    expect_value(__wrap_memset, __b, 0);
    expect_value(__wrap_memset, __c, MSCP_ATU_AP_WINDOW_ARSM_DIE_0_SIZE);

    // RSM memset expectations
    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    expect_function_call(__wrap_atu_map);

    expect_value(__wrap_memset, __a, (void*)mapped_rsm_addr);
    expect_value(__wrap_memset, __b, 0);
    expect_value(__wrap_memset, __c, VOYAGER_RSM_SIZE);

    expect_value(__wrap_atu_unmap, atu_id, ATU_ID_MSCP);
    expect_function_call(__wrap_atu_unmap);

    const auto test_die = (KNG_DIE_ID)1;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_SHARED_MEM_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_shared_mem.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}

TEST_FUNCTION(test_arsm_init_svp_bypass, test_setup, test_teardown)
{
    // Set up expectations
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_SHARED_MEM_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_shared_mem.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_INIT_STATUS_SUCCESS);
    assert_null(result.associated_handle);
}
