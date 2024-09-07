//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_polling_tests.cpp
 * DDR Manager Polling tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstddef>         // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <ddr_manager.h>
#include <ddr_manager_bwl.h>
#include <ddr_manager_i.h>
#include <ddr_manager_temp_interrupts.h>
#include <ddrss_lib.h>
#include <ddrss_runtime_api.h>
#include <FpFwUtils.h>
#include "common_mocks.h"
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
int __wrap_ddrss_refresh_rate_read_status(uint32_t mc, ddrss_refresh_rate_sts_t *refresh_rate_sts)
{
    assert_non_null(refresh_rate_sts);
    check_expected(mc);
    for (uint8_t dimm_rank = 0; dimm_rank < DDRSS_MAX_DIMM_RANK; dimm_rank++)
    {
        for (uint8_t sdram_dev = 0; sdram_dev < DDRSS_MAX_DIMM_SDRAM_DEV_PER_SUB_CH; sdram_dev++)
        {
            refresh_rate_sts->dram_temp[dimm_rank][sdram_dev] = mock_type(uint8_t);
        }
    }
    return 0;
}
}

//
// Fixtures
//
static int setup(void** state)
{
    FPFW_UNUSED(state);

    ddr_manager_disable_bwl_i3c();
    ddr_manager_disable_bwl_force();
    ddr_manager_disable_bwl_mr4();

    return 0;
}

//
// Tests
//
TEST_FUNCTION(test_ddr_manager_interrupts_critical_cb, NULL, NULL)
{
    // Arrange
    expect_function_call(__wrap_ddr_manager_set_thermal_trip_gpio);

    expect_any(__wrap__txe_queue_send, queue_ptr);
    ddr_manager_message_t msg_temp_changed = {.message_type = DDR_TEMP_CHANGED_EVENT, .message_data = 0};
    expect_memory(__wrap__txe_queue_send, source_ptr, &msg_temp_changed, sizeof(msg_temp_changed));
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    // Act
    ddr_manager_temp_interrupts_critical_cb(0);
}

TEST_FUNCTION(test_ddr_manager_interrupts_changed_cb, NULL, NULL)
{
    // Arrange
    expect_any(__wrap__txe_queue_send, queue_ptr);
    ddr_manager_message_t msg_temp_changed = {.message_type = DDR_TEMP_CHANGED_EVENT, .message_data = 0};
    expect_memory(__wrap__txe_queue_send, source_ptr, &msg_temp_changed, sizeof(msg_temp_changed));
    expect_value(__wrap__txe_queue_send, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    // Act
    ddr_manager_temp_interrupts_changed_cb(0);
}

TEST_FUNCTION(test_ddr_manager_temp_interrupts_process_changed_event, setup, NULL)
{
    // Arrange
    expect_value(__wrap_ddrss_refresh_rate_read_status, mc, 0);
    will_return_always(__wrap_ddrss_refresh_rate_read_status, 0);

    // Act
    ddr_manager_temp_interrupts_process_changed_event(0);

    // Assert
    assert_false(ddr_manager_get_bwl_engaged());
}

TEST_FUNCTION(test_ddr_manager_temp_interrupts_process_changed_event_high, setup, NULL)
{
    // Arrange
    expect_value(__wrap_ddrss_refresh_rate_read_status, mc, 0);
    will_return_always(__wrap_ddrss_refresh_rate_read_status, DDR_MR4_TEMP_85_90);

    // Act
    ddr_manager_temp_interrupts_process_changed_event(0);

    // Assert
    assert_true(ddr_manager_get_bwl_engaged());
}