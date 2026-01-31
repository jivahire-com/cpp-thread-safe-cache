//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mts_mock.h
 * Mocks for the Message Transfer Service platform specific functions
 */

 #pragma once

 /*----------- Nested includes ------------*/
#include <stdint.h>
#include <tx_api.h>

 /*-- Symbolic Constant Macros (defines) --*/
#define MAX_ICC_BUFFER_SIZE (512)
 /*-------------- Typedefs ----------------*/

 /*-- Declarations (Statics and globals) --*/


 /*--------- Function Prototypes ----------*/
UINT __wrap__tx_thread_sleep(ULONG timer_ticks);
