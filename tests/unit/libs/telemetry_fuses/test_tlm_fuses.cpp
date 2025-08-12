//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_tlm_fuses.cpp
 * tlm_fuses tests
 */
// @SSI_Unit_Test
// @SSI_Unit_Test:tlm_fuses

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstddef> // for NULL
#include <cstdint> // for uintptr_t

extern "C" {
#include <FpFwUtils.h>   // for FPFW_UNUSED
#include <fpfw_status.h> // for fpfw_status_t
#include <fuse.h>
#include <fuse_dist_platform_exclusions.h>
#include <fuse_struct.h>
#include <fuses_top_regs.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <kingsgate_fuse_defines.h> // Test revision get
#include <limits.h>
#include <setjmp.h>
#include <silibs_platform.h>
#include <silibs_scp_exp_top_regs.h>
#include <silibs_scp_top_regs.h>
#include <stdnoreturn.h>
#include <tlm_fuses.h>
#include <tlm_fuses_events_i.h>
#include <tlm_fuses_i.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

fpfw_status_t __real_tlm_fuses_read(const uintptr_t fuse_store_addr,
                                    const unsigned int fuse_bit_offset,
                                    const unsigned int fuse_bit_size);

/*-- Declarations (Statics and globals) --*/

extern bool isFuseServiceUp;

static dts_tlm_coeff_t dts_coeff[2];
static uint32_t count = 2;
static unsigned int y_offset;
static uint32_t y_width;

static unsigned int k_offset;
static uint32_t k_width;
static uint32_t coeff_spacing;

static bool mock_tlm_fuses_read = false;

/*------------- Functions ----------------*/

//
// Mocks
//

uint64_t __wrap_fuse_read(const unsigned int fuse_bit_offset, const unsigned int fuse_bit_size)
{
    check_expected(fuse_bit_offset);
    check_expected(fuse_bit_size);
    return mock_type(uint64_t);
}

fpfw_status_t __wrap_tlm_fuses_read(const uintptr_t fuse_store_addr, const unsigned int fuse_bit_offset, const unsigned int fuse_bit_size)
{

    if (mock_tlm_fuses_read)
    {
        return mock_type(fpfw_status_t);
    }
    return __real_tlm_fuses_read(fuse_store_addr, fuse_bit_offset, fuse_bit_size);
}
}

//
// setup functions
//

static int setup_tile_dts_defaults(void** state)
{
    FPFW_UNUSED(state);

    y_offset = TILE_THERMALS_SENSOR_RTS_Y_BIT_OFFSET;
    y_width = TILE_THERMALS_SENSOR_RTS_Y_WIDTH;

    k_offset = TILE_THERMALS_SENSOR_RTS_K_BIT_OFFSET;
    k_width = TILE_THERMALS_SENSOR_RTS_K_WIDTH;
    coeff_spacing = TILE_PVT_NUM_CHANNELS_DTS;

    for (uint32_t i = 0; i < count; i++)
    {
        dts_coeff[i].y_val = DEFAULT_DTS_FUSED_Y_VAL;
        dts_coeff[i].k_val = DEFAULT_DTS_FUSED_K_VAL;
    }
    return 0;
}

static int setup_soctop_defaults(void** state)
{
    FPFW_UNUSED(state);
    isFuseServiceUp = true;
    y_offset = TOP_THERMALS_SENSOR_RTS_Y_BIT_OFFSET;
    y_width = TOP_THERMALS_SENSOR_RTS_Y_WIDTH;

    k_offset = TOP_THERMALS_SENSOR_RTS_K_BIT_OFFSET;
    k_width = TOP_THERMALS_SENSOR_RTS_K_WIDTH;
    coeff_spacing = 1;
    for (uint32_t i = 0; i < count; i++)
    {
        dts_coeff[i].y_val = DEFAULT_DTS_FUSED_Y_VAL;
        dts_coeff[i].k_val = DEFAULT_DTS_FUSED_K_VAL;
    }
    return 0;
}

//
// Tests
//

//
// Public Header Tests
//

TEST_FUNCTION(test_tlm_fuses_init, NULL, NULL)
{
    isFuseServiceUp = false;
    tlm_fuses_init();
    assert_int_equal(isFuseServiceUp, true);
}

