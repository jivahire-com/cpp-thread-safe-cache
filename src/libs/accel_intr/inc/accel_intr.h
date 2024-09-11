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

/**
 * SDM and CDED IRQ Numbers
 * CDEDSS : 0x76 (118)
 * SDMSS  : 0x77 (119)
 */
#define CDEDSS_IRQ_NUMBER           (0x76)
#define SDMSS_IRQ_NUMBER            (0x77)

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/**
 * @brief Get IRQ Number based on accel_type
 * 
 * @param[in] accel_type: CDED / SDM accelerator_type
 * 
 * @retval
 * IRQ Number : 119 (SDM) / 118 (CDED)
 */
uint32_t accel_intr_get_irq_num_from_accel_type(eACCELERATOR_TYPE accel_type);

/**
 * @brief Get accel_type based on IRQ number
 * 
 * @param[in] IRQnum: 119 (SDM) / 118 (CDED)
 * 
 * @retval
 * accel_type : SDM / CDED
 */
eACCELERATOR_TYPE accel_intr_get_accel_type_from_irq_num(uint32_t IRQnum);

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
