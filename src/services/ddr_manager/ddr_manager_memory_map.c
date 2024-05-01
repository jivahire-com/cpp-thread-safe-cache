//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @ddr_memory_map.c
 * This file contains the implementation for creating a memory map that will be passed to UEFI.
 * The initial memory map is requested from Silicon Libs, reserved regions are added, and the map is
 * reformatted.
 */

/*------------- Includes -----------------*/
#include "ddr_manager_i.h"

#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void ddr_create_memory_map()
{
    printf("Creating DDR Memory Map\n");
    // Get DDR Memory Map from SiLibs - Task 1485459: Retrieve Memory Maps

    // Add reserved regions

    // Reformat memory map

    // Pass to UEFI through the HSP mailbox as a UEFI variable (same as CC)
}