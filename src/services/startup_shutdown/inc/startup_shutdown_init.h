// Copyright Microsoft. All rights reserved.

/**
 * @file startup_shutdown_init.h
 * This file contains the interface needed for startup/shutdown service init
 */

#pragma once

/*----------- Nested includes ------------*/

#include <DfwkCommon.h>
#include <stdbool.h>
#include <fpfw_icc_base.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef struct {
    DFWK_DEVICE_HEADER header;
    DFWK_QUEUE default_queue;
} sos_device_t, *psos_device_t;

typedef struct  {
    DFWK_INTERFACE_HEADER header;
    psos_device_t p_device;
} sos_interface_t, *psos_interface_t;


/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

void sos_interface_init(psos_device_t p_device, psos_interface_t p_interface, bool shared);
void sos_init(psos_device_t p_device, PDFWK_SCHEDULE p_schedule);
void sos_icc_init(fpfw_icc_base_ctx_t* icc_ctx);
