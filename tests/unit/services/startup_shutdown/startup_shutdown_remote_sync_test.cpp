//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file startup_shutdown_thread_test.cpp
 * Startup shutdown service thread tests
 */

/*------------- Includes -----------------*/
#include "sos_test.h" // for POWER_TEST

#include <cstddef> // for NULL
#include <cstdint> // for uintptr_t

extern "C" {

#include <CMockaWrapper.h> // for check_expected, check_expected_ptr
#include <DfwkCommon.h>    // for DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE
#include <FpFwLinkedList.h>
#include <FpFwUtils.h>            // for FPFW_ARRAY_SIZE
#include <startup_shutdown.h>     // for sos_queue_start_phase, sos_thread_...
#include <startup_shutdown_i.h>   // for sos_queue_start_phase, sos_thread_...
#include <startup_shutdown_ssi.h> // for _ssi_startup_stage, _startup_type
#include <string.h>
#include <idhw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <tx_api.h> // for ULONG, UINT, VOID, TX_EVENT_FLAGS_...


} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/
#define SOS_THREAD_PRIORITY          (10)
#define SOS_THREAD_PREEMPT_THRESHOLD (10)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
extern "C" {

bool __wrap_idhw_is_single_die_boot_en(void)
{
    return mock_type(bool);
}

KNG_DIE_ID __wrap_idsw_get_die_id(void)
{
    return mock_type(KNG_DIE_ID);
}

extern void __real_remote_die_sync_init();
} // extern "C"


// test for rremote_die_sync_init
SOS_TEST(remote_die_sync_init, NULL, NULL)
{
    // expectations for ThreadX APIs
    expect_not_value(__wrap__txe_event_flags_create, event_flags_group_ptr, NULL);
    expect_any(__wrap__txe_event_flags_create, name_ptr);
    will_return(__wrap__txe_event_flags_create, TX_SUCCESS);

    // call the function
    __real_remote_die_sync_init();
}

// test for wait_for_remote_die_boot_stage
SOS_TEST(wait_for_remote_die_boot_stage_single_die, NULL, NULL)
{
    int status = 0;
    will_return(__wrap_idhw_is_single_die_boot_en, true);

    // call the function
    startup_shutdown_boot_stage_t test_stage = {STARTUP_MCP_LOAD, STARTUP_MCP_LOAD, 0, false, false};
    
    status = wait_for_remote_die_boot_stage(test_stage, test_stage);

    assert_true(status);

}

SOS_TEST(wait_for_remote_die_boot_stage_dual_die, NULL, NULL)
{
    int status = 0;
    startup_shutdown_boot_stage_t test_stage = {STARTUP_MCP_LOAD, STARTUP_MCP_LOAD, 0, false, true};

    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);

    expect_not_value(__wrap__txe_event_flags_set, event_flags_group_ptr, NULL);
    expect_value(__wrap__txe_event_flags_set, flags, (uint32_t)(1 << 30 | test_stage.stage));
    expect_value(__wrap__txe_event_flags_set, options, TX_OR);
    will_return(__wrap__txe_event_flags_set, TX_SUCCESS);

    expect_not_value(__wrap__txe_event_flags_get, group_ptr, NULL);
    expect_value(__wrap__txe_event_flags_get, requested_flags, (uint32_t)(1 << 30 | test_stage.stage));
    expect_value(__wrap__txe_event_flags_get, get_option, TX_AND_CLEAR);
    expect_not_value(__wrap__txe_event_flags_get, actual_flags_ptr, NULL);
    expect_not_value(__wrap__txe_event_flags_get, wait_option, TX_WAIT_FOREVER);
    will_return(__wrap__txe_event_flags_get, TX_SUCCESS);

    // call the function
    
    status = wait_for_remote_die_boot_stage(test_stage, test_stage);


    assert_true(status);
}