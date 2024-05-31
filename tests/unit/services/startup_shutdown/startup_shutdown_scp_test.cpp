//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file startup_shutdown_scp_test.cpp
 * Startup shutdown service core-specific (SCP) tests
 */

/*------------- Includes -----------------*/
#include "sos_test.h" // for POWER_TEST

#include <cstddef> // for NULL

extern "C" {

#include <CMockaWrapper.h>        // for check_expected, check_expected_ptr
#include <startup_shutdown_i.h>   // for sos_queue_start_phase, sos_thread_...
#include <startup_shutdown_ssi.h> // for _ssi_startup_stage, _startup_type

} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
extern "C" {
unsigned __real_sos_core_boot_stage_count();
const startup_shutdown_boot_stage_t* __real_sos_core_boot_stages();
} // extern "C"

//
// Tests
//

// test for sos_core_boot_stage_count()
SOS_TEST(sos_core_boot_stage_count, NULL, NULL)
{
    unsigned count = __real_sos_core_boot_stage_count();
    // ensure some number of boot stages are returned
    assert_int_not_equal(count, 0);
}

// test for sos_core_boot_stages
SOS_TEST(sos_core_boot_stages, NULL, NULL)
{
    const startup_shutdown_boot_stage_t* p_stages = __real_sos_core_boot_stages();
    // ensure the boot stages are returned
    assert_non_null(p_stages);
    // check we have stages from both boot phases
    assert_int_equal(p_stages[0].phase, STARTUP_PHASE_MSCP_ASYNC);
    assert_int_equal(p_stages[__real_sos_core_boot_stage_count() - 1].phase, STARTUP_PHASE_AP_ASYNC);
}

// test for sos_core_shutdown_handler
SOS_TEST(sos_core_shutdown_handler, NULL, NULL)
{
#define TEST_SHUTDOWN_TYPE (MSCP_SUBSYS_RESET)
    // TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1824038/
    //       provide a real test; currently the APi does nothing, so there are no expectations.
    sos_core_shutdown_handler(TEST_SHUTDOWN_TYPE);
}