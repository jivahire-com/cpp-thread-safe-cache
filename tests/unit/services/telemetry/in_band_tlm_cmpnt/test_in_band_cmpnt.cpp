//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_ddr_manager.cpp
 * Test entry into in band component
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:telemetry

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <dcs_manager_i.h>
#include <ddr_manager_i.h>
#include <in_band_tlm_cmpnt.h>
#include <in_band_tlm_cmpnt_i.h>
#include <package_creation_i.h>
#include <stdint.h> // for uint32_t, uint64_t, int32_t
}
/*-- Symbolic Constant Macros (defines) --*/
#define DIE_ID (0)
/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/
uint8_t ib_max_package_mem[POWER_PKG_MAX_SIZE] = {0};

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

TEST_FUNCTION(test_in_band_cmpnt_init, test_setup, test_teardown)
{
    expect_any_always(__wrap__txe_queue_create, queue_ptr);
    expect_any_always(__wrap__txe_queue_create, name_ptr);
    expect_any_always(__wrap__txe_queue_create, message_size);
    expect_any_always(__wrap__txe_queue_create, queue_start);
    expect_any_always(__wrap__txe_queue_create, queue_size);
    expect_any_always(__wrap__txe_queue_create, queue_control_block_size);
    will_return_always(__wrap__txe_queue_create, TX_SUCCESS);

    expect_any_always(__wrap__txe_queue_flush, queue_ptr);
    will_return_always(__wrap__txe_queue_flush, TX_SUCCESS);

    expect_any_always(__wrap__txe_queue_send, queue_ptr);
    expect_any_always(__wrap__txe_queue_send, source_ptr);
    expect_any_always(__wrap__txe_queue_send, wait_option);
    will_return_always(__wrap__txe_queue_send, TX_SUCCESS);

    expect_function_calls(__wrap_FpFwAssertWithArgs, NUM_POWER_POOL_BLOCKS + NUM_INST_POOL_BLOCKS + 5);

    in_band_tlm_cmpnt_init(DIE_ID, 3);
}

TEST_FUNCTION(test_gen_inst_report, test_setup, test_teardown)
{
    inband_inst_samples_per_pkg = MAX_INST_SAMPLES_PER_PACKAGE;

    for (uint16_t i = 0; i < INST_TELEMETRY_ELEMENT_ID_MAX; i++)
    {
        inst_pkg_element_enable[i] = false;
    }
    inst_pkg_element_enable[INST_TELEMETRY_ELEMENT_SOC_RAILS] = true;
    expect_function_calls(data_proc_tlm_cmpnt_get_inst_soc_rail_data, MAX_NUM_OF_VR_RAILS * inband_inst_samples_per_pkg);

    // first time allocates packager
    expect_value(__wrap__txe_queue_receive, queue_ptr, &inst_pkg_free_queue);
    expect_any(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);

    uint32_t* mock_data = (uint32_t*)ib_max_package_mem;
    will_return(__wrap__txe_queue_receive, sizeof(uint32_t));
    will_return(__wrap__txe_queue_receive, &mock_data);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    // last time will dcs_manager_queue_tlm_package
    tlm_package_t tlm_pkg = {0xaabb, 0xaabb};
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

    for (uint16_t sample_count = 0; sample_count < inband_inst_samples_per_pkg; sample_count++)
    {
        in_band_tlm_cmpnt_generate_inst_pkg();
    }

    for (uint16_t i = 0; i < INST_TELEMETRY_ELEMENT_ID_MAX; i++)
    {
        inst_pkg_element_enable[i] = false;
    }

    // first time allocates packager
    expect_value(__wrap__txe_queue_receive, queue_ptr, &inst_pkg_free_queue);
    expect_any(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);

    mock_data = (uint32_t*)ib_max_package_mem;
    will_return(__wrap__txe_queue_receive, sizeof(uint32_t));
    will_return(__wrap__txe_queue_receive, &mock_data);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    // last time will call ddr_manager_deallocate_mem

    for (uint16_t sample_count = 0; sample_count < inband_inst_samples_per_pkg; sample_count++)
    {
        in_band_tlm_cmpnt_generate_inst_pkg();
    }
}

TEST_FUNCTION(test_gen_pwr_report_no_records, test_setup, test_teardown)
{
    for (uint16_t i = 0; i < POWER_TELEMETRY_ELEMENT_ID_MAX; i++)
    {
        power_pkg_element_enable[i] = false;
    }

    expect_value(__wrap__txe_queue_receive, queue_ptr, &pwr_pkg_free_queue);
    expect_any(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);

    uint32_t* mock_data = (uint32_t*)ib_max_package_mem;
    will_return(__wrap__txe_queue_receive, sizeof(uint32_t));
    will_return(__wrap__txe_queue_receive, &mock_data);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    in_band_tlm_cmpnt_generate_pwr_pkg();
}

TEST_FUNCTION(test_gen_pwr_report_some_records, test_setup, test_teardown)
{
    power_pkg_element_enable[POWER_TELEMETRY_ELEMENT_CORE_PSTATE] = true;
    expect_function_calls(data_proc_tlm_cmpnt_get_pwr_core_pstate_data, NUMBER_OF_CORES_PER_DIE);

    // for ddr_manager_allocate_mem_for_pwr_pkg
    expect_value(__wrap__txe_queue_receive, queue_ptr, &pwr_pkg_free_queue);
    expect_any(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);

    uint32_t* mock_data = (uint32_t*)ib_max_package_mem;
    will_return(__wrap__txe_queue_receive, sizeof(uint32_t));
    will_return(__wrap__txe_queue_receive, &mock_data);
    will_return(__wrap__txe_queue_receive, TX_SUCCESS);

    // for dcs_manager_queue_tlm_package
    expect_value(__wrap__txe_queue_receive, queue_ptr, &dcs_pkg_pending_queue);
    expect_any(__wrap__txe_queue_receive, destination_ptr);
    expect_value(__wrap__txe_queue_receive, wait_option, TX_NO_WAIT);

    mock_data = (uint32_t*)ib_max_package_mem;
    will_return(__wrap__txe_queue_receive, sizeof(uint32_t));
    will_return(__wrap__txe_queue_receive, &mock_data);
    will_return(__wrap__txe_queue_receive, TX_QUEUE_EMPTY);

    // for dcs_manager_queue_tlm_package
    expect_any(__wrap__txe_queue_send, queue_ptr);
    expect_any(__wrap__txe_queue_send, source_ptr);
    expect_any(__wrap__txe_queue_send, wait_option);
    will_return(__wrap__txe_queue_send, TX_SUCCESS);

    in_band_tlm_cmpnt_generate_pwr_pkg();
}