TEST_FUNCTION(test_tlm_fuses_macros, NULL, NULL)
{
    // Temperature = ((( 𝒅𝒐𝒖𝒕 /16384 ) ∗ 𝒀+𝑲) * 10) + .5 [dC]]
    uint16_t actual_dC = TLM_FUSE_DOUT_TO_TEMP_DC(10000, 4, 200);
    uint16_t expected_dC = (uint16_t)(((10000.0F / 16384.0F * 200.0F) - 4.0F) * 10.0F + 0.5F);

    assert_int_equal(actual_dC, expected_dC);

    uint16_t reversed = (uint16_t)(TLM_FUSE_TEMP_DC_TO_DOUT(actual_dC, 4, 200));
    assert_true(abs(reversed - 10000) <= 8);
}

TEST_FUNCTION(test_tlm_fuses_get_tile_dts, setup_tile_dts_defaults, NULL)
{
    for (uint32_t i = 0; i < 2; i++)
    {
        expect_value(__wrap_fuse_read, fuse_bit_offset, y_offset);
        expect_value(__wrap_fuse_read, fuse_bit_size, y_width);
        will_return(__wrap_fuse_read, 6);

        expect_value(__wrap_fuse_read, fuse_bit_offset, k_offset);
        expect_value(__wrap_fuse_read, fuse_bit_size, k_width);
        will_return(__wrap_fuse_read, 5);

        k_offset += (k_width * coeff_spacing);
        y_offset += (y_width * coeff_spacing);
    }

    tlm_fuses_get_dts_coeff_tile(dts_coeff, count);
    for (uint32_t i = 0; i < count; i++)
    {
        assert_int_equal(dts_coeff[i].y_val, 6);
        assert_int_equal(dts_coeff[i].k_val, 5);
    }
}

TEST_FUNCTION(test_tlm_fuses_tile_dts_fuses_svc_not_initialized, NULL, NULL)
{
    isFuseServiceUp = false;

    tlm_fuses_get_dts_coeff_tile(dts_coeff, count);

    for (uint32_t i = 0; i < count; i++)
    {
        assert_int_equal(dts_coeff[i].y_val, DEFAULT_DTS_FUSED_Y_VAL);
        assert_int_equal(dts_coeff[i].k_val, DEFAULT_DTS_FUSED_K_VAL);
    }
}

TEST_FUNCTION(test_tlm_fuses_get_soctop_dts, setup_soctop_defaults, NULL)
{
    for (uint32_t i = 0; i < 2; i++)
    {
        expect_value(__wrap_fuse_read, fuse_bit_offset, y_offset);
        expect_value(__wrap_fuse_read, fuse_bit_size, y_width);
        will_return(__wrap_fuse_read, 6);

        expect_value(__wrap_fuse_read, fuse_bit_offset, k_offset);
        expect_value(__wrap_fuse_read, fuse_bit_size, k_width);
        will_return(__wrap_fuse_read, 5);

        k_offset += (k_width * coeff_spacing);
        y_offset += (y_width * coeff_spacing);
    }
    tlm_fuses_get_dts_coeff_soctop(dts_coeff, count);
    for (uint32_t i = 0; i < count; i++)
    {
        assert_int_equal(dts_coeff[i].y_val, 6);
        assert_int_equal(dts_coeff[i].k_val, 5);
    }
}

TEST_FUNCTION(test_tlm_fuses_soctop_dts_fuses_svc_not_initialized, NULL, NULL)
{
    isFuseServiceUp = false;

    tlm_fuses_get_dts_coeff_soctop(dts_coeff, count);

    for (uint32_t i = 0; i < count; i++)
    {
        assert_int_equal(dts_coeff[i].y_val, DEFAULT_DTS_FUSED_Y_VAL);
        assert_int_equal(dts_coeff[i].k_val, DEFAULT_DTS_FUSED_K_VAL);
    }
}

TEST_FUNCTION(test_tlm_fuses_get_dts_coeff_fuse_is_zero, NULL, NULL)
{
    // Setup expectations
    isFuseServiceUp = true;

    y_offset = TOP_THERMALS_SENSOR_RTS_Y_BIT_OFFSET;
    y_width = TOP_THERMALS_SENSOR_RTS_Y_WIDTH;

    k_offset = TOP_THERMALS_SENSOR_RTS_K_BIT_OFFSET;
    k_width = TOP_THERMALS_SENSOR_RTS_K_WIDTH;
    coeff_spacing = 1;

    for (uint32_t i = 0; i < 2; i++)
    {
        expect_value(__wrap_fuse_read, fuse_bit_offset, y_offset);
        expect_value(__wrap_fuse_read, fuse_bit_size, y_width);
        will_return(__wrap_fuse_read, 0);

        expect_value(__wrap_fuse_read, fuse_bit_offset, k_offset);
        expect_value(__wrap_fuse_read, fuse_bit_size, k_width);
        will_return(__wrap_fuse_read, 0);

        k_offset += (k_width * coeff_spacing);
        y_offset += (y_width * coeff_spacing);
    }
    tlm_fuses_get_dts_coeff_soctop(dts_coeff, count);
    for (uint32_t i = 0; i < count; i++)
    {
        assert_int_equal(dts_coeff[i].y_val, DEFAULT_DTS_FUSED_Y_VAL);
        assert_int_equal(dts_coeff[i].k_val, DEFAULT_DTS_FUSED_K_VAL);
    }
}

