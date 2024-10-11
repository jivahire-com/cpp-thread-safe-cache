//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr_client.h
 * Header file for Accel Interrupt Client Service (This provides functions to call from ISR)
 */

#pragma once

/*----------- Nested includes ------------*/
#include "accel_intr_service_dfwk.h"  // for paccel_intr_service_interface_t
#include <stdint.h>                   // for uint32_t

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Send SDM_MSG Interrupt received notification to Accel Interrupt device
 *
 *
 * @param[in] IRQnum : IRQnum on which interrupt is received
 *
 * @retval void
 * 
 */
void send_sdm_msg_async_request(uint32_t IRQnum);

/**
 * @brief Send FATAL Interrupt received notification to Accel Interrupt device
 *
 *
 * @param[in] IRQnum : IRQnum on which interrupt is received
 *
 * @retval void
 * 
 */
void send_fatal_intr_async_request(uint32_t IRQnum);

/**
 * @brief Init for accel_intr client : Opens Interface and Registers Async Request
 *
 *
 * @param[in] p_interface : Interface to initialize
 *
 * @retval 
 * void
 */
void accel_intr_client_init(paccel_intr_service_interface_t p_interface);

/**
 * @brief Send mailbox Interrupt received notification to Accel Interrupt device
 *
 *
 * @param[in] IRQnum : IRQnum on which interrupt is received
 *
 * @retval 
 * void
 */
void send_mailbox_async_request(uint32_t IRQnum);
