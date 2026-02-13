//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_pwr_tlm_exch.cpp
 * Test inter core exchange
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:telemetry

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <pwr_tlm_core_exchange.h>
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

TEST_FUNCTION(test_droop_counts, test_setup, test_teardown)
{
    uint64_t droop_count_array[NUM_AP_CORES_PER_DIE];
    memset(droop_count_array, 0xAB, sizeof(droop_count_array));

    uint64_t verify_array[NUM_AP_CORES_PER_DIE];
    memset(verify_array, 0x00, sizeof(verify_array));

    pwr_tlm_core_exch_init();

    uint8_t seq_num = pwr_tlm_core_exch_mcp_read_droop_counts(&verify_array);
    assert_int_equal(seq_num, 0);

    pwr_tlm_core_exch_scp_write_droop_counts(&droop_count_array);

    seq_num = pwr_tlm_core_exch_mcp_read_droop_counts(&verify_array);
    assert_int_equal(seq_num, 1);

    // stub just clears for now
    for (size_t i = 0; i < NUM_AP_CORES_PER_DIE; ++i)
    {
        assert_true(verify_array[i] == 0xABABABABABABABAB);
    }
}

TEST_FUNCTION(test_mpam_pmu_counts, test_setup, test_teardown)
{
    uint64_t mpam_pmu_count_array[NUMBER_OF_MEM_CONTROLLERS_PER_DIE][NUMBER_OF_MPAMS_PER_MEM_AND_UNTRACK_CTRLR];
    memset(mpam_pmu_count_array, 0xCD, sizeof(mpam_pmu_count_array));

    uint64_t verify_array[NUMBER_OF_MEM_CONTROLLERS_PER_DIE][NUMBER_OF_MPAMS_PER_MEM_AND_UNTRACK_CTRLR];
    memset(verify_array, 0x00, sizeof(verify_array));

    pwr_tlm_core_exch_init();

    uint8_t seq_num = pwr_tlm_core_exch_mcp_read_mpam_pmu_counts((uint64_t*)verify_array);
    assert_int_equal(seq_num, 0);

    pwr_tlm_core_exch_scp_write_mpam_pmu_counts((uint64_t*)mpam_pmu_count_array);

    seq_num = pwr_tlm_core_exch_mcp_read_mpam_pmu_counts((uint64_t*)verify_array);
    assert_int_equal(seq_num, 0);

    // Verify data was written and read correctly
    for (size_t mc = 0; mc < NUMBER_OF_MEM_CONTROLLERS_PER_DIE; ++mc)
    {
        for (size_t mpam = 0; mpam < NUMBER_OF_MPAMS_PER_MEM_AND_UNTRACK_CTRLR; ++mpam)
        {
            assert_true(verify_array[mc][mpam] == 0xCDCDCDCDCDCDCDCD);
        }
    }
}

TEST_FUNCTION(test_mpam_pmu_counts_write_read_multiple, test_setup, test_teardown)
{
    uint64_t write_array[NUMBER_OF_MEM_CONTROLLERS_PER_DIE][NUMBER_OF_MPAMS_PER_MEM_AND_UNTRACK_CTRLR];
    uint64_t read_array[NUMBER_OF_MEM_CONTROLLERS_PER_DIE][NUMBER_OF_MPAMS_PER_MEM_AND_UNTRACK_CTRLR];

    pwr_tlm_core_exch_init();

    // First write with pattern 0x1111111111111111
    memset(write_array, 0x11, sizeof(write_array));
    pwr_tlm_core_exch_scp_write_mpam_pmu_counts((uint64_t*)write_array);

    memset(read_array, 0x00, sizeof(read_array));
    pwr_tlm_core_exch_mcp_read_mpam_pmu_counts((uint64_t*)read_array);

    for (size_t mc = 0; mc < NUMBER_OF_MEM_CONTROLLERS_PER_DIE; ++mc)
    {
        for (size_t mpam = 0; mpam < NUMBER_OF_MPAMS_PER_MEM_AND_UNTRACK_CTRLR; ++mpam)
        {
            assert_true(read_array[mc][mpam] == 0x1111111111111111);
        }
    }

    // Second write with pattern 0x2222222222222222
    memset(write_array, 0x22, sizeof(write_array));
    pwr_tlm_core_exch_scp_write_mpam_pmu_counts((uint64_t*)write_array);

    memset(read_array, 0x00, sizeof(read_array));
    pwr_tlm_core_exch_mcp_read_mpam_pmu_counts((uint64_t*)read_array);

    for (size_t mc = 0; mc < NUMBER_OF_MEM_CONTROLLERS_PER_DIE; ++mc)
    {
        for (size_t mpam = 0; mpam < NUMBER_OF_MPAMS_PER_MEM_AND_UNTRACK_CTRLR; ++mpam)
        {
            assert_true(read_array[mc][mpam] == 0x2222222222222222);
        }
    }
}
