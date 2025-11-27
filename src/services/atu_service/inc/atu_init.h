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
 * @param accel_id SDM or CDED accel device ID
 */
void accel_atu_config(ACCEL_ID accel_id);

/**
 * @brief Config ATU mapping for PF cfg space of accel devices
 * 
 * @param accel_id SDM or CDED accel device ID
 * @retval SILIBS_SUCCESS - everything okay 
 *         else otherwise
 */
int accel_atu_pf_ap_config(ACCEL_ID accel_id, atu_api_type_t operation);

/**
 * @brief Returns ATU address for passed accel device
 * 
 * @param accel_id SDM or CDED accel device ID
 * @return uint32_t ATU address of given accel device.
 * zero is returned in case invalid accel id is passed
 */
uint32_t atu_svc_accel_atu_addr(ACCEL_ID accel_id);

/**
 * @brief Returns PF config ATU address for passed accel device
 * 
 * @param accel_id SDM or CDED accel device ID
 * @return uint32_t ATU address of PF config space for given accel device.
 * zero is returned in case invalid accel id is passed
 */
uint32_t atu_svc_accel_ap_pf_cfg_atu_addr(ACCEL_ID accel_id);
