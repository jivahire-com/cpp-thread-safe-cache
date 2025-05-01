//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file power_tlm_fuse.c
 *  This library provide apis to read the power fuses for telemetry. .
 **/

/*------------- Includes -----------------*/

#include "power_fuse_events_i.h"

#include <fpfw_status.h>
#include <fuse.h>
#include <fuses_top_regs.h>
#include <idsw_kng.h>
#include <kingsgate_fuse_defines.h>
#include <kng_soc_constants.h>
#include <limits.h>
#include <power_tlm_fuse.h>
#include <silibs_platform.h>

/*-- Symbolic Constant Macros (defines) --*/

#define BITS_PER_BYTE (CHAR_BIT)

/**
 * @brief isFuseServiceUp flag to check if fuse service is already up or not, only needed on SCP.
 * On MCP it is expected that fuse service will already be up and running from the SCP when the
 * MCP boots.
 */
bool isFuseServiceUp = false;

/*------------- Typedefs -----------------*/

/*------------- Functions ----------------*/

//
// Private Functions
//

/**
 * @brief Read a fuse.
 *
 * @param[out] fuse_store_address - Address to store fuse value, location size must be greater than or equal to fuse_bit_width
 * @param[in] fuse_bit_offset - Bit offset of fuse (from header)
 * @param[in] fuse_bit_width - Bit width of fuse (from header, expect max 64)
 *
 * @return FPFW_INIT_STATUS_SUCCESS on success
 *
 */
