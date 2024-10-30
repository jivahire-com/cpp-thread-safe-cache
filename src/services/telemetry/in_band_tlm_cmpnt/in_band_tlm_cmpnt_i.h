//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file in_band_tlm_cmpnt_i.h
 * This file defines data and api's shared internally to the component and not with other components.
 */

#pragma once

/*----------- Nested includes ------------*/
#include "telemetry_package_defs.h"

#include <event_trace_providers.h>
#include <fpfw_status.h>
#include <stddef.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/


/*-------------- Typedefs ----------------*/


/*-- Declarations (Statics and globals) --*/

extern uint8_t inband_die_id;
extern uint16_t inband_inst_samples_per_pkg;
/*--------- Function Prototypes ----------*/

