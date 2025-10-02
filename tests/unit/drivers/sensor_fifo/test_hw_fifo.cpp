//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_hw_fifo.cpp
 *
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:sensor_fifo

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include "sensor_fifo_driver_interface.h" // for STRIDE_INDEX_UNUSED
#include "sensor_fifo_mock.h"

#include <FpFwUtils.h>      // for FPFW_UNUSED
#include <device_fifo_id.h> // for DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD
#include <fpfw_status.h>    // for FPFW_STATUS_SUCCESS, fpfw_status_t
#include <hw_fifo.h>
#include <sensor_ram_bridge.h>
#include <stddef.h> // for NULL
#include <stdint.h> // for uint32_t
#include <string.h> // for memse
#include <telemetry_config_struct.h>
}

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
extern "C" {
}

static int fw_fifo_setup(void** pContext)
{
    FPFW_UNUSED(pContext);
    snsr_fifo_mock_use_real_mmio = true;

    initialize_mock_fifos();

    return 0;
}

static int fw_fifo_teardown(void** pContext)
{
    FPFW_UNUSED(pContext);
    snsr_fifo_mock_use_real_mmio = false;

    return 0;
}

//
// Tests
//
TEST_FUNCTION(test_hw_fifo_get_hw_enable, fw_fifo_setup, fw_fifo_teardown)
{
    for (uint32_t fifo_id = DEVICE_FIFO_PSTATE_TLM_HW_PROD; fifo_id <= LAST_HW_PROD_FIFO_ID; fifo_id++)
    {
        assert_int_equal(test_hw_fifo_control[fifo_id].enabled, 0);
    }

    pstate_fifo_ctrl_reg = PSTATE_FIFO_CTRL_EN_MASK;
    msg_fifo_ctrl_reg = SCP_MSG_FIFO_CTRL_EN_MASK;
    tile_temp_fifo_ctrl_reg = TILE_TEMP_FIFO_CTRL_EN_MASK;
    tile_volt_fifo_ctrl_reg = TILE_VOLT_FIFO_CTRL_EN_MASK;
    core_current_fifo_ctrl_reg = CORE_CURRENT_FIFO_CTRL_EN_MASK;
    hw_fifo_get_enabled_from_hw();

    for (uint32_t fifo_id = DEVICE_FIFO_PSTATE_TLM_HW_PROD; fifo_id <= LAST_HW_PROD_FIFO_ID; fifo_id++)
    {
        assert_int_not_equal(test_hw_fifo_control[fifo_id].enabled, 0);
    }

    pstate_fifo_ctrl_reg = 0;
    msg_fifo_ctrl_reg = 0;
    tile_temp_fifo_ctrl_reg = 0;
    tile_volt_fifo_ctrl_reg = 0;
    core_current_fifo_ctrl_reg = 0;
}

TEST_FUNCTION(test_hw_fifo_enable_disable, fw_fifo_setup, fw_fifo_teardown)
{
    // hardware produced fifo
    assert_false(hw_fifo_is_enabled(DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD));
    assert_int_equal(tile_temp_fifo_ctrl_reg, 0);

    hw_fifo_enable(DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD);
    assert_true(hw_fifo_is_enabled(DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD));
    assert_int_equal(tile_temp_fifo_ctrl_reg, TILE_TEMP_FIFO_CTRL_EN_MASK);

    hw_fifo_disable(DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD);
    assert_false(hw_fifo_is_enabled(DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD));
    assert_int_equal(tile_temp_fifo_ctrl_reg, 0);

    // firmware produced fifo
    assert_false(hw_fifo_is_enabled(DEVICE_FIFO_VR_TEMP_TLM_FW_PROD));

    hw_fifo_enable(DEVICE_FIFO_VR_TEMP_TLM_FW_PROD);
    hw_fifo_enable(DEVICE_FIFO_VR_TEMP_TLM_FW_PROD); // 2nd enable is harmless
    assert_true(hw_fifo_is_enabled(DEVICE_FIFO_VR_TEMP_TLM_FW_PROD));

    hw_fifo_disable(DEVICE_FIFO_VR_TEMP_TLM_FW_PROD);
    assert_false(hw_fifo_is_enabled(DEVICE_FIFO_VR_TEMP_TLM_FW_PROD));
}
TEST_FUNCTION(test_hw_fifo_get_stride_index, fw_fifo_setup, fw_fifo_teardown)
{
    // entry size is the same as stride, so not used
    uint16_t stride_index =
        hw_fifo_get_stride_index(DEVICE_FIFO_PSTATE_TLM_HW_PROD,
                                 (uint32_t)(fifo_mem.pstate_fifo + (2 * PSTATE_FIFO_ENTRY_SIZE_BYTES)));
    assert_int_equal(stride_index, STRIDE_INDEX_UNUSED);

    // TILE_TEMP_FIFO_STRIDE_SIZE_BYTES
    stride_index = hw_fifo_get_stride_index(DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD, (uint32_t)(fifo_mem.tile_temp_fifo));
    assert_int_equal(stride_index, 0);

    stride_index = hw_fifo_get_stride_index(
        DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD,
        (uint32_t)(fifo_mem.tile_temp_fifo + TILE_TEMP_FIFO_STRIDE_SIZE_BYTES - TILE_TEMP_FIFO_ENTRY_SIZE_BYTES));
    assert_int_equal(stride_index, (TILE_TEMP_FIFO_STRIDE_SIZE_BYTES / TILE_TEMP_FIFO_ENTRY_SIZE_BYTES) - 1);

    stride_index = hw_fifo_get_stride_index(
        DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD,
        (uint32_t)(fifo_mem.tile_temp_fifo + (2 * TILE_TEMP_FIFO_STRIDE_SIZE_BYTES) - TILE_TEMP_FIFO_ENTRY_SIZE_BYTES));
    assert_int_equal(stride_index, (TILE_TEMP_FIFO_STRIDE_SIZE_BYTES / TILE_TEMP_FIFO_ENTRY_SIZE_BYTES) - 1);

    stride_index =
        hw_fifo_get_stride_index(DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD,
                                 (uint32_t)(fifo_mem.tile_temp_fifo + (2 * TILE_TEMP_FIFO_STRIDE_SIZE_BYTES)));
    assert_int_equal(stride_index, 0);
}

