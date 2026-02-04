//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file warm_start_tags.h
 * File that contains the tags used during warm starts
 */

#pragma once

/*----------- Nested includes ------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

// Add start data ids here (do not reorder)
typedef enum _mod_ws_data_id_t_
{
    WARM_START_ID_RESERVED           = 0,
    WARM_START_ID_RESERVED_CLI_TEST,
    WARM_START_ID_POWER_FUSE,
    WARM_START_ID_AP_WDT,
    WARM_START_ID_POWER_CAP,
    WARM_START_ID_LAST  // Keep last
} mod_ws_data_id_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
