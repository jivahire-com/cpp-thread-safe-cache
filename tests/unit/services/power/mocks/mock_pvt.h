/*
 * Copyright (c) Microsoft Corporation.
 */

/**
 * @file mock_pvt.h
 *
 */

#pragma once

/*----------- Nested includes ------------*/
#include <pvt.h>
#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
const tile_pvt_telem_setting_config_t *mock_pvt_last_tile_telem_config();
const pvt_setting_config_t *mock_pvt_last_tile_config();
const pvt_setting_config_t *mock_pvt_last_soc_config();