TEST_FUNCTION(test_hw_fifo_update_write_ptr_by_stride, fw_fifo_setup, fw_fifo_teardown)
{
    uint32_t expected_write_ptr = (uint32_t)fifo_mem.core_current_fifo + FIFO_TIMESTAMP_SIZE;
    uint32_t total_fifo_size = CORE_CURRENT_FIFO_NUM_ENTRIES * CORE_CURRENT_FIFO_STRIDE_SIZE_BYTES;

    assert_int_equal(core_current_fifo_write_reg, expected_write_ptr);

    // add CORE_CURRENT_FIFO_NUM_ENTRIES + 1
    for (uint32_t entry = 0; entry <= CORE_CURRENT_FIFO_NUM_ENTRIES; entry++)
    {
        hw_fifo_update_write_ptr_by_stride_size(DEVICE_FIFO_CORE_CURRENT_TLM_HW_PROD);

        expected_write_ptr += CORE_CURRENT_FIFO_STRIDE_SIZE_BYTES;

        if (expected_write_ptr >= ((uint32_t)fifo_mem.core_current_fifo + total_fifo_size))
        {
            expected_write_ptr = (uint32_t)fifo_mem.core_current_fifo + FIFO_TIMESTAMP_SIZE;
        }

        assert_int_equal(core_current_fifo_write_reg, expected_write_ptr);
    }

    // test fifo wrap around to 1 entry past start
    assert_int_equal(core_current_fifo_write_reg,
                     (uint32_t)fifo_mem.core_current_fifo + FIFO_TIMESTAMP_SIZE + CORE_CURRENT_FIFO_STRIDE_SIZE_BYTES);
}

TEST_FUNCTION(test_hw_fifo_update_read_ptr_by_entry, fw_fifo_setup, fw_fifo_teardown)
{
    uint32_t expected_read_ptr = (uint32_t)fifo_mem.dimm_temp_fifo + FIFO_TIMESTAMP_SIZE;
    uint32_t total_fifo_size = DIMM_FIFO_NUM_ENTRIES * DIMM_FIFO_STRIDE_SIZE_BYTES;

    assert_int_equal(dimm_temp_fifo_read_reg, expected_read_ptr);

    // read DIMM_FIFO_NUM_ENTRIES + 1
    for (uint32_t entry = 1; entry <= DIMM_FIFO_NUM_ENTRIES + 1; entry++)
    {
        hw_fifo_update_read_ptr_by_entry_size(DEVICE_FIFO_DIMM_TEMP_TLM_FW_PROD, 1);

        expected_read_ptr += DIMM_FIFO_ENTRY_SIZE_BYTES;

        if (expected_read_ptr >= ((uint32_t)fifo_mem.dimm_temp_fifo + total_fifo_size))
        {
            expected_read_ptr = (uint32_t)fifo_mem.dimm_temp_fifo + FIFO_TIMESTAMP_SIZE;
        }

        assert_int_equal(dimm_temp_fifo_read_reg, expected_read_ptr);
    }
    // test fifo wrap around to 1 entry past start
    assert_int_equal(dimm_temp_fifo_read_reg,
                     (uint32_t)fifo_mem.dimm_temp_fifo + FIFO_TIMESTAMP_SIZE + DIMM_FIFO_ENTRY_SIZE_BYTES);

    // test multiple wrap around
    initialize_mock_fifos();

    hw_fifo_update_read_ptr_by_entry_size(DEVICE_FIFO_DIMM_TEMP_TLM_FW_PROD, DIMM_FIFO_NUM_ENTRIES + 2);
    expected_read_ptr = (uint32_t)fifo_mem.dimm_temp_fifo + FIFO_TIMESTAMP_SIZE + (DIMM_FIFO_ENTRY_SIZE_BYTES * 2);
    assert_int_equal(dimm_temp_fifo_read_reg, expected_read_ptr);
}

