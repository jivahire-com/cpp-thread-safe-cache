//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file startup_shutdown_local_sync_test.cpp
 * Tests the local core synchronization functionality of the startup shutdown service.
 */

/*------------- Includes -----------------*/

#include "sos_test.h" // for POWER_TEST

#include <CMockaWrapper.h> // for check_expected, check_expected_ptr

extern "C" {
#include <FpFwUtils.h> // for FPFW_ARRAY_SIZE
#include <idhw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <semaphore_lib.h>
#include <startup_shutdown.h> // for sos_queue_start_phase, sos_thread_...
#include <startup_shutdown_i.h>
#include <string.h>
} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

static bool update_remote_core_on_sleep = false;
static bool mock_sdv = false;

/*------------- Functions ----------------*/

//
// Mocks
//
extern "C" {

extern uint32_t test_memory[2];

uint32_t remote_value = 0;

idsw_cpu_type_t __wrap_idsw_get_cpu_type()
{
    return mock_type(idsw_cpu_type_t);
}

// Provide the signature for the real version and provide the implementation for the wrapped version
idsw_plat_id_t __real_idsw_get_platform_sdv();
idsw_plat_id_t __wrap_idsw_get_platform_sdv()
{
    if (mock_sdv)
    {
        return mock_type(idsw_plat_id_t);
    }
    return __real_idsw_get_platform_sdv();
}

void __wrap_wait_for_semaphore(SEMAPHORE_ID id, uint32_t key)
{
    FPFW_UNUSED(id);
    FPFW_UNUSED(key);
}

void __wrap_release_semaphore(SEMAPHORE_ID id)
{
    FPFW_UNUSED(id);
}

UINT __wrap__tx_thread_sleep(ULONG timer_ticks)
{
    FPFW_UNUSED(timer_ticks);

    if (update_remote_core_on_sleep)
    {
        // Simulate a remote core update on sleep
        test_memory[1] = remote_value;
    }

    return mock_type(UINT);
}

static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);

    update_remote_core_on_sleep = false;
    mock_sdv = false;
    memset(test_memory, 0, sizeof(test_memory));

    return 0;
}

static int test_teardown(void** pContext)
{
    FPFW_UNUSED(pContext);
    return 0;
}

} // extern "C"

SOS_TEST(wait_for_local_core_boot_stage_not_needed, test_setup, test_teardown)
{
    startup_shutdown_boot_stage_t stage = {
        .phase = STARTUP_PHASE_MSCP_ASYNC,
        .stage = STARTUP_AP_SOC_POWER_INIT_POST_SYNC,
        .local_core_sync_required = false,
        .remote_die_sync_required = false,
    };

    bool result = wait_for_local_core_boot_stage(stage);

    assert_true(result);
}

SOS_TEST(wait_for_local_core_boot_stage_needed_but_emulation_skips, test_setup, test_teardown)
{
    mock_sdv = true;
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_EMU);

    startup_shutdown_boot_stage_t stage = {
        .phase = STARTUP_PHASE_MSCP_ASYNC,
        .stage = STARTUP_AP_SOC_POWER_INIT_POST_SYNC,
        .local_core_sync_required = true,
        .remote_die_sync_required = false,
    };

    bool result = wait_for_local_core_boot_stage(stage);

    assert_true(result);
}

SOS_TEST(wait_for_local_core_boot_stage_needed_scp_no_sleep, test_setup, test_teardown)
{

    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    test_memory[1] = STARTUP_AP_SOC_POWER_INIT_POST_SYNC;

    startup_shutdown_boot_stage_t stage = {
        .phase = STARTUP_PHASE_MSCP_ASYNC,
        .stage = STARTUP_AP_SOC_POWER_INIT_POST_SYNC,
        .local_core_sync_required = true,
        .remote_die_sync_required = false,
    };

    bool result = wait_for_local_core_boot_stage(stage);

    assert_true(result);
}

SOS_TEST(wait_for_local_core_boot_stage_needed_mcp_no_sleep, test_setup, test_teardown)
{

    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    test_memory[0] = STARTUP_AP_SOC_POWER_INIT_POST_SYNC;

    startup_shutdown_boot_stage_t stage = {
        .phase = STARTUP_PHASE_MSCP_ASYNC,
        .stage = STARTUP_AP_SOC_POWER_INIT_POST_SYNC,
        .local_core_sync_required = true,
        .remote_die_sync_required = false,
    };

    bool result = wait_for_local_core_boot_stage(stage);

    assert_true(result);
}

SOS_TEST(wait_for_local_core_boot_stage_needed_scp_sleep_needed, test_setup, test_teardown)
{

    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap__tx_thread_sleep, TX_SUCCESS);
    remote_value = STARTUP_AP_SOC_POWER_INIT_POST_SYNC;
    update_remote_core_on_sleep = true;

    startup_shutdown_boot_stage_t stage = {
        .phase = STARTUP_PHASE_MSCP_ASYNC,
        .stage = STARTUP_AP_SOC_POWER_INIT_POST_SYNC,
        .local_core_sync_required = true,
        .remote_die_sync_required = false,
    };

    bool result = wait_for_local_core_boot_stage(stage);

    assert_true(result);
}
