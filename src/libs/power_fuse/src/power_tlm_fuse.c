//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 *  @file power_tlm_fuse.c
 *  This library provide apis to read the power fuses for telemetry. .
 **/

/*------------- Includes -----------------*/
#include "power_fuse_events_i.h"

#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <bug_check.h>
#include <fpfw_init.h>   // for fpfw_init_get_handle, FPFW_INIT_C...
#include <fpfw_status.h> // for fpfw_status_t
#include <fuse.h>        // fuse library functions
#include <fuse_dma.h>    // apply copy fuse to ram
#include <fuses_top_regs.h>
#include <idsw.h> // SW platform id
#include <idsw_kng.h>
#include <kingsgate_fuse_defines.h> // Test revision get
#include <kng_soc_constants.h>      // for DIE_INSTANCE
#include <limits.h>
#include <power_tlm_fuse.h>
#include <silibs_platform.h> // silibs status and result

/*-- Symbolic Constant Macros (defines) --*/
#define MAX_BYTES_PER_FUSE 8
#define MAX_BITS_PER_FUSE  (MAX_BYTES_PER_FUSE * CHAR_BIT)
#define BITS_PER_BYTE      CHAR_BIT
/**
 * @brief isFuseServiceUp flag to check if fuse service is already up or not , only needed on SCP
 * on MCP it is expected that fuse service will already be up and running  when
 * MCP boot.
 */
bool isFuseServiceUp = false;

/*------------- Typedefs -----------------*/

/*------------- Functions ----------------*/

/**
 * @brief This API read Fuse(DTS k/y coefficients) value using offsets and widths passed.
 * These coefficients values are used by the power telemetry logger
 * to log tile/core/hnf temperature before sending to DDR.
 *
 * Note :  This library can be used on both the processor MCP and SCP.
 * SCP will have some startup dependencies so make sure fuse service is up and running on SCP.
 *
 * @param[in] k_offset - bit offset for specific dts k_val fuse
 * @param[in] k_width - width for specific dts k_val fus
 * @param[in] y_offset - bit offset for specific dts y_val fuse
 * @param[in] y_width - width for specific dts y_val fuse
 * @param[in] coeff_count - size of the array, where we are reading dts coeff.
 * @param[in] coeff_spacing -
 * @param[out] dts_coeff    - coeff array to read the fuse values
 * @return int32_t  FPFW_STATUS_SUCCESS if successful, else FPFW_STATUS_UNEXPECTED/FPFW_STATUS_NULL_POINTER
 */
static fpfw_status_t platform_power_fuses_get_dts_coeff(unsigned int k_offset,
                                                        uint32_t k_width,
                                                        unsigned int y_offset,
                                                        uint32_t y_width,
                                                        uint32_t coeff_count,
                                                        uint32_t coeff_spacing,
                                                        dts_tlm_coeff_t* dts_coeff);

/**
 * @brief Get if platform support the fuse read or not.
 * @param none
 * @return true if platform support the fuse read otherwise false.
 */
static bool platform_power_fuses_is_power_hw_supported();

void power_fuse_init(void)
{
    /* Update fuse serive status to true from runtime init */
    isFuseServiceUp = true;
}
static bool platform_power_fuses_is_power_hw_supported()
{
    bool status = false;
    KNG_PLAT_ID plat_id = idsw_get_platform_sdv();
    switch (plat_id)
    {
    case PLATFORM_RVP_EVT_SILICON:
        status = true;
        break;
    case PLATFORM_RTL_SIM:
        status = true;
        break;
    case PLATFORM_FPGA:
        status = true;
        break;
    case PLATFORM_FPGA_LARGE:
        status = true;
        break;
    case PLATFORM_FPGA_LARGE_RVP:
        status = true;
        break;
    default:

        FPFW_ET_LOG(PlatformPowrFuseNotSupported);
        break;
    }
    return status;
}

/**
 * @brief Read fuse API - This API will return the value of a fuse based on provided
 * bit offset and width within fuse data
 * @param[out] fuse_store_address - Address to store fuse value, location size must be greater than or equal to fuse_bit_width
 * @param[in] fuse_bit_offset - Bit offset of fuse (from header)
 * @param[in] fuse_bit_width - Bit width of fuse (from header, expect max 64)
 *
 * @return FPFW_INIT_STATUS_SUCCES on success
 *
 */