TEST_FUNCTION(test_hw_fifo_update_write_ptr_by_size, fw_fifo_setup, fw_fifo_teardown)
{
    // write_ptr > read_ptr after update, no overflow
    pstate_fifo_read_reg = (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE + PSTATE_FIFO_ENTRY_SIZE_BYTES);
    pstate_fifo_write_reg = (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE +
                                       ((PSTATE_FIFO_NUM_ENTRIES - 1) * PSTATE_FIFO_ENTRY_SIZE_BYTES));

    hw_fifo_update_write_ptr_by_size(DEVICE_FIFO_PSTATE_TLM_HW_PROD, PSTATE_FIFO_ENTRY_SIZE_BYTES, 1);

    assert_int_equal(pstate_fifo_read_reg,
                     (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE + PSTATE_FIFO_ENTRY_SIZE_BYTES)); // unchanged
    assert_int_equal(pstate_fifo_write_reg, (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE)); // wrap around, but not pass read ptr

    // write_ptr < read_ptr after update, write pointer moves to read pointer, read pointer advances
    test_hw_fifo_control[DEVICE_FIFO_PSTATE_TLM_HW_PROD].overflow_count = 2;
    pstate_fifo_read_reg = (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE);
    pstate_fifo_write_reg = (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE +
                                       ((PSTATE_FIFO_NUM_ENTRIES - 1) * PSTATE_FIFO_ENTRY_SIZE_BYTES));

    hw_fifo_update_write_ptr_by_size(DEVICE_FIFO_PSTATE_TLM_HW_PROD, PSTATE_FIFO_ENTRY_SIZE_BYTES, 1);

    assert_int_equal(pstate_fifo_read_reg,
                     (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE + PSTATE_FIFO_ENTRY_SIZE_BYTES)); // moved forward
    assert_int_equal(pstate_fifo_write_reg, (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE)); // wrap around, less than read ptr
    assert_int_equal(test_hw_fifo_control[DEVICE_FIFO_PSTATE_TLM_HW_PROD].overflow_count, 3);

    // write_ptr > read_ptr , write pointer moves to read pointer, read pointer advances
    test_hw_fifo_control[DEVICE_FIFO_PSTATE_TLM_HW_PROD].overflow_count = 8;
    pstate_fifo_read_reg = (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE + PSTATE_FIFO_ENTRY_SIZE_BYTES);
    pstate_fifo_write_reg = (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE +
                                       ((PSTATE_FIFO_NUM_ENTRIES - 1) * PSTATE_FIFO_ENTRY_SIZE_BYTES));

    hw_fifo_update_write_ptr_by_size(DEVICE_FIFO_PSTATE_TLM_HW_PROD, PSTATE_FIFO_ENTRY_SIZE_BYTES, PSTATE_FIFO_NUM_ENTRIES + 1);

    assert_int_equal(pstate_fifo_read_reg,
                     (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE + PSTATE_FIFO_ENTRY_SIZE_BYTES)); // moved forward
    assert_int_equal(pstate_fifo_write_reg, (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE)); // wrap around, less than read ptr
    assert_int_equal(test_hw_fifo_control[DEVICE_FIFO_PSTATE_TLM_HW_PROD].overflow_count, 9);

    // write_ptr < read_ptr , write pointer moves to read pointer, read pointer advances and wraps around
    test_hw_fifo_control[DEVICE_FIFO_PSTATE_TLM_HW_PROD].overflow_count = 4;
    pstate_fifo_read_reg = (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE +
                                      ((PSTATE_FIFO_NUM_ENTRIES - 1) * PSTATE_FIFO_ENTRY_SIZE_BYTES));
    pstate_fifo_write_reg = (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE);

    hw_fifo_update_write_ptr_by_size(DEVICE_FIFO_PSTATE_TLM_HW_PROD, PSTATE_FIFO_ENTRY_SIZE_BYTES, PSTATE_FIFO_NUM_ENTRIES - 1);

    assert_int_equal(pstate_fifo_read_reg, (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE)); // moved forward and wrapped around
    assert_int_equal(pstate_fifo_write_reg,
                     (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE +
                                ((PSTATE_FIFO_NUM_ENTRIES - 1) * PSTATE_FIFO_ENTRY_SIZE_BYTES))); // wrap around, less than read ptr
    assert_int_equal(test_hw_fifo_control[DEVICE_FIFO_PSTATE_TLM_HW_PROD].overflow_count, 5);
}

