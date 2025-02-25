//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hm_cli.h
 * Header file for health monitor cli
 */

#pragma once

/*----------- Nested includes ------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef struct {
    guid_t guid;
    const char* name;
} guid_name_map_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
void hm_cli_init();
