//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file crash_dump_serial.h
 *  Public API to include serial output into crash dump
 */

#pragma once

/*--------------- Includes ---------------*/
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
void crash_dump_write_serial_byte(uint8_t byte);