TEST_FUNCTION(test_hw_fifo_empty, fw_fifo_setup, fw_fifo_teardown)
{
    hw_fifo_disable(DEVICE_FIFO_PSTATE_TLM_HW_PROD);

    pstate_fifo_read_reg = (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE + PSTATE_FIFO_ENTRY_SIZE_BYTES);
    pstate_fifo_write_reg = (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE +
                                       ((PSTATE_FIFO_NUM_ENTRIES - 1) * PSTATE_FIFO_ENTRY_SIZE_BYTES));

    // empty because it's disabled
    assert_true(hw_fifo_is_empty(DEVICE_FIFO_PSTATE_TLM_HW_PROD));

    // empty because cleared on enable
    hw_fifo_enable(DEVICE_FIFO_PSTATE_TLM_HW_PROD);
    assert_true(hw_fifo_is_empty(DEVICE_FIFO_PSTATE_TLM_HW_PROD));

    pstate_fifo_read_reg = (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE + PSTATE_FIFO_ENTRY_SIZE_BYTES);
    pstate_fifo_write_reg = (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE +
                                       ((PSTATE_FIFO_NUM_ENTRIES - 1) * PSTATE_FIFO_ENTRY_SIZE_BYTES));
    assert_false(hw_fifo_is_empty(DEVICE_FIFO_PSTATE_TLM_HW_PROD));
}

TEST_FUNCTION(test_hw_fifo_get_latched_entries, fw_fifo_setup, fw_fifo_teardown)
{
    test_hw_fifo_control[DEVICE_FIFO_PSTATE_TLM_HW_PROD].latched_write_address = UINT32_MAX;
    size_t latched_entries = hw_fifo_get_remaining_latched_entries(DEVICE_FIFO_PSTATE_TLM_HW_PROD);
    assert_int_equal(latched_entries, 0);

    pstate_fifo_read_reg = (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE + PSTATE_FIFO_ENTRY_SIZE_BYTES);
    test_hw_fifo_control[DEVICE_FIFO_PSTATE_TLM_HW_PROD].latched_write_address =
        (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE + ((PSTATE_FIFO_NUM_ENTRIES - 1) * PSTATE_FIFO_ENTRY_SIZE_BYTES));
    latched_entries = hw_fifo_get_remaining_latched_entries(DEVICE_FIFO_PSTATE_TLM_HW_PROD);
    assert_int_equal(latched_entries, PSTATE_FIFO_NUM_ENTRIES - 1 - 1); // read ptr is at +1, write ptr is at PSTATE_FIFO_NUM_ENTRIES - 1

    pstate_fifo_read_reg = (uint32_t)(fifo_mem.pstate_fifo + FIFO_TIMESTAMP_SIZE +
                                      ((PSTATE_FIFO_NUM_ENTRIES - 1) * PSTATE_FIFO_ENTRY_SIZE_BYTES));
    test_hw_fifo_control[DEVICE_FIFO_PSTATE_TLM_HW_PROD].latched_write_address =
        (uint32_t)(fifo_mem.pstate_fifo) + FIFO_TIMESTAMP_SIZE;
    latched_entries = hw_fifo_get_remaining_latched_entries(DEVICE_FIFO_PSTATE_TLM_HW_PROD);
    assert_int_equal(latched_entries, 1);
}

TEST_FUNCTION(test_hw_fifo_write_helper, fw_fifo_setup, fw_fifo_teardown)
{
    uint8_t src_data[PVT_TEMP_FIFO_NUM_ENTRIES * PVT_TEMP_FIFO_ENTRY_SIZE_BYTES];

    uint8_t* ptr_src = src_data;

    for (uint16_t i = 1; i <= PVT_TEMP_FIFO_NUM_ENTRIES; i++)
    {
        memset(ptr_src, i, PVT_TEMP_FIFO_ENTRY_SIZE_BYTES);
        ptr_src += PVT_TEMP_FIFO_ENTRY_SIZE_BYTES;
    }

    memset(fifo_mem.pvt_temp_fifo, 0x00, sizeof(fifo_mem.pvt_temp_fifo));

    uint8_t placeholder[1000];
    scf_base_config scf_config;
    scf_config.scf_mhu_base_address = (uintptr_t)placeholder;
    scf_config.scf_ram_base_address = (uintptr_t)fifo_mem.pvt_temp_fifo;
    scf_config.scf_ram_buffer_size = sizeof(fifo_mem.pvt_temp_fifo);
    scf_set_working_config((uintptr_t)&scf_config);

    fpfw_status_t status =
        hw_fifo_write_helper((uintptr_t)fifo_mem.pvt_temp_fifo, src_data, PVT_TEMP_FIFO_ENTRY_SIZE_BYTES, PVT_TEMP_FIFO_NUM_ENTRIES);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    for (uint16_t i = 0; i < PVT_TEMP_FIFO_NUM_ENTRIES; i++)
    {
        for (uint16_t j = 0; j < PVT_TEMP_FIFO_ENTRY_SIZE_BYTES; j++)
        {
            uint16_t index = i * PVT_TEMP_FIFO_ENTRY_SIZE_BYTES + j;
            // Compare each entry
            assert_int_equal(fifo_mem.pvt_temp_fifo[index], i + 1);
        }
    }

    // scf_ram_write_entry will fail because destination is outside of scf_config.scf_ram_base_address
    status = hw_fifo_write_helper((uintptr_t)(fifo_mem.pvt_temp_fifo - 4), src_data, PVT_TEMP_FIFO_ENTRY_SIZE_BYTES, PVT_TEMP_FIFO_NUM_ENTRIES);
    assert_int_equal(status, FPFW_STATUS_FAIL);
}

