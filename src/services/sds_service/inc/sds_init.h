//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sds_init.h
 * Header containing definitions for initialization of sds service
 */

#pragma once

/*----------- Nested includes ------------*/
#include "sds_api.h"

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
void sds_init(psds_service_t p_device, PDFWK_SCHEDULE p_schedule);
void sds_interface_init(psds_service_t p_device, psds_service_interface_t p_interface);