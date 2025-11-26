//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_dfwk.h
 * Interface of DDR driver framework.
 */
#pragma once

/*----------- Nested includes ------------*/
#include <DfwkDriver.h>      // for DFWK_DEVICE_HEADER, DFWK_INTERFACE_HEADER

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
/**
 * @brief DDR Device structure.
 *
 */
typedef struct {
    DFWK_DEVICE_HEADER Header;
    DFWK_QUEUE Queue;
} ddr_device_t, *pddr_device_t;

/**
 * @brief Crash dump Interface structure.
 *
 */
typedef struct {
    DFWK_INTERFACE_HEADER Header;
    pddr_device_t Device;
} ddr_interface_t, *pddr_interface_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Dispatch function for DDR Manager DFWK requests.
 *
 * @param request Pointer to the async request header.
 * @param context Pointer to the context (DDR device).
 */
void ddr_manager_dfwk_dispatch(PDFWK_ASYNC_REQUEST_HEADER request, void* context);

/**
 * @brief Initialize the crash dump device.
 *
 * @param device
 * @param schedule
 */
void ddr_manager_dfwk_init();
