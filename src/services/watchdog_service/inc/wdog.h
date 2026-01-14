//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file wdog.h
 * This file contains the definitions for the watchdog service.
 */
 #pragma once

 /*----------- Nested includes ------------*/
 #include <kng_error.h>
 #include <stdint.h>

 /*-- Symbolic Constant Macros (defines) --*/
 
 /*-------------- Typedefs ----------------*/
 
 /*-- Declarations (Statics and globals) --*/
 
 /*--------- Function Prototypes ----------*/
 KNG_STATUS wdog_service_init(uint32_t timeout_in_s, uint32_t wdog_freq);