TEST_FUNCTION(test_hw_fifo_write_single_entry, fw_fifo_setup, fw_fifo_teardown)
{
    uint8_t src_data[PVT_TEMP_FIFO_ENTRY_SIZE_BYTES] = {0};

    memset(src_data, 0x48, PVT_TEMP_FIFO_ENTRY_SIZE_BYTES);
    memset(fifo_mem.pvt_temp_fifo, 0x00, sizeof(fifo_mem.pvt_temp_fifo));

    uint8_t placeholder[1000];
    scf_base_config scf_config;
    scf_config.scf_mhu_base_address = (uintptr_t)placeholder;
    scf_config.scf_ram_base_address = (uintptr_t)fifo_mem.pvt_temp_fifo;
    scf_config.scf_ram_buffer_size = sizeof(fifo_mem.pvt_temp_fifo);
    scf_set_working_config((uintptr_t)&scf_config);

    pvt_temp_fifo_read_reg = (uint32_t)(fifo_mem.pvt_temp_fifo + FIFO_TIMESTAMP_SIZE + PVT_TEMP_FIFO_ENTRY_SIZE_BYTES);
    pvt_temp_fifo_write_reg = (uint32_t)(fifo_mem.pvt_temp_fifo + FIFO_TIMESTAMP_SIZE + PVT_TEMP_FIFO_ENTRY_SIZE_BYTES);

    assert_true(hw_fifo_is_empty(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD));

    hw_fifo_enable(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD);

    fpfw_status_t status =
        hw_fifo_write_entry(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD, src_data, PVT_TEMP_FIFO_ENTRY_SIZE_BYTES, 1, 0);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    assert_false(hw_fifo_is_empty(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD));

    // now read it back
    uint8_t read_data[PVT_TEMP_FIFO_ENTRY_SIZE_BYTES] = {0};
    uint16_t num_entries_read = 0;
    uint16_t num_entries_remaining = 0;
    uint16_t stride_index = 0;

    status = hw_fifo_read_entry(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD,
                                PVT_TEMP_FIFO_ENTRY_SIZE_BYTES,
                                read_data,
                                &num_entries_read,
                                &num_entries_remaining,
                                &stride_index);

    assert_int_equal(status, FPFW_STATUS_SUCCESS);
    assert_int_equal(num_entries_read, 1);
    assert_int_equal(num_entries_remaining, 0);
    assert_int_equal(stride_index, STRIDE_INDEX_UNUSED);
    assert_true(hw_fifo_is_empty(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD));

    for (uint16_t j = 0; j < PVT_TEMP_FIFO_ENTRY_SIZE_BYTES; j++)
    {
        // Compare each entry
        assert_int_equal(read_data[j], 0x48);
    }
}

TEST_FUNCTION(test_hw_fifo_write_multiple_entry_wrap_read_one, fw_fifo_setup, fw_fifo_teardown)
{
    // same as test above, but reads back one at a time instead all at once

    uint8_t src_data[(PVT_TEMP_FIFO_NUM_ENTRIES - 1) * PVT_TEMP_FIFO_ENTRY_SIZE_BYTES] = {0};
    uint8_t* ptr_src = src_data;

    for (uint16_t i = 1; i <= (PVT_TEMP_FIFO_NUM_ENTRIES - 1); i++)
    {
        memset(ptr_src, i, PVT_TEMP_FIFO_ENTRY_SIZE_BYTES);
        ptr_src += PVT_TEMP_FIFO_ENTRY_SIZE_BYTES;
    }

    memset(fifo_mem.pvt_temp_fifo, 0x00, sizeof(fifo_mem.pvt_temp_fifo));

    uint8_t placeholder[1000];
    scf_base_config scf_config;
    scf_config.scf_mhu_base_address = (uintptr_t)placeholder;
    scf_config.scf_ram_base_address = (uintptr_t)fifo_mem.pvt_temp_fifo;
    scf_config.scf_ram_buffer_size = sizeof(fifo_mem.pvt_temp_fifo);
    scf_set_working_config((uintptr_t)&scf_config);

    pvt_temp_fifo_read_reg = (uint32_t)(fifo_mem.pvt_temp_fifo + FIFO_TIMESTAMP_SIZE +
                                        ((PVT_TEMP_FIFO_NUM_ENTRIES - 1) * PVT_TEMP_FIFO_ENTRY_SIZE_BYTES));
    pvt_temp_fifo_write_reg = (uint32_t)(fifo_mem.pvt_temp_fifo + FIFO_TIMESTAMP_SIZE +
                                         ((PVT_TEMP_FIFO_NUM_ENTRIES - 1) * PVT_TEMP_FIFO_ENTRY_SIZE_BYTES));

    assert_true(hw_fifo_is_empty(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD));

    hw_fifo_enable(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD);

    fpfw_status_t status = hw_fifo_write_entry(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD,
                                               src_data,
                                               PVT_TEMP_FIFO_ENTRY_SIZE_BYTES,
                                               (PVT_TEMP_FIFO_NUM_ENTRIES - 1),
                                               0);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);

    assert_false(hw_fifo_is_empty(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD));

    // now read it back
    uint8_t read_data[(PVT_TEMP_FIFO_NUM_ENTRIES - 1) * PVT_TEMP_FIFO_ENTRY_SIZE_BYTES] = {0};
    uint16_t num_entries_read = 0;
    uint16_t num_entries_remaining = 0;
    uint16_t stride_index;

    uint8_t* read_ptr = read_data;

    do
    {
        status = hw_fifo_read_entry(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD,
                                    PVT_TEMP_FIFO_ENTRY_SIZE_BYTES,
                                    read_ptr,
                                    &num_entries_read,
                                    &num_entries_remaining,
                                    &stride_index);
        read_ptr += PVT_TEMP_FIFO_ENTRY_SIZE_BYTES;
        assert_int_equal(stride_index, STRIDE_INDEX_UNUSED);
    } while ((status == FPFW_STATUS_SUCCESS) && (num_entries_remaining > 0));

    assert_int_equal(status, FPFW_STATUS_SUCCESS);
    assert_int_equal(num_entries_remaining, 0);
    assert_true(hw_fifo_is_empty(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD));

    for (uint16_t i = 0; i < (PVT_TEMP_FIFO_NUM_ENTRIES - 1); i++)
    {
        for (uint16_t j = 0; j < PVT_TEMP_FIFO_ENTRY_SIZE_BYTES; j++)
        {
            uint16_t index = i * PVT_TEMP_FIFO_ENTRY_SIZE_BYTES + j;
            // Compare each entry
            assert_int_equal(read_data[index], i + 1);
        }
    }
}

