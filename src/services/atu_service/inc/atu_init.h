//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file atu_init.h
 * Header containing definitions for initialization of atu service
 */

#pragma once

/*----------- Nested includes ------------*/
#include "atu_api.h"

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
void atu_svc_init(atu_service_t* atu_service, PDFWK_SCHEDULE schedule);