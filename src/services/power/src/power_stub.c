//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_stub.c
 * Stub implementation for power related functions
 */

/*------------- Includes -----------------*/
#include "power_runconfig_i.h" // for power_runconfig_t, power_runconfig_get

#include <FpFwUtils.h>
#include <dvfs.h>
#include <dvfs_struct.h>
#include <fpfw_status.h>            // for FPFW_STATUS_INVALID_ARGS, FPFW_STATUS_S...
#include <kingsgate_fuse_defines.h> // for TEST_FLOW_CHECKS_PMM_REVISION_BIT_OFFSET
#include <pex_regs.h>               // for PEX_DVFS_NON_ADDRESS, ptr_dvfs_nonsecure_reg
#include <power_runconfig.h>        // for BITS_PER_BYTE, MAX_BITS_PER_FUSE
#include <power_stub_i.h>           // for platform_read_fuse, power_fuses_is_powe...
#include <silibs_platform.h>        // for DVFS
#include <stdbool.h>                // for bool, true
#include <stdint.h>                 // for uint32_t, int32_t, uint64_t
#include <string.h>                 // for memcpy, size_t
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

// Proper implementation for platform power fuse support needs to be added in platform files
// TODO ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/908090
bool power_fuses_is_power_hw_supported()
{
    return true;
}

// Proper implementation for this wrapper must be added at platform level
// TODO ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/908090
int32_t platform_read_fuse(const uint32_t* target_addr, const uint32_t fuse_bit_offset, const uint32_t fuse_bit_size)
{
    uint64_t fuse_data = 0xdeadbeef;

    if ((fuse_bit_size == 0) || (fuse_bit_size > MAX_BITS_PER_FUSE))
    {
        return FPFW_STATUS_INVALID_ARGS;
    }

    // need valid fuse values for DTS Y/K val
    const uint32_t tile_y_start = TILE_THERMALS_SENSOR_RTS_Y_BIT_OFFSET;
    const uint32_t tile_y_end = TILE_THERMALS_SENSOR_RTS_Y_BIT_OFFSET +
                                TILE_THERMALS_SENSOR_RTS_Y_WIDTH * TILE_THERMALS_SENSOR_RTS_Y_ARRAY_ELEMENTS;
    const uint32_t tile_k_start = TILE_THERMALS_SENSOR_RTS_K_BIT_OFFSET;
    const uint32_t tile_k_end = TILE_THERMALS_SENSOR_RTS_K_BIT_OFFSET +
                                TILE_THERMALS_SENSOR_RTS_K_WIDTH * TILE_THERMALS_SENSOR_RTS_K_ARRAY_ELEMENTS;
    const uint32_t top_y_start = TOP_THERMALS_SENSOR_RTS_Y_BIT_OFFSET;
    const uint32_t top_y_end = TOP_THERMALS_SENSOR_RTS_Y_BIT_OFFSET +
                               TOP_THERMALS_SENSOR_RTS_Y_WIDTH * TOP_THERMALS_SENSOR_RTS_Y_ARRAY_ELEMENTS;
    const uint32_t top_k_start = TOP_THERMALS_SENSOR_RTS_K_BIT_OFFSET;
    const uint32_t top_k_end = TOP_THERMALS_SENSOR_RTS_K_BIT_OFFSET +
                               TOP_THERMALS_SENSOR_RTS_K_WIDTH * TOP_THERMALS_SENSOR_RTS_K_ARRAY_ELEMENTS;

    if ((fuse_bit_offset == TEST_FLOW_CHECKS_PMM_REVISION_BIT_OFFSET) ||
        (fuse_bit_offset >= VF_CORE_ANCHOR_SEL_BIT_OFFSET && fuse_bit_size == VF_CORE_ANCHOR_SEL_WIDTH) ||
        (fuse_bit_offset >= CORE_DEFECT_MFG_MASK_CORE_DEFECT_MFG_31_0_BIT_OFFSET &&
         fuse_bit_offset <= CORE_DEFECT_IFT_MASK_CORE_DEFECT_IFT_67_64_BIT_OFFSET))
    {
        fuse_data = 0;
    }
    else if ((fuse_bit_offset >= tile_y_start && fuse_bit_offset < tile_y_end) ||
             (fuse_bit_offset >= top_y_start && fuse_bit_offset < top_y_end))
    {
        fuse_data = (uint64_t)DTS_Y_COEFFICIENT;
    }
    else if ((fuse_bit_offset >= tile_k_start && fuse_bit_offset < tile_k_end) ||
             (fuse_bit_offset >= top_k_start && fuse_bit_offset < top_k_end))
    {
        fuse_data = (uint64_t)(-1.0F * DTS_K_COEFFICIENT);
    }

    // number of valid bytes to copy from fuse_data
    size_t fuse_size = ((fuse_bit_size + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE);
    memcpy((void*)target_addr, (void*)&fuse_data, fuse_size);

    // Always return Success.
    return FPFW_STATUS_SUCCESS;
}
