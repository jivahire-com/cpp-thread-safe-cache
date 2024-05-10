//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_init.h
 * Header containing definitions for initialization of power service
 */

#pragma once

/*----------- Nested includes ------------*/
#include "power_dfwk.h"

#include <DfwkTypes.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

void power_init(ppower_service_t p_device, PDFWK_SCHEDULE p_schedule);
void power_interface_init(ppower_service_t p_device, ppower_service_interface_t p_interface);

#ifdef __cplusplus
}
#endif