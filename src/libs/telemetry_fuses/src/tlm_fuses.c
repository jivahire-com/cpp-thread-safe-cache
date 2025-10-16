//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file tlm_fuses.c
 *  This library provide apis to read the fuses for telemetry.
 **/

/*------------- Includes -----------------*/

#include "tlm_fuses_events_i.h"

#include <fpfw_status.h>
#include <fuse.h>
#include <fuses_top_regs.h>
#include <idsw_kng.h>
#include <kingsgate_fuse_defines.h>
#include <kng_soc_constants.h>
#include <limits.h>
#include <silibs_platform.h>
#include <tlm_fuses.h>

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

static void tlm_fuses_fill_default_dts_coeff(int tile_index, uint32_t coeff_count, dts_tlm_coeff_t* dts_coeff)
{
    for (uint32_t coeff_idx = tile_index; coeff_idx < coeff_count; ++coeff_idx)
    {
        dts_coeff[coeff_idx].y_val = DEFAULT_DTS_FUSED_Y_VAL;
        dts_coeff[coeff_idx].k_val = DEFAULT_DTS_FUSED_K_VAL;
    }
}

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
fpfw_status_t tlm_fuses_read(void* fuse_store_addr, const unsigned int fuse_bit_offset, const unsigned int fuse_bit_size)
{
    if ((fuse_bit_size == 0) || (fuse_bit_size > MAX_BITS_PER_FUSE))
    {
        FPFW_ET_LOG(TlmFusesRequestedFuseBitReadSizeInValid, fuse_bit_size);
        return FPFW_STATUS_INVALID_ARGS;
    }

    // Read the fuse and copy the data into the desired location
    uint64_t fuse_data = fuse_read(fuse_bit_offset, fuse_bit_size);
    size_t fuse_size = ((fuse_bit_size + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE);
    memcpy(fuse_store_addr, (void*)&fuse_data, fuse_size);

    return FPFW_STATUS_SUCCESS;
}

fpfw_status_t tlm_fuses_get_dts_coeff(unsigned int k_offset,
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

    for (uint32_t coeff_idx = 0; coeff_idx < coeff_count; ++coeff_idx)
    {
        // Clear data for each fuse read
        uint64_t fuse_data_y = 0;
        uint64_t fuse_data_k = 0;

        // read DTS Y value
        status = tlm_fuses_read(&fuse_data_y, y_offset, y_width);
        if (FPFW_STATUS_FAILED(status))
        {
            FPFW_ET_LOG(TlmFusesReadFailedY_offset, status);
            tlm_fuses_fill_default_dts_coeff(coeff_idx, coeff_count, dts_coeff);
            return status;
        }

        if (fuse_data_y == 0)
        {
            FPFW_ET_LOG(TlmFusesDTScoeffyValIsZeroAt, y_offset);
            fuse_data_y = DEFAULT_DTS_FUSED_Y_VAL;
        }

        // read DTS K value
        status = tlm_fuses_read(&fuse_data_k, k_offset, k_width);
        if (FPFW_STATUS_FAILED(status))
        {
            FPFW_ET_LOG(TlmFusesReadFailedK_offset, status);
            tlm_fuses_fill_default_dts_coeff(coeff_idx, coeff_count, dts_coeff);
            return status;
        }

        if (fuse_data_k == 0)
        {
            FPFW_ET_LOG(TlmFusesDTScoeffyValIsZeroAt, k_offset);
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

void tlm_fuses_init(void)
{
    isFuseServiceUp = true;
}

fpfw_status_t tlm_fuses_get_dts_coeff_tile(dts_tlm_coeff_t* dts_coeff, uint32_t count)
{
    fpfw_status_t status = FPFW_STATUS_SUCCESS;

    // only read fuses on silicon platform
    if (isFuseServiceUp)
    {
        status = tlm_fuses_get_dts_coeff(TILE_THERMALS_SENSOR_RTS_K_BIT_OFFSET,
                                         TILE_THERMALS_SENSOR_RTS_K_WIDTH,
                                         TILE_THERMALS_SENSOR_RTS_Y_BIT_OFFSET,
                                         TILE_THERMALS_SENSOR_RTS_Y_WIDTH,
                                         count,
                                         TILE_PVT_NUM_CHANNELS_DTS,
                                         dts_coeff);
        if (FPFW_STATUS_FAILED(status))
        {
            FPFW_ET_LOG(TlmFusesReadFailedtileCoeff, status);
            return status;
        }
    }
    else
    {
        tlm_fuses_fill_default_dts_coeff(0, count, dts_coeff);
        status = FPFW_STATUS_UNEXPECTED;
        FPFW_ET_LOG(TlmFusesReadTileCoeffDefaultValues, status);
    }

    return status;
}

fpfw_status_t tlm_fuses_get_dts_coeff_soctop(dts_tlm_coeff_t* dts_coeff, uint32_t count)
{
    fpfw_status_t status = FPFW_STATUS_SUCCESS;

    if (isFuseServiceUp)
    {
        status = tlm_fuses_get_dts_coeff(TOP_THERMALS_SENSOR_RTS_K_BIT_OFFSET,
                                         TOP_THERMALS_SENSOR_RTS_K_WIDTH,
                                         TOP_THERMALS_SENSOR_RTS_Y_BIT_OFFSET,
                                         TOP_THERMALS_SENSOR_RTS_Y_WIDTH,
                                         count,
                                         1,
                                         dts_coeff);
        if (FPFW_STATUS_FAILED(status))
        {
            FPFW_ET_LOG(TlmFusesReadFailedSocTopCoeff, status);
            return status;
        }
    }
    else
    {
        tlm_fuses_fill_default_dts_coeff(0, count, dts_coeff);
        status = FPFW_STATUS_UNEXPECTED;
        FPFW_ET_LOG(TlmFusesReadSoCTopCoeffDefaultValues, status);
    }

    return status;
}

fpfw_status_t tlm_fuses_get_ecid(ecid_t* ecid)
{
    fpfw_status_t status = FPFW_STATUS_SUCCESS;

    if (isFuseServiceUp)
    {
        // Read the wafer lot number
        for (uint8_t i = 0; i < ECID_WAFER_LOT_NUMBER_CHAR_SIZE; i++)
        {
            status = tlm_fuses_read(&ecid->wafer_lot_num[i],
                                    ECID_WAFER_LOT_NUMBER_CHAR0_BIT_OFFSET + (i * ECID_WAFER_LOT_NUMBER_CHAR0_WIDTH),
                                    ECID_WAFER_LOT_NUMBER_CHAR0_WIDTH);
            if (FPFW_STATUS_FAILED(status))
            {
                FPFW_ET_LOG(TlmFusesReadEcidWaferLotNum, status, i);
                return status;
            }
        }

        // Read the wafer number
        status = tlm_fuses_read(&ecid->wafer_num, ECID_WAFER_NUMBER_BIT_OFFSET, ECID_WAFER_NUMBER_WIDTH);
        if (FPFW_STATUS_FAILED(status))
        {
            FPFW_ET_LOG(TlmFusesReadEcidWaferNum, status);
            return status;
        }

        // Read the x coordinate
        status = tlm_fuses_read(&ecid->x_coord, ECID_X_COORDINATE_BIT_OFFSET, ECID_X_COORDINATE_WIDTH);
        if (FPFW_STATUS_FAILED(status))
        {
            FPFW_ET_LOG(TlmFusesReadEcidXCoord, status);
            return status;
        }

        // Read the y coordinate
        status = tlm_fuses_read(&ecid->y_coord, ECID_Y_COORDINATE_BIT_OFFSET, ECID_Y_COORDINATE_WIDTH);
        if (FPFW_STATUS_FAILED(status))
        {
            FPFW_ET_LOG(TlmFusesReadEcidYCoord, status);
            return status;
        }

        // Read the parity bit
        status = tlm_fuses_read(&ecid->parity_bits, ECID_ECID_PARITY_BIT_BIT_OFFSET, ECID_ECID_PARITY_BIT_WIDTH);
        if (FPFW_STATUS_FAILED(status))
        {
            FPFW_ET_LOG(TlmFusesReadEcidParityBits, status);
            return status;
        }

        FPFW_ET_LOG(TlmFusesReadEcid,
                    ecid->wafer_lot_num[0],
                    ecid->wafer_lot_num[1],
                    ecid->wafer_lot_num[2],
                    ecid->wafer_lot_num[3],
                    ecid->wafer_lot_num[4],
                    ecid->wafer_lot_num[5],
                    ecid->wafer_lot_num[6],
                    ecid->wafer_lot_num[7],
                    ecid->wafer_lot_num[8],
                    ecid->wafer_num,
                    ecid->x_coord,
                    ecid->y_coord,
                    ecid->parity_bits);
    }
    else
    {
        status = FPFW_STATUS_FAIL;
        FPFW_ET_LOG(TlmFusesReadEcidFuseServiceNotUp);
    }

    return status;
}
