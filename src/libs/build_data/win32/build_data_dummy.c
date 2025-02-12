//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file build_data_dummy.c
 * Used for unit testing
 */

/*------------- Includes -----------------*/
#include <build_data.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

const NOTE_GNU_BUILD_ID g_note_gnu_build_id = {
    .NameSize = 4,
    .BuildIdSize = 20,
    .Type = 3,
    .Name = 0x00554e47,
    .BuildId = {
        0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19
    }
};
/*------------- Functions ----------------*/