static fpfw_status_t platform_power_fuse_read(const uintptr_t fuse_store_addr,
                                              const unsigned int fuse_bit_offset,
                                              const unsigned int fuse_bit_size)
{
    if ((fuse_bit_size == 0) || (fuse_bit_size > MAX_BITS_PER_FUSE))
    {
        FPFW_ET_LOG(RequestedFuseBitReadSizeInValid, fuse_bit_size);
        return FPFW_STATUS_INVALID_ARGS;
    }

    // Read the fuse and copy the data into the desired location
    uint64_t fuse_data = fuse_read(fuse_bit_offset, fuse_bit_size);
    size_t fuse_size = ((fuse_bit_size + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE);
    memcpy((void*)fuse_store_addr, (void*)&fuse_data, fuse_size);

    return FPFW_STATUS_SUCCESS;
}

static void platform_power_fuses_fill_default_dts_coeff(int tile_index, uint32_t coeff_count, dts_tlm_coeff_t* dts_coeff)
{
    for (uint32_t coeff_idx = tile_index; coeff_idx < coeff_count; ++coeff_idx)
    {
        dts_coeff[coeff_idx].y_val = DEFAULT_DTS_FUSED_Y_VAL;
        dts_coeff[coeff_idx].k_val = DEFAULT_DTS_FUSED_K_VAL;
    }
}

static fpfw_status_t platform_power_fuses_get_dts_coeff(unsigned int k_offset,
                                                        uint32_t k_width,
                                                        unsigned int y_offset,
                                                        uint32_t y_width,
                                                        uint32_t coeff_count,
                                                        uint32_t coeff_spacing,
                                                        dts_tlm_coeff_t* dts_coeff)
{
    // check for NULL parameter
    if (dts_coeff == NULL)
    {
        return FPFW_STATUS_NULL_POINTER;
    }

    fpfw_status_t status;

    //
    // Read each dts coefficient from the fuses. If any fuses fails to be read, fill
    // the remaining dts coefficients with default values.
    //
    // If a y\k val fuse read is successful, but the value is 0, assign a default value.
    //

    uint64_t fuse_data_y = 0;
    uint64_t fuse_data_k = 0;

    for (uint32_t coeff_idx = 0; coeff_idx < coeff_count; ++coeff_idx)
    {
        // read DTS Y value
        status = platform_power_fuse_read((uintptr_t)&fuse_data_y, y_offset, y_width);
        if (FPFW_STATUS_FAILED(status))
        {
            FPFW_ET_LOG(PowerFuseReadFailedY_offset, status);
            platform_power_fuses_fill_default_dts_coeff(coeff_idx, coeff_count, dts_coeff);
            return status;
        }

        if (fuse_data_y == 0)
        {
            FPFW_ET_LOG(DTScoeffyValIsZeroAt, y_offset);
            fuse_data_y = DEFAULT_DTS_FUSED_Y_VAL;
        }

        // read DTS K value
        status = platform_power_fuse_read((uintptr_t)&fuse_data_k, k_offset, k_width);
        if (FPFW_STATUS_FAILED(status))
        {
            FPFW_ET_LOG(PowerFuseReadFailedK_offset, status);
            platform_power_fuses_fill_default_dts_coeff(coeff_idx, coeff_count, dts_coeff);
            return status;
        }

        if (fuse_data_k == 0)
        {
            FPFW_ET_LOG(DTScoeffyValIsZeroAt, k_offset);
            fuse_data_k = DEFAULT_DTS_FUSED_K_VAL;
        }

        dts_coeff[coeff_idx].y_val = (uint16_t)fuse_data_y;
        dts_coeff[coeff_idx].k_val = (uint16_t)fuse_data_k;

        // update fuse array offsets
        k_offset += (k_width * coeff_spacing);
        y_offset += (y_width * coeff_spacing);
    }

    return FPFW_STATUS_SUCCESS;
}

//
// Public Functions
//

void power_fuse_init(void)
{
    isFuseServiceUp = true;
}

fpfw_status_t platform_power_fuses_get_dts_coeff_tile(dts_tlm_coeff_t* dts_coeff, uint32_t count)
{
    fpfw_status_t status = FPFW_STATUS_SUCCESS;

    // only read fuses on silicon platform
    if (isFuseServiceUp)
    {
        status = platform_power_fuses_get_dts_coeff(TILE_THERMALS_SENSOR_RTS_K_BIT_OFFSET,
                                                    TILE_THERMALS_SENSOR_RTS_K_WIDTH,
                                                    TILE_THERMALS_SENSOR_RTS_Y_BIT_OFFSET,
                                                    TILE_THERMALS_SENSOR_RTS_Y_WIDTH,
                                                    count,
                                                    TILE_PVT_NUM_CHANNELS_DTS,
                                                    dts_coeff);
        if (FPFW_STATUS_FAILED(status))
        {
            FPFW_ET_LOG(PowerFuseReadFailedtileCoeff, status);
            return status;
        }
    }
    else
    {
        platform_power_fuses_fill_default_dts_coeff(0, count, dts_coeff);
        status = FPFW_STATUS_UNEXPECTED;
        FPFW_ET_LOG(PowerFuseReadTileCoeffDeafultValues, status);
    }

    return status;
}

fpfw_status_t platform_power_fuses_get_dts_coeff_soctop(dts_tlm_coeff_t* dts_coeff, uint32_t count)
{
    fpfw_status_t status = FPFW_STATUS_SUCCESS;

    if (isFuseServiceUp)
    {
        status = platform_power_fuses_get_dts_coeff(TOP_THERMALS_SENSOR_RTS_K_BIT_OFFSET,
                                                    TOP_THERMALS_SENSOR_RTS_K_WIDTH,
                                                    TOP_THERMALS_SENSOR_RTS_Y_BIT_OFFSET,
                                                    TOP_THERMALS_SENSOR_RTS_Y_WIDTH,
                                                    count,
                                                    1,
                                                    dts_coeff);
        if (FPFW_STATUS_FAILED(status))
        {
            FPFW_ET_LOG(PowerFuseReadFailedSocTopCoeff, status);
            return status;
        }
    }
    else
    {
        platform_power_fuses_fill_default_dts_coeff(0, count, dts_coeff);
        status = FPFW_STATUS_UNEXPECTED;
        FPFW_ET_LOG(PowerFuseReadSoCTopCoeffDefaultValues, status);
    }

    return status;
}
