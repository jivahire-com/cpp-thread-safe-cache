/*
 * Copyright (c) Microsoft Corporation.
 */

/**
 * @file mock_odcm.h
 *
 */

#pragma once

/*----------- Nested includes ------------*/
// clang-format off
// stdbool is currently required before odcm.h
#include <stdbool.h>
#include <stdint.h>
#include <odcm.h>

// clang-format on

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
const odcm_config_t *mock_odcm_last_config();
