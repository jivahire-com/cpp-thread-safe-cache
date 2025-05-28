//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_dfwk.h
 * Interface of crash dump driver framework.
 */
#pragma once

/*----------- Nested includes ------------*/
#include <DfwkDriver.h>      // for DFWK_DEVICE_HEADER, DFWK_INTERFACE_HEADER

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
/**
 * @brief Crash dump Device structure.
 * 
 */
typedef struct {
    DFWK_DEVICE_HEADER Header;
    DFWK_QUEUE Queue;
} crash_dump_device_t, *pcrash_dump_device_t;

/**
 * @brief Crash dump Interface structure.
 * 
 */
typedef struct {
    DFWK_INTERFACE_HEADER Header;
    pcrash_dump_device_t Device;
} crash_dump_interface_t, *pcrash_dump_interface_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
void crash_dump_device_initialize(pcrash_dump_device_t device, PDFWK_SCHEDULE schedule);
void crash_dump_interface_initialize(pcrash_dump_interface_t intf, pcrash_dump_device_t device);