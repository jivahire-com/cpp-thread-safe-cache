//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr.h
 * Header file for Accel Interrupt Library code
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/

#include <accelerator_ip.h> // for ACCELERATOR_CDEDSS, ACCELERATOR_SDMSS
#include <stdint.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/
typedef enum {
    ACCEL_INTR_RET_FAIL_INTR_INIT = (-3), // Total number of error types
    ACCEL_INTR_RET_FAIL_INTR_NVIC,
    ACCEL_INTR_RET_FAIL_TIMER_CREATE,
    ACCEL_INTR_RET_SUCCESS = 0
} eACCEL_INTR_RET_CODES;

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/**
 * @brief Initialize ACCEL interrupts
 *
 * \b Description:
 * This will Initialize all supported ACCEL Interrupts handled in SCP
 * 1. Unmask at Level1 and Level 2
 * 2. Register ISR
 * 3. Enable in NVIC
 *
 * @param[in] accel_type : CDED / SDM accelerator_type
 * @param[in] atu_mapped_address : Address used to calculate Accel Ip register addresses
 * 
 * @retval
 *  On success, ACCEL_INTR_RET_SUCCESS
 *  On failure, ACCEL_INTR_RET_FAIL_*
 */
int accel_intr_irq_init(eACCELERATOR_TYPE accel_type);

/**
 * @brief Function called when ASYNC request for SDM_MSG0_INTR is  received
 *  
 * @param[in] IRQnum : This will have IRQNum to identify if interrupt is received for CDED / SDM
 *
 * @retval void
 *
 */
void accel_intr_handle_sdm_msg_recvd(uint32_t IRQnum);

/**
 * @brief Function called when ASYNC request for FATAL_INTR is received
 *  
 * @param[in] IRQnum : This will have IRQNum to identify if interrupt is received for CDED / SDM
 *
 * @retval void
 *
 */
void accel_intr_handle_fatal_intr_recvd(uint32_t IRQnum);
