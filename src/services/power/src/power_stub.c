//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_stub.c
 * Stub implementation for power related functions
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h>
#include <fpfw_status.h>            // for FPFW_STATUS_INVALID_ARGS, FPFW_STATUS_S...
#include <kingsgate_fuse_defines.h> // for TEST_FLOW_CHECKS_PMM_REVISION_BIT_OFFSET
#include <power_runconfig.h>        // for BITS_PER_BYTE, MAX_BITS_PER_FUSE
#include <power_stub_i.h>           // for platform_read_fuse, power_fuses_is_powe...
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

    if ((fuse_bit_offset == TEST_FLOW_CHECKS_PMM_REVISION_BIT_OFFSET) ||
        (fuse_bit_offset >= VF_CORE_ANCHOR_SEL_BIT_OFFSET && fuse_bit_size == VF_CORE_ANCHOR_SEL_WIDTH) ||
        (fuse_bit_offset >= CORE_DISABLE_CORE_DISABLE0_BIT_OFFSET && fuse_bit_offset <= CORE_DISABLE_CORE_DISABLE2_BIT_OFFSET))
    {
        fuse_data = 0;
    }

    // number of valid bytes to copy from fuse_data
    size_t fuse_size = ((fuse_bit_size + (BITS_PER_BYTE - 1)) / BITS_PER_BYTE);
    memcpy((void*)target_addr, (void*)&fuse_data, fuse_size);

    // Always return Success.
    return FPFW_STATUS_SUCCESS;
}

// Implement force_pmin
// TODO ADO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491042/
void power_hw_clear_force_pmin(power_pmin_type_t type)
{
    FPFW_UNUSED(type);
}

void power_hw_force_pmin(power_pmin_type_t type)
{
    FPFW_UNUSED(type);
}
