//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip_priv.h
 * Accelerator IP specific definitions to be consumed by FW.
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include <accelerator_ip.h>
#include <accelip_id.h>
#include <boot_status.h>
#include <tower_vab.h>

/* Target SoC Register Includes */
#define __NO_CSR_TYPEDEFS__     // Needed to avoid huge buffers in ap_top_regs.h and vab_sdm_top_regs.h
#define __NO_ADDRMAP_TYPEDEFS__ // Needed to avoid huge buffers in ap_top_regs.h

#include <ap_top_regs.h>
#include <vab_sdm_top_regs.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

#define ACCEL_NAME_LEN 8

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/
/**
 * @brief Return Accelerator contaxt data
 *
 * \b Description:
 * Based on the provided input data, this function will return the corresponding
 * accelerator context data.
 *
 * @param[out] accel_instance_size
 *  Return Accel ctxt array size.
 *
 * @retval
 *  On success, Accelerator context data
 * 
 */
subsystem_ctxt_t *get_accelerator_ctxt(uint32_t* accel_instance_size);

/**
 * @brief Send accel boot status code to HSP
 * 
 * @param led_boot_status LED Boot status code to send
 */
void accel_send_led_boot_status(led_status_codes_t led_boot_status);