TEST_FUNCTION(test_platform_power_fuses_get_dts_coeff_fuse_is_zero_tile, NULL, NULL)
{
    // Setup expectations
    isFuseServiceUp = true;

    y_offset = TILE_THERMALS_SENSOR_RTS_Y_BIT_OFFSET;
    y_width = TILE_THERMALS_SENSOR_RTS_Y_WIDTH;

    k_offset = TILE_THERMALS_SENSOR_RTS_K_BIT_OFFSET;
    k_width = TILE_THERMALS_SENSOR_RTS_K_WIDTH;
    coeff_spacing = TILE_PVT_NUM_CHANNELS_DTS;

    for (uint32_t i = 0; i < 2; i++)
    {
        expect_value(__wrap_fuse_read, fuse_bit_offset, y_offset);
        expect_value(__wrap_fuse_read, fuse_bit_size, y_width);
        will_return(__wrap_fuse_read, 0);

        expect_value(__wrap_fuse_read, fuse_bit_offset, k_offset);
        expect_value(__wrap_fuse_read, fuse_bit_size, k_width);
        will_return(__wrap_fuse_read, 0);

        k_offset += (k_width * coeff_spacing);
        y_offset += (y_width * coeff_spacing);
    }
    tlm_fuses_get_dts_coeff_tile(dts_coeff, count);
    for (uint32_t i = 0; i < count; i++)
    {
        assert_int_equal(dts_coeff[i].y_val, DEFAULT_DTS_FUSED_Y_VAL);
        assert_int_equal(dts_coeff[i].k_val, DEFAULT_DTS_FUSED_K_VAL);
    }
}

TEST_FUNCTION(test_tlm_fuses_get_dts_coeff_failed_y_read, NULL, NULL)
{

    uint32_t test_coeff_count = 1;

    // Setup expectations
    isFuseServiceUp = true;
    mock_tlm_fuses_read = true;
    will_return(__wrap_tlm_fuses_read, FPFW_STATUS_FAIL);

    fpfw_status_t status = tlm_fuses_get_dts_coeff(TOP_THERMALS_SENSOR_RTS_K_BIT_OFFSET,
                                                   TOP_THERMALS_SENSOR_RTS_K_WIDTH,
                                                   TOP_THERMALS_SENSOR_RTS_Y_BIT_OFFSET,
                                                   TOP_THERMALS_SENSOR_RTS_Y_WIDTH,
                                                   test_coeff_count,
                                                   TILE_PVT_NUM_CHANNELS_DTS,
                                                   dts_coeff);

    assert_int_equal(status, FPFW_STATUS_FAIL);

    mock_tlm_fuses_read = false;
}

TEST_FUNCTION(test_tlm_fuses_get_dts_coeff_failed_k_read, NULL, NULL)
{

    uint32_t test_coeff_count = 1;

    // Setup expectations
    isFuseServiceUp = true;
    mock_tlm_fuses_read = true;
    will_return(__wrap_tlm_fuses_read, FPFW_STATUS_SUCCESS);
    will_return(__wrap_tlm_fuses_read, FPFW_STATUS_FAIL);

    fpfw_status_t status = tlm_fuses_get_dts_coeff(TOP_THERMALS_SENSOR_RTS_K_BIT_OFFSET,
                                                   TOP_THERMALS_SENSOR_RTS_K_WIDTH,
                                                   TOP_THERMALS_SENSOR_RTS_Y_BIT_OFFSET,
                                                   TOP_THERMALS_SENSOR_RTS_Y_WIDTH,
                                                   test_coeff_count,
                                                   TILE_PVT_NUM_CHANNELS_DTS,
                                                   dts_coeff);

    assert_int_equal(status, FPFW_STATUS_FAIL);

    mock_tlm_fuses_read = false;
}