static fpfw_status_t platform_power_fuse_read(const uintptr_t fuse_store_addr,
                                              const unsigned int fuse_bit_offset,
                                              const unsigned int fuse_bit_size)
{
    uint64_t fuse_data = 0;
    if ((fuse_bit_size == 0) || (fuse_bit_size > MAX_BITS_PER_FUSE))
    {
        FPFW_ET_LOG(RequestedFuseBitReadSizeInValid, fuse_bit_size);
        return FPFW_STATUS_INVALID_ARGS;
    }
    fuse_data = read_fuse(fuse_bit_offset, fuse_bit_size);
    // number of valid bytes to copy from fuse_data
    size_t fuse_size = ((fuse_bit_size + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE);
    memcpy((void*)fuse_store_addr, (void*)&fuse_data, fuse_size);

    return FPFW_STATUS_SUCCESS;
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
    if (!dts_coeff)
    {
        return FPFW_STATUS_NULL_POINTER;
    }

    fpfw_status_t status;
    // temp space for fuse data
    uint64_t fuse_data_y = 0;
    uint64_t fuse_data_k = 0;

    for (uint32_t coeff_idx = 0; coeff_idx < coeff_count; ++coeff_idx)
    {
        // read DTS Y value
        status = platform_power_fuse_read((uintptr_t)&fuse_data_y, y_offset, y_width);
        if (FPFW_STATUS_FAILED(status))
        {
            FPFW_ET_LOG(PowerFuseReadFailedY_offset, status);
            return status;
        }
        // a value of 0 for Y is invalid; would cause a divide by 0
        if (fuse_data_y == 0)
        {
            FPFW_ET_LOG(DTScoeffyValIsZeroAt, y_offset);
            /* DTS Y coefficient is 0 (would cause div by 0) hence assign default to 1 */
            fuse_data_y = 1;
            return FPFW_STATUS_UNEXPECTED;
        }

        // read DTS K value
        status = platform_power_fuse_read((uintptr_t)&fuse_data_k, k_offset, k_width);
        if (FPFW_STATUS_FAILED(status))
        {
            FPFW_ET_LOG(PowerFuseReadFailedK_offset, status);
            return status;
        }

        dts_coeff[coeff_idx].y_val = (uint16_t)fuse_data_y;
        dts_coeff[coeff_idx].k_val = (uint16_t)fuse_data_k;

        // update fuse array offsets
        k_offset += (k_width * coeff_spacing);
        y_offset += (y_width * coeff_spacing);
    }

    return FPFW_STATUS_SUCCESS;
}

fpfw_status_t platform_power_fuses_get_dts_coeff_tile(dts_tlm_coeff_t* dts_coeff, uint32_t count)
{
    fpfw_status_t status = FPFW_STATUS_SUCCESS;
    uint32_t i = 0;

    // only read fuses on silicon platform
    if (platform_power_fuses_is_power_hw_supported() && isFuseServiceUp)
    {
        status = platform_power_fuses_get_dts_coeff(TILE_THERMALS_SENSOR_K_BIT_OFFSET,
                                                    TILE_THERMALS_SENSOR_K_WIDTH,
                                                    TILE_THERMALS_SENSOR_Y_BIT_OFFSET,
                                                    TILE_THERMALS_SENSOR_Y_WIDTH,
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

        for (i = 0; i < count; i++)
        {
            dts_coeff[i].y_val = 1; // dummy
            dts_coeff[i].k_val = 1;
        }
        status = FPFW_STATUS_UNEXPECTED;
        FPFW_ET_LOG(PowerFuseReadTileCoeffDeafultValues, status);
    }

    return status;
}

fpfw_status_t platform_power_fuses_get_dts_coeff_soctop(dts_tlm_coeff_t* dts_coeff, uint32_t count)
{
    fpfw_status_t status = FPFW_STATUS_SUCCESS;
    uint32_t i = 0;
    // only read fuses on silicon platform
    if (platform_power_fuses_is_power_hw_supported() && isFuseServiceUp)
    {
        status = platform_power_fuses_get_dts_coeff(TOP_THERMALS_SENSOR_K_BIT_OFFSET,
                                                    TOP_THERMALS_SENSOR_K_WIDTH,
                                                    TOP_THERMALS_SENSOR_Y_BIT_OFFSET,
                                                    TOP_THERMALS_SENSOR_Y_WIDTH,
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
        for (i = 0; i < count; i++)
        {
            dts_coeff[i].y_val = 1;
            dts_coeff[i].k_val = 1;
        }
        status = FPFW_STATUS_UNEXPECTED;
        FPFW_ET_LOG(PowerFuseReadSoCTopCoeffDeafultValues, status);
    }

    return status;
}
