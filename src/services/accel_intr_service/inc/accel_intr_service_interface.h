//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr_service_interface.h
 * Header file for Accel Interrupt interface with the Accel Interrupt service
 */

#pragma once

/*----------- Nested includes ------------*/
#include "accel_intr_service_dfwk.h"
#include "accel_intr_service.h"

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

typedef struct
{
    paccel_intr_service_interface_t p_interface;
    accel_intr_service_request_t request;
    e_accel_intr_service_command_id_t command_id;
} accel_intr_service_cmd_context_t, *paccel_intr_service_cmd_context_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
