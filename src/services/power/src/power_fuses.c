//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_fuses.c
 * Implementation of power fuse reads
 */

/*------------- Includes -----------------*/
// clang-format off
#include <kng_soc_constants.h> // for NUM_AP_CORES_PER_DIE
// clang-format on

#include "power_runconfig.h"   // for power_fuse_data_t, dts_coeff_t, power...
#include "power_runconfig_i.h" // for power_fuses_get_curve_assignment, pow...

#include <FpFwAssert.h>    // for FPFW_RUNTIME_ASSERT
#include <corebits.h>      // for corebits_set_bit
#include <dvfs_struct.h>   // for dvfs_vft_t, DVFS_DEF_NOMINAL_FREQ
#include <silibs_common.h> // for ARRAY_SIZE
#include <stddef.h>        // for NULL

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
// defaults for gradient/offset equation for platforms where fuse isn't available
#define DVFS_LDODAC_TO_VOLT_SLOPE  2000
#define DVFS_LDODAC_TO_VOLT_OFFSET 2000
// defaults for dts k/y values for platforms where fuse isn't available
#define PVT_DTS_DEFAULT_K_VAL 286
#define PVT_DTS_DEFAULT_Y_VAL 649
// default for TDP power
#define TDP_DEFAULT_POWER_A 400

/*------------- Functions ----------------*/

unsigned int power_fuses_get_curve_assignment(unsigned core)
{
    // TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491015/
    /* temp implementation for curve assignment; always returns 0 */
    FPFW_RUNTIME_ASSERT(core < NUM_AP_CORES_PER_DIE);
    return 0;
}

void power_fuses_read(power_fuse_data_t* p_fuses)
{
    // TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491015/
    // Implement actual fuse code

    /* temp implementation for fuse reads */
    FPFW_RUNTIME_ASSERT(p_fuses != NULL);

    // TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1811925/
    // use system_info_get_core_count() to get core count
    const unsigned core_count = NUM_AP_CORES_PER_DIE; // system_info_get_core_count();

    // set all the valid bits for platform cores; fuses may clear further in
    for (unsigned core_idx = 0; core_idx < core_count; ++core_idx)
    {
        corebits_set_bit(&p_fuses->valid_cores, core_idx);
    }

    // default gradient/offset parameters
    const dvfs_vf_slope_t default_ldodac_to_volt = {
        .slope_uvolt = DVFS_LDODAC_TO_VOLT_SLOPE,
        .offset_uvolt = DVFS_LDODAC_TO_VOLT_OFFSET,
    };
    p_fuses->ldodac_to_volt = default_ldodac_to_volt;
    // default dts_coeff parameters
    const dts_coeff_t default_dts_coeff = {
        .k_val = PVT_DTS_DEFAULT_K_VAL,
        .y_val = PVT_DTS_DEFAULT_Y_VAL,
    };
    for (unsigned int idx = 0; idx < ARRAY_SIZE(p_fuses->dts_coeff_tile); ++idx)
    {
        p_fuses->dts_coeff_tile[idx] = default_dts_coeff;
    }
    for (unsigned int idx = 0; idx < ARRAY_SIZE(p_fuses->dts_coeff_soctop); ++idx)
    {
        p_fuses->dts_coeff_soctop[idx] = default_dts_coeff;
    }

    // need to default num cores
    const power_fuse_tdp_t default_tdp_config = {.num_cores = NUM_AP_CORES_PER_DIE,
                                                 .freq_MHz = DVFS_DEF_NOMINAL_FREQ,
                                                 .power_A = TDP_DEFAULT_POWER_A};
    p_fuses->tdp_config = default_tdp_config;
}
