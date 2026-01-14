//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_boot_notify.h
 * This file contains the definitions for sending boot complete to accelerators
 */
 #pragma once

 /*----------- Nested includes ------------*/
 #include <kng_error.h>
 #include <stdint.h>

 /*-- Symbolic Constant Macros (defines) --*/
 
 /*-------------- Typedefs ----------------*/
 
 /*-- Declarations (Statics and globals) --*/
 
 /*--------- Function Prototypes ----------*/
 KNG_STATUS accel_boot_notify_service( void );
