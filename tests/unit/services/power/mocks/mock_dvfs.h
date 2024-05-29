/*
 * Copyright (c) Microsoft Corporation.
 */

/**
 * @file mock_dvfs.h
 *
 */

#pragma once

/*----------- Nested includes ------------*/
#include <dvfs.h>
#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
const dvfs_config_t *mock_dvfs_last_config();
const dvfs_vft_t *mock_dvfs_last_vft_config();