TEST_FUNCTION(test_hw_fifo_write_stride, fw_fifo_setup, fw_fifo_teardown)
{
#define NUM_STRIDE_ENTRIES    (2)
#define ENTRY_INDEX_IN_STRIDE (2)
    uint8_t src_data[(NUM_STRIDE_ENTRIES)*TILE_TEMP_FIFO_ENTRY_SIZE_BYTES] = {0};
    uint8_t* ptr_src = src_data;
    for (uint16_t i = 1; i <= (NUM_STRIDE_ENTRIES); i++)
    {
        memset(ptr_src, i, TILE_TEMP_FIFO_ENTRY_SIZE_BYTES);
        ptr_src += TILE_TEMP_FIFO_ENTRY_SIZE_BYTES;
    }
    memset(fifo_mem.tile_temp_fifo, 0x00, sizeof(fifo_mem.tile_temp_fifo));

    uint8_t placeholder[1000];
    scf_base_config scf_config;
    scf_config.scf_mhu_base_address = (uintptr_t)placeholder;
    scf_config.scf_ram_base_address = (uintptr_t)fifo_mem.tile_temp_fifo;
    scf_config.scf_ram_buffer_size = sizeof(fifo_mem.tile_temp_fifo);
    scf_set_working_config((uintptr_t)&scf_config);

    // set the fifo as empty starting on the 2nd stride
    tile_temp_fifo_read_reg =
        (uint32_t)(fifo_mem.tile_temp_fifo + FIFO_TIMESTAMP_SIZE + (TILE_TEMP_FIFO_STRIDE_SIZE_BYTES));
    tile_temp_fifo_write_reg =
        (uint32_t)(fifo_mem.tile_temp_fifo + FIFO_TIMESTAMP_SIZE + (TILE_TEMP_FIFO_STRIDE_SIZE_BYTES));

    assert_true(hw_fifo_is_empty(DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD));
    hw_fifo_enable(DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD);

    fpfw_status_t status = hw_fifo_write_entry(DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD,
                                               src_data,
                                               TILE_TEMP_FIFO_ENTRY_SIZE_BYTES,
                                               NUM_STRIDE_ENTRIES,
                                               ENTRY_INDEX_IN_STRIDE);
    assert_int_equal(status, FPFW_STATUS_SUCCESS);
    // the write pointer in real hw buffers is moved when the stride data is full
    // for sw sim, the write pointer won't be updated until this test moves is
    assert_true(hw_fifo_is_empty(DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD));

    hw_fifo_update_write_ptr_by_stride_size(DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD);
    assert_false(hw_fifo_is_empty(DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD));

    // now read it back
    uint8_t read_data[TILE_TEMP_FIFO_NUM_ENTRIES * TILE_TEMP_FIFO_STRIDE_SIZE_BYTES] = {0}; // need to read full stride to empty
    uint16_t num_entries_read = 0;
    uint16_t num_entries_remaining = 0;
    uint16_t stride_index = 0;
    uint8_t* read_ptr = read_data;
    uint16_t expected_stride_index = 0;
    do
    {
        status = hw_fifo_read_entry(DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD,
                                    TILE_TEMP_FIFO_ENTRY_SIZE_BYTES,
                                    read_ptr,
                                    &num_entries_read,
                                    &num_entries_remaining,
                                    &stride_index);
        read_ptr += TILE_TEMP_FIFO_ENTRY_SIZE_BYTES;
        assert_int_equal(stride_index, expected_stride_index++);
    } while ((status == FPFW_STATUS_SUCCESS) && (num_entries_remaining > 0));

    assert_int_equal(status, FPFW_STATUS_SUCCESS);
    assert_int_equal(num_entries_remaining, 0);
    assert_int_equal(expected_stride_index, ENTRY_INDEX_IN_STRIDE + NUM_STRIDE_ENTRIES); // one past last index due to post increment in loop
    assert_true(hw_fifo_is_empty(DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD));

    for (uint16_t i = ENTRY_INDEX_IN_STRIDE; i < (ENTRY_INDEX_IN_STRIDE + NUM_STRIDE_ENTRIES); i++)
    {
        for (uint16_t j = 0; j < TILE_TEMP_FIFO_ENTRY_SIZE_BYTES; j++)
        {
            uint16_t index = i * TILE_TEMP_FIFO_ENTRY_SIZE_BYTES + j;
            // Compare each entry
            assert_int_equal(read_data[index], i - ENTRY_INDEX_IN_STRIDE + 1);
        }
    }
}

