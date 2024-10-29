//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file platform_core_config.h
 * This file contains defines for getting enabled cores mask and core count in various platforms
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include <corebits.h>


/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

// fpga platform has an unusual set of available cores
extern const corebits_t fpga_platform_cores;
extern const corebits_t platform_cores;
extern const corebits_t zebu_cores;
extern const corebits_t svp_cores;

/*--------------------------- Function Prototypes ---------------------------*/
