//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file out_of_band_tlm_cmpnt.c
 * Out of band telemetry processing
 */

/*------------- Includes -----------------*/
#include "out_of_band_tlm_cmpnt.h"

#include "out_of_band_tlm_cmpnt_i.h"

#include <data_proc_tlm_cmpnt.h>
#include <exec_tlm_cmpnt.h>
/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static uint8_t this_die_id;
/*------------- Functions ----------------*/
void out_of_band_tlm_cmpnt_init(uint8_t die_id)
{
    this_die_id = die_id;

    if (this_die_id == PRIMARY_DIE_ID)
    {
        // TODO: register PLDM handlers for sensors here
    }
}

void out_of_band_tlm_cmpnt_handle_new_pldm_requests(void)
{

    // TODO:   need to check exec_tlm_cmpnt_change_telemetry_mode()
    //  if not TLM_OP_MODE_PUBLISHING or TLM_OP_MODE_COLLECTING_DATA return not available

    // This function is a placeholder for handling PLDM requests related to sensor data.
}