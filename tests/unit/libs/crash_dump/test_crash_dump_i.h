//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_crash_dump_i.h
 * File containing function prototype for tests
 */
#pragma once

/*----------- Nested includes ------------*/
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/
// ECID INFO GUID
#define CD_ECID_INFO_GUID                                  \
    {                                                      \
        0x185084EC, 0xE4AD, 0x4DA1,                        \
        {                                                  \
            0xA4, 0xDF, 0x8A, 0xDC, 0xF3, 0x3D, 0xD1, 0xB4 \
        }                                                  \
    }
// Serial output info GUID
#define CD_SERIAL_OUTPUT_INFO_GUID                         \
    {                                                      \
        0xFB05C11B, 0x2980, 0x4FB6,                        \
        {                                                  \
            0xA6, 0xBE, 0x64, 0xFD, 0xB1, 0xE8, 0x8C, 0x50 \
        }                                                  \
    }

extern "C" {
/*-------------- Typedefs ----------------*/
typedef enum
{
    CUSTOM_CHUNK_CALLBACK_ECID,
    CUSTOM_CHUNK_CALLBACK_SERIAL_OUTPUT,
    CUSTOM_CHUNK_CALLBACK_MAX
} CUSTOM_CHUNK_CALLBACKS;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
}