//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_power_fuse.cpp
 * power_fuse tests
 */
// @SSI_Unit_Test
// @SSI_Unit_Test:power_fuse

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
#include <power_fuse_events_i.h>
#include <power_tlm_fuse.h>
#include <power_tlm_fuse_i.h>
#include <setjmp.h>
#include <silibs_platform.h>
#include <silibs_scp_exp_top_regs.h>
#include <silibs_scp_top_regs.h>
#include <stdnoreturn.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

static dts_tlm_coeff_t dts_coeff[2];
static uint32_t count = 2;
static unsigned int y_offset;
static uint32_t y_width;

static unsigned int k_offset;
static uint32_t k_width;
static uint32_t coeff_spacing;

/*------------- Functions ----------------*/
//
// Mocks
//
KNG_PLAT_ID __wrap_idsw_get_platform_sdv()
{
    return mock_type(KNG_PLAT_ID);
}

uint64_t __wrap_read_fuse(const unsigned int fuse_bit_offset, const unsigned int fuse_bit_size)
{
    check_expected(fuse_bit_offset);
    check_expected(fuse_bit_size);
    return mock_type(uint64_t);
}
}

//
// setup functions
//

static int set_power_fuse_parameters_tile(void** state)
{
    FPFW_UNUSED(state);

    y_offset = TILE_THERMALS_SENSOR_RTS_Y_BIT_OFFSET;
    y_width = TILE_THERMALS_SENSOR_RTS_Y_WIDTH;

    k_offset = TILE_THERMALS_SENSOR_RTS_K_BIT_OFFSET;
    k_width = TILE_THERMALS_SENSOR_RTS_K_WIDTH;
    coeff_spacing = TILE_PVT_NUM_CHANNELS_DTS;

    for (uint32_t i = 0; i < count; i++)
    {
        dts_coeff[i].y_val = 16384;
        dts_coeff[i].k_val = 0;
    }
    return 0;
}

static int set_power_fuse_parameters_soctop(void** state)
{
    FPFW_UNUSED(state);
    y_offset = TOP_THERMALS_SENSOR_RTS_Y_BIT_OFFSET;
    y_width = TOP_THERMALS_SENSOR_RTS_Y_WIDTH;

    k_offset = TOP_THERMALS_SENSOR_RTS_K_BIT_OFFSET;
    k_width = TOP_THERMALS_SENSOR_RTS_K_WIDTH;
    coeff_spacing = 1;
    for (uint32_t i = 0; i < count; i++)
    {
        dts_coeff[i].y_val = 16384;
        dts_coeff[i].k_val = 0;
    }
    return 0;
}
//
// Tests
//

//
// Public Header Tests
//

TEST_FUNCTION(test_platform_power_fuses_get_dts_coeff_tile_supported, set_power_fuse_parameters_tile, NULL)
{
    isFuseServiceUp = true;
    //   Test case where power hardware is supported
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);

    for (uint32_t i = 0; i < 2; i++)
    {
        expect_value(__wrap_read_fuse, fuse_bit_offset, y_offset);
        expect_value(__wrap_read_fuse, fuse_bit_size, y_width);
        will_return(__wrap_read_fuse, 6);

        expect_value(__wrap_read_fuse, fuse_bit_offset, k_offset);
        expect_value(__wrap_read_fuse, fuse_bit_size, k_width);
        will_return(__wrap_read_fuse, 5);

        k_offset += (k_width * coeff_spacing);
        y_offset += (y_width * coeff_spacing);
    }
    platform_power_fuses_get_dts_coeff_tile(dts_coeff, count);
    for (uint32_t i = 0; i < count; i++)
    {
        assert_int_equal(dts_coeff[i].y_val, 6);
        assert_int_equal(dts_coeff[i].k_val, 5);
    }
}

TEST_FUNCTION(test_platform_power_fuses_get_dts_coeff_tile_unsupported, set_power_fuse_parameters_tile, NULL)
{
    // Test case where power hardware is unsupported
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);

    platform_power_fuses_get_dts_coeff_tile(dts_coeff, count);
    for (uint32_t i = 0; i < count; i++)
    {
        assert_int_equal(dts_coeff[i].y_val, 16384);
        assert_int_equal(dts_coeff[i].k_val, 0);
    }
}

TEST_FUNCTION(test_platform_power_fuses_get_dts_coeff_soctop_supported, set_power_fuse_parameters_soctop, NULL)
{
    // Test case where power hardware is supported
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);

    for (uint32_t i = 0; i < 2; i++)
    {
        expect_value(__wrap_read_fuse, fuse_bit_offset, y_offset);
        expect_value(__wrap_read_fuse, fuse_bit_size, y_width);
        will_return(__wrap_read_fuse, 6);

        expect_value(__wrap_read_fuse, fuse_bit_offset, k_offset);
        expect_value(__wrap_read_fuse, fuse_bit_size, k_width);
        will_return(__wrap_read_fuse, 5);

        k_offset += (k_width * coeff_spacing);
        y_offset += (y_width * coeff_spacing);
    }
    platform_power_fuses_get_dts_coeff_soctop(dts_coeff, count);
    for (uint32_t i = 0; i < count; i++)
    {
        assert_int_equal(dts_coeff[i].y_val, 6);
        assert_int_equal(dts_coeff[i].k_val, 5);
    }
}

TEST_FUNCTION(test_platform_power_fuses_get_dts_coeff_soctop_unsupported, set_power_fuse_parameters_soctop, NULL)
{
    // Test case where power hardware is unsupported
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);

    platform_power_fuses_get_dts_coeff_soctop(dts_coeff, count);
    for (uint32_t i = 0; i < count; i++)
    {
        assert_int_equal(dts_coeff[i].y_val, 16384);
        assert_int_equal(dts_coeff[i].k_val, 0);
    }
}