TEST_FUNCTION(test_hw_fifo_write_fail_cases, fw_fifo_setup, fw_fifo_teardown)
{
#define NUM_STRIDE_ENTRIES    (2)
#define ENTRY_INDEX_IN_STRIDE (2)
    uint8_t src_data[(NUM_STRIDE_ENTRIES)*TILE_TEMP_FIFO_ENTRY_SIZE_BYTES] = {0};
    uint8_t* ptr_src = src_data;
    for (uint16_t i = 1; i <= (NUM_STRIDE_ENTRIES); i++)
    {
        memset(ptr_src, i, TILE_TEMP_FIFO_ENTRY_SIZE_BYTES);
        ptr_src += TILE_TEMP_FIFO_ENTRY_SIZE_BYTES;
    }
    memset(fifo_mem.tile_temp_fifo, 0x00, sizeof(fifo_mem.tile_temp_fifo));

    assert_true(hw_fifo_is_empty(DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD));
    // will return FPFW_STATUS_DISABLED
    hw_fifo_disable(DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD);

    fpfw_status_t status = hw_fifo_write_entry(DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD,
                                               src_data,
                                               TILE_TEMP_FIFO_ENTRY_SIZE_BYTES,
                                               NUM_STRIDE_ENTRIES,
                                               ENTRY_INDEX_IN_STRIDE);
    assert_int_equal(status, FPFW_STATUS_DISABLED);

    hw_fifo_enable(DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD);

    uint8_t placeholder[1000];
    scf_base_config scf_config;
    scf_config.scf_mhu_base_address = (uintptr_t)placeholder;
    scf_config.scf_ram_base_address = (uintptr_t)fifo_mem.msg_fifo; // this will cause a write failure in scf_ram_write_entry
    scf_config.scf_ram_buffer_size = sizeof(fifo_mem.msg_fifo);
    scf_set_working_config((uintptr_t)&scf_config);

    // set the fifo as empty starting on the 2nd stride
    tile_temp_fifo_read_reg =
        (uint32_t)(fifo_mem.tile_temp_fifo + FIFO_TIMESTAMP_SIZE + (TILE_TEMP_FIFO_STRIDE_SIZE_BYTES));
    tile_temp_fifo_write_reg =
        (uint32_t)(fifo_mem.tile_temp_fifo + FIFO_TIMESTAMP_SIZE + (TILE_TEMP_FIFO_STRIDE_SIZE_BYTES));

    test_hw_fifo_control[DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD].write_errors = 5;

    status = hw_fifo_write_entry(DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD, src_data, TILE_TEMP_FIFO_ENTRY_SIZE_BYTES, NUM_STRIDE_ENTRIES, ENTRY_INDEX_IN_STRIDE);
    assert_int_equal(status, FPFW_STATUS_FAIL);
    assert_int_equal(test_hw_fifo_control[DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD].write_errors, 6);
}

