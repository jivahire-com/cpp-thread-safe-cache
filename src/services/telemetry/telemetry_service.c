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

void telemetry_service_init(void)
{
    // initialize runtime first
    exec_tlm_cmpnt_init();

    data_proc_tlm_cmpnt_init();
    in_band_tlm_cmpnt_init();
    out_of_band_tlm_cmpnt_init();
}