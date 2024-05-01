//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_polling.c
 * This file contains the DIMM polling functionality.
 * DDR I3C calls through DFWK
 * BWL/Throttling
 * Telemetry reporting
 *
 * New for KNG: Each die will have 2 I3C controllers, addressing 3 DIMMs each
 */

/*------------- Includes -----------------*/
// Need ddr I3C driver
#include "ddr_manager_i.h"

#include <stdio.h>

// Tell cspell to ignore .. why are we using cSpell?
/* cSpell:ignore DIMM DIMMS DFWK */

/*-- Symbolic Constant Macros (defines) --*/
#define MAX_DIMM_IDX (5) // Each die will address 6 DIMMs

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void ddr_poll_dimms()
{
    ddr_poll_low_dimms();
    ddr_poll_high_dimms();
}

void ddr_poll_low_dimms()
{
    // DIMMS 0-2
    static int i3c0_dimm_idx = 0;
    printf("I3C0: Polling DIMM %d\n", i3c0_dimm_idx);

    // Todo: implement as async. calls to DDR I3C driver with callback that puts event on DDR manager queue
    // DFWK DDR I3C read with callback: Poll both DIMM temperature sensors (ch. 0/1)
    // Poll DIMM PMIC power register
    // Send all to telemetry driver

    i3c0_dimm_idx++;
    if (i3c0_dimm_idx > 2)
    {
        i3c0_dimm_idx = 0;
    }
}

void ddr_poll_high_dimms()
{
    // DIMMS 3-5
    static int i3c1_dimm_idx = 3;
    printf("I3C1: Polling DIMM %d\n", i3c1_dimm_idx);

    // Todo: implement as async. calls to DDR I3C driver with callback that puts event on DDR manager queue
    // DFWK DDR I3C read with callback: Poll both DIMM temperature sensors (ch. 0/1)
    // Poll DIMM PMIC power register
    // Send all to telemetry driver

    i3c1_dimm_idx++;
    if (i3c1_dimm_idx > 5)
    {
        i3c1_dimm_idx = 3;
    }
}

void ddr_process_i3c_data()
{
    // Process I3C data callback from DDR I3C driver when implemented
    printf("Processing DDR I3C data\n");
}