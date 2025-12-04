//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file in_band_package_interface_i.h
 * API's to retrieve data for in band packaging and out of band reporting
 */

#pragma once

/*----------- Nested includes ------------*/
#include "data_sampling_i.h" // internal APIs

#include <fpfw_status.h>
#include <stdint.h>
#include <telemetry_package_defs.h>
/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Initialize the in band telemetry component.
 * @param[in] mpam_vm_mem_fixed_pwr_mW The fixed power in mW for MPAM VM memory reporting.
 * @param[in] all_zero_filtering_enable True to enable all-zero filtering for diagnostic decoder, false to disable.
 */
void package_inf_init(uint32_t mpam_vm_mem_fixed_pwr_mW, bool all_zero_filtering_enable);