//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_ddr_manager.cpp
 * Test DDR memory management
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:telemetry

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <ddr_manager_i.h>
#include <in_band_tlm_cmpnt_i.h>
#include <package_creation_i.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t
}
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/
uint8_t max_package_mem[POWER_PKG_MAX_SIZE] = {0};

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

TEST_FUNCTION(test_ddr_mgr_allocate_mem_for_pwr_report, test_setup, test_teardown)
{
    uintptr_t pkg_location = 0;
    size_t available_size = 0;

    expect_value(__wrap__txe_queue_receive, queue_ptr, &pwr_pkg_free_queue);
    expect_value(__wrap__txe_queue_receive, destination_ptr, &pkg_location);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);

    will_return(__wrap__txe_queue_receive, sizeof(uint32_t));
    will_return(__wrap__txe_queue_receive, max_package_mem);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    fpfw_status_t status = ddr_manager_allocate_mem_for_pwr_pkg(&pkg_location, &available_size);

    assert_true(FPFW_STATUS_SUCCEEDED(status));
    assert_int_equal(available_size, POWER_POOL_BLOCK_SIZE);

    pkg_location = 0;
    available_size = 0;

    expect_value(__wrap__txe_queue_receive, queue_ptr, &pwr_pkg_free_queue);
    expect_value(__wrap__txe_queue_receive, destination_ptr, &pkg_location);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);

    will_return(__wrap__txe_queue_receive, sizeof(uint32_t));
    will_return(__wrap__txe_queue_receive, max_package_mem);
    will_return(__wrap__txe_queue_receive, TX_NOT_AVAILABLE);

    status = ddr_manager_allocate_mem_for_pwr_pkg(&pkg_location, &available_size);

    assert_false(FPFW_STATUS_SUCCEEDED(status));
    assert_int_equal(available_size, 0);
}

TEST_FUNCTION(test_ddr_mgr_allocate_mem_for_inst_report, test_setup, test_teardown)
{
    uintptr_t pkg_location = 0;
    size_t available_size = 0;

    expect_value(__wrap__txe_queue_receive, queue_ptr, &inst_pkg_free_queue);
    expect_value(__wrap__txe_queue_receive, destination_ptr, &pkg_location);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);

    will_return(__wrap__txe_queue_receive, sizeof(uint32_t));
    will_return(__wrap__txe_queue_receive, max_package_mem);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    fpfw_status_t status = ddr_manager_allocate_mem_for_inst_pkg(&pkg_location, &available_size);

    assert_true(FPFW_STATUS_SUCCEEDED(status));
    assert_int_equal(available_size, INST_POOL_BLOCK_SIZE);

    pkg_location = 0;
    available_size = 0;

    expect_value(__wrap__txe_queue_receive, queue_ptr, &inst_pkg_free_queue);
    expect_value(__wrap__txe_queue_receive, destination_ptr, &pkg_location);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);

    will_return(__wrap__txe_queue_receive, sizeof(uint32_t));
    will_return(__wrap__txe_queue_receive, max_package_mem);
    will_return(__wrap__txe_queue_receive, TX_NOT_AVAILABLE);

    status = ddr_manager_allocate_mem_for_inst_pkg(&pkg_location, &available_size);

    assert_false(FPFW_STATUS_SUCCEEDED(status));
    assert_int_equal(available_size, 0);
}

TEST_FUNCTION(test_ddr_manager_deallocate_mem, test_setup, test_teardown)
{
    // valid pwr block
    expect_value(__wrap__txe_queue_send, queue_ptr, &pwr_pkg_free_queue);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    uintptr_t pkg_mem = POWER_POOL_MEM_START;
    ddr_manager_deallocate_mem(&pkg_mem);

    // valid inst block
    expect_value(__wrap__txe_queue_send, queue_ptr, &inst_pkg_free_queue);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    pkg_mem = INST_POOL_MEM_START;
    ddr_manager_deallocate_mem(&pkg_mem);

    // invalid pwr block
    pkg_mem = POWER_POOL_MEM_START + POWER_POOL_TOTAL_SIZE;
    ddr_manager_deallocate_mem(&pkg_mem);

    // invalid inst block
    pkg_mem = INST_POOL_MEM_START + INST_POOL_TOTAL_SIZE;
    ddr_manager_deallocate_mem(&pkg_mem);

    // fail to queue free block
    expect_value(__wrap__txe_queue_send, queue_ptr, &inst_pkg_free_queue);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_QUEUE_ERROR);

    pkg_mem = INST_POOL_MEM_START;
    ddr_manager_deallocate_mem(&pkg_mem);
}