TEST_FUNCTION(test_hw_fifo_write_fail_2, fw_fifo_setup, fw_fifo_teardown)
{
    uint8_t src_data[PVT_TEMP_FIFO_ENTRY_SIZE_BYTES] = {0};

    memset(src_data, 0x48, PVT_TEMP_FIFO_ENTRY_SIZE_BYTES);
    memset(fifo_mem.pvt_temp_fifo, 0x00, sizeof(fifo_mem.pvt_temp_fifo));

    uint8_t placeholder[1000];
    scf_base_config scf_config;
    scf_config.scf_mhu_base_address = (uintptr_t)placeholder;
    scf_config.scf_ram_base_address = (uintptr_t)fifo_mem.dimm_temp_fifo; // this will cause scf_ram_write_entry failure
    scf_config.scf_ram_buffer_size = sizeof(fifo_mem.dimm_temp_fifo);
    scf_set_working_config((uintptr_t)&scf_config);

    pvt_temp_fifo_read_reg = (uint32_t)(fifo_mem.pvt_temp_fifo + FIFO_TIMESTAMP_SIZE + PVT_TEMP_FIFO_ENTRY_SIZE_BYTES);
    pvt_temp_fifo_write_reg = (uint32_t)(fifo_mem.pvt_temp_fifo + FIFO_TIMESTAMP_SIZE + PVT_TEMP_FIFO_ENTRY_SIZE_BYTES);

    assert_true(hw_fifo_is_empty(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD));

    hw_fifo_enable(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD);

    test_hw_fifo_control[DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD].write_errors = 7;

    fpfw_status_t status =
        hw_fifo_write_entry(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD, src_data, PVT_TEMP_FIFO_ENTRY_SIZE_BYTES, 1, 0);
    assert_int_equal(status, FPFW_STATUS_FAIL);

    assert_int_equal(test_hw_fifo_control[DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD].write_errors, 8);
}

TEST_FUNCTION(test_hw_fifo_read_fail, fw_fifo_setup, fw_fifo_teardown)
{
    memset(fifo_mem.pvt_temp_fifo, 0x00, sizeof(fifo_mem.pvt_temp_fifo));

    uint8_t placeholder[1000];
    scf_base_config scf_config;
    scf_config.scf_mhu_base_address = (uintptr_t)placeholder;
    scf_config.scf_ram_base_address = (uintptr_t)fifo_mem.pvt_temp_fifo;
    scf_config.scf_ram_buffer_size = sizeof(fifo_mem.pvt_temp_fifo);
    scf_set_working_config((uintptr_t)&scf_config);

    pvt_temp_fifo_read_reg = (uint32_t)(fifo_mem.pvt_temp_fifo + FIFO_TIMESTAMP_SIZE + PVT_TEMP_FIFO_ENTRY_SIZE_BYTES);
    pvt_temp_fifo_write_reg =
        (uint32_t)(fifo_mem.pvt_temp_fifo + FIFO_TIMESTAMP_SIZE + (PVT_TEMP_FIFO_ENTRY_SIZE_BYTES * 2));

    // test disabled
    hw_fifo_disable(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD);

    uint8_t read_data[PVT_TEMP_FIFO_ENTRY_SIZE_BYTES] = {0};
    uint16_t num_entries_read = 0;
    uint16_t num_entries_remaining = 0;
    uint16_t stride_index = 0;

    fpfw_status_t status = hw_fifo_read_entry(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD,
                                              PVT_TEMP_FIFO_ENTRY_SIZE_BYTES,
                                              read_data,
                                              &num_entries_read,
                                              &num_entries_remaining,
                                              &stride_index);

    assert_int_equal(status, FPFW_STATUS_SUCCESS);
    assert_int_equal(num_entries_read, 0);
    assert_int_equal(num_entries_remaining, 0);
    assert_int_equal(stride_index, STRIDE_INDEX_UNUSED);

    // test empty
    hw_fifo_enable(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD);
    status = hw_fifo_read_entry(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD,
                                PVT_TEMP_FIFO_ENTRY_SIZE_BYTES,
                                read_data,
                                &num_entries_read,
                                &num_entries_remaining,
                                &stride_index);

    assert_int_equal(status, FPFW_STATUS_SUCCESS);
    assert_int_equal(num_entries_read, 0);
    assert_int_equal(num_entries_remaining, 0);
    assert_int_equal(stride_index, STRIDE_INDEX_UNUSED);

    scf_config.scf_mhu_base_address = (uintptr_t)placeholder;
    scf_config.scf_ram_base_address = (uintptr_t)fifo_mem.dimm_temp_fifo; // this will cause scf_ram_read_entry failure
    scf_config.scf_ram_buffer_size = sizeof(fifo_mem.dimm_temp_fifo);
    scf_set_working_config((uintptr_t)&scf_config);

    pvt_temp_fifo_read_reg = (uint32_t)(fifo_mem.pvt_temp_fifo + FIFO_TIMESTAMP_SIZE + PVT_TEMP_FIFO_ENTRY_SIZE_BYTES);
    pvt_temp_fifo_write_reg =
        (uint32_t)(fifo_mem.pvt_temp_fifo + FIFO_TIMESTAMP_SIZE + (PVT_TEMP_FIFO_ENTRY_SIZE_BYTES * 2));

    status = hw_fifo_read_entry(DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD,
                                PVT_TEMP_FIFO_ENTRY_SIZE_BYTES,
                                read_data,
                                &num_entries_read,
                                &num_entries_remaining,
                                &stride_index);

    assert_int_equal(status, FPFW_STATUS_FAIL);
    assert_int_equal(num_entries_read, 0);
    assert_int_equal(num_entries_remaining, 1);
    assert_int_equal(stride_index, STRIDE_INDEX_UNUSED);
}
