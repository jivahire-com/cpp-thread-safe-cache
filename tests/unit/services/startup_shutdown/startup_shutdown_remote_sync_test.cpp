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
#include <mscp_exp_spi_synchronize_dies.h>

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

bool __wrap_idhw_is_single_die_boot_en(void)
{
    return mock_type(bool);
}

KNG_DIE_ID __wrap_idsw_get_die_id(void)
{
    return mock_type(KNG_DIE_ID);
}

int __wrap_mscp_exp_spi_synchronize_dies(mscp_exp_spi_sync_point_t sync_point, int die_id)
{
    FPFW_UNUSED(sync_point);
    FPFW_UNUSED(die_id);
    return mock_type(int);
}
} // extern "C"

// test for wait_for_remote_die_boot_stage
SOS_TEST(wait_for_remote_die_boot_stage_single_die, NULL, NULL)
{
    int status = 0;
    will_return(__wrap_idhw_is_single_die_boot_en, true);

    // call the function
    startup_shutdown_boot_stage_t test_stage = {STARTUP_MCP_LOAD, STARTUP_MCP_LOAD, 0, false, false};
    
    status = wait_for_remote_die_boot_stage(test_stage);

    assert_true(status);

}

SOS_TEST(wait_for_remote_die_boot_stage_dual_die, NULL, NULL)
{
    int status = 0;
    startup_shutdown_boot_stage_t test_stage = {STARTUP_MCP_LOAD, STARTUP_MCP_LOAD, 0, false, true};

    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);

    //! Will synchronize with the remote die for dual die boot
    will_return(__wrap_mscp_exp_spi_synchronize_dies, SILIBS_SUCCESS);

    //! call the function
    status = wait_for_remote_die_boot_stage(test_stage);

    assert_true(status);
}