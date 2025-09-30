//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file data_proc_mock.h
 * This file contains the definitions of structures used for packages for in band telemetry.
 */

#pragma once

/*----------- Nested includes ------------*/
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <mcp_telemetry_shared.h> //for cstate_instr_timestamp_t
/*-- Symbolic Constant Macros (defines) --*/
/*-------------- Typedefs ----------------*/
/*-- Declarations (Statics and globals) --*/
extern cstate_instr_timestamp_t* cstate_tfa_timestamp_base;
/*--------- Function Prototypes ----------*/
void setup_cstate_tfa_mock_buffer(void);
#ifdef __cplusplus
}
#endif


