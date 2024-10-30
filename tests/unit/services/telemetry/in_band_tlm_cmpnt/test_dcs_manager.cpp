//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_dcs_manager.cpp
 * Test manager for data collection service
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:telemetry

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {

#include <dcs_manager_i.h>
#include <ddr_manager_i.h>

#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <in_band_tlm_cmpnt_i.h>
#include <package_creation_i.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t



}
/*-- Symbolic Constant Macros (defines) --*/


/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/


/*-------- Function Prototypes -----------*/

static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);
    return 0;
}

static int test_teardown(void** pContext)
{
    FPFW_UNUSED(pContext);
    return 0;
}

TEST_FUNCTION(test_dcs_manager_queue_tlm_package, test_setup, test_teardown)
{
    tlm_package_t tlm_pkg = {0xaabb,0xaabb};

    // no receive data, successfully queues in pending
    expect_value(__wrap__txe_queue_receive, queue_ptr, &dcs_pkg_pending_queue);
    expect_any(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);

    will_return(__wrap__txe_queue_receive, sizeof(tlm_package_t));
    will_return(__wrap__txe_queue_receive, &tlm_pkg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    dcs_manager_queue_tlm_package(0x1234, 0x5678);

    // receive data, de-allocates memory, fails to queue in pending

    expect_value(__wrap__txe_queue_receive, queue_ptr, &dcs_pkg_pending_queue);
    expect_any(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);

    will_return(__wrap__txe_queue_receive, sizeof(tlm_package_t));
    will_return(__wrap__txe_queue_receive, &tlm_pkg);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    expect_value(__wrap__txe_queue_receive, queue_ptr, &dcs_pkg_pending_queue);
    expect_any(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);

    will_return(__wrap__txe_queue_receive, sizeof(tlm_package_t));
    will_return(__wrap__txe_queue_receive, &tlm_pkg);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_QUEUE_ERROR);

    dcs_manager_queue_tlm_package(0x1234, 0x5678);
}