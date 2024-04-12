//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file Debug.h
 * Debug public API
 */

#pragma once

/*----------- Nested includes ------------*/

/*-- Symbolic Constant Macros (defines) --*/
#define DBGPRINT(...)   printf(__VA_ARGS__)
#define UNUSED(x) (void)(x)

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
void debug_init();
