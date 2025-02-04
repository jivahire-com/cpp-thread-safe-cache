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
#include <accelip_id.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
void atu_svc_init(atu_service_t* atu_service, PDFWK_SCHEDULE schedule);

/**
 * @brief Config ATU mapping for accel devices 
 * 
 */
void accel_atu_config();

/**
 * @brief Returns ATU address for passed accel device
 * 
 * @param accel_id SDM or CDED accel device ID
 * @return uint32_t ATU address of given accel device.
 * zero is returned in case invalid accel id is passed
 */
uint32_t atu_svc_accel_atu_addr(ACCEL_ID accel_id);
