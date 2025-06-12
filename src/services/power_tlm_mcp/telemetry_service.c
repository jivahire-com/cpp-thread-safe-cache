//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file telemetry_service.c
 * High level telemetry service implementation.
 */

/*------------- Includes -----------------*/
#include "data_proc_tlm_cmpnt.h"
#include "exec_tlm_cmpnt.h"
#include "in_band_tlm_cmpnt.h"
#include "out_of_band_tlm_cmpnt.h"

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void telemetry_service_init(uint8_t die_id,
                            uint32_t pwr_pkg_period_ms,
                            uint32_t inst_pkg_sample_period_ms,
                            uint16_t inst_samples_per_pkg,
                            uint32_t _24_hr_pkg_sample_period_ms)
{
    // initialize runtime first
    exec_tlm_cmpnt_init(die_id, pwr_pkg_period_ms, inst_pkg_sample_period_ms, _24_hr_pkg_sample_period_ms);

    data_proc_tlm_cmpnt_init(die_id);
    in_band_tlm_cmpnt_init(die_id, inst_samples_per_pkg);
    out_of_band_tlm_cmpnt_init();
}