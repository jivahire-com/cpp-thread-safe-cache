//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file fuse_main.c
 * Implements the primary fuse service interface.
 */

/*------------- Includes -----------------*/

#include <fuse_events.h>
#include <fuse_init.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

// TODO: Actual fuse service implementation is part of ADO
// https://azurecsi.visualstudio.com/Dev/_workitems/edit/908090
// This placeholder here is to verify the Fuse event trace log
void fuse_init()
{
    FUSE_ET_STATUS(FUSE_ET_TYPE_SVC_START);
}
