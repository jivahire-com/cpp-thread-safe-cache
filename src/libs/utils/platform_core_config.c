//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file platform_core_config.c
 *
 * This file contains defines for getting enabled cores mask and core count in various platforms
 *
 */

/*------------- Includes -----------------*/
#include <platform_core_config.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

const corebits_t fpga_platform_cores = (corebits_t)COREBITS_INIT_UINT32(0x000c0300, 0x00c03000, 0);
const corebits_t platform_cores = (corebits_t)COREBITS_INIT_UINT32(0xFFFFFFFF, 0xFFFFFFFF, 0x3);
const corebits_t zebu_cores_8C_model = (corebits_t)COREBITS_INIT_UINT32(0xFF, 0x0, 0x0);
const corebits_t svp_cores = (corebits_t)COREBITS_INIT_UINT32(0xF, 0x0, 0x0);
const corebits_t svp_min_config_cores = (corebits_t)COREBITS_INIT_UINT32(0x01, 0x0, 0x0);

/*------------- Functions ----------------*/