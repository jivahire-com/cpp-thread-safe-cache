//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr.h
 * Header file for Accel Interrupt Library code
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/

#include <accelip_id.h>             // for ACCEL_ID_CDED, ACCEL_ID_SDM
#include <stdbool.h>
#include <stdint.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/
typedef enum {
    ACCEL_INTR_RET_FAIL_VIRT_IRQ_REG = (-4), // Total number of error types
    ACCEL_INTR_RET_FAIL_INTR_INIT,
    ACCEL_INTR_RET_FAIL_INTR_NVIC,
    ACCEL_INTR_RET_FAIL_TIMER_CREATE,
    ACCEL_INTR_RET_SUCCESS = 0
} eACCEL_INTR_RET_CODES;

/**
 * @brief Configurations for ACCEL Interrupt Initialization
 * E_ACCEL_INTR_INIT_FULL_INTR_TREE: Initialize all levels in the tree.
 * E_ACCEL_INTR_INIT_ONLY_LEVEL1_INTR: Initialize only Level 1 interrupts.
 *
 */
typedef enum {
    E_ACCEL_INTR_INIT_FULL_INTR_TREE,
    E_ACCEL_INTR_INIT_ONLY_LEVEL1_INTR,
    E_ACCEL_INTR_INIT_MAX,
} E_ACCEL_INTR_INIT_CONFIG;

/**
 * SDM and CDED IRQ Numbers
 * CDEDSS : 0x76 (118)
 * SDMSS  : 0x77 (119)
 */
#define CDEDSS_IRQ_NUMBER           (0x76)
#define SDMSS_IRQ_NUMBER            (0x77)

/**
 * ACCEL EmCPU WDT Enable / Disable
 */
#define ACCEL_INTR_DISABLE_ACCEL_EMCPU_WDT          (0x0)
#define ACCEL_INTR_ENABLE_ACCEL_EMCPU_WDT           (0x1)

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/**
 * @brief Set IRQ Number based on accel_type
 *
 * @param[in] accel_type: CDED / SDM accelerator_type
 * @param[in] irq_num: NVIC Irq num of the accelarator for the corresponding core
 * @retval
 */
void accel_intr_set_irq_num_for_accel(ACCEL_ID accel_type, uint32_t irq_num);

/**
 * @brief Get IRQ Number based on accel_type
 *
 * @param[in] accel_type: CDED / SDM accelerator_type
 *
 * @retval
 * IRQ Number : 119 (SDM) / 118 (CDED)
 */
uint32_t accel_intr_get_irq_num_from_accel_type(ACCEL_ID accel_type);

/**
 * @brief Get accel_type based on IRQ number
 *
 * @param[in] IRQnum: 119 (SDM) / 118 (CDED)
 *
 * @retval
 * accel_type : SDM / CDED
 */
ACCEL_ID accel_intr_get_accel_type_from_irq_num(uint32_t IRQnum);

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
 *
 * @retval
 *  On success, ACCEL_INTR_RET_SUCCESS
 *  On failure, ACCEL_INTR_RET_FAIL_*
 */
int32_t accel_scp_intr_init(ACCEL_ID accel_type);

/**
 * @brief Initialize ACCEL interrupts
 *
 * \b Description:
 * This will Initialize all supported ACCEL Interrupts handled in MCP
 * 1. Unmask at Level1 and Level 2
 * 2. Register ISR
 * 3. Enable in NVIC
 *
 * @param[in] accel_type : CDED / SDM accelerator_type
 *
 * @retval
 *  On success, ACCEL_INTR_RET_SUCCESS
 *  On failure, ACCEL_INTR_RET_FAIL_*
 */
int32_t accel_mcp_intr_init(ACCEL_ID accel_type);

/**
 * @brief Function called when ASYNC request for FATAL_INTR is received
 *
 * @param[in] accel_type : CDED / SDM accelerator_type
 *
 * @retval void
 *
 */
void accel_intr_handle_fatal_intr_recvd(ACCEL_ID accel_type);

/**
 * @brief enable interrupt in NVIC
 *
 * @param[in] accel_type : CDED / SDM accelerator_type
 *
 * @retval int32_t
 * On Success : ACCEL_INTR_RET_SUCCESS
 * On Failure : ACCEL_INTR_RET_FAIL_INTR_NVIC
 *
 */
int32_t accel_intr_init(ACCEL_ID accel_type);

/**
 * @brief Initialize SDM/CDED interrupts for SCP core
 *
 * @param[in] accel_type : Accel type (SDM /CDED)
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 */
void accel_intr_scp_init(ACCEL_ID accel_type, uint32_t ext_cfg_addr);

/**
 * @brief Enable / Disable ACCEL emCPU Watchdog
 *
 *
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 * @param[in] enable : ACCEL_INTR_ENABLE_ACCEL_EMCPU_WDT for enable and ACCEL_INTR_DISABLE_ACCEL_EMCPU_WDT for disable
 *
 * @retval void
 */
void accel_intr_emcpu_wdt_control(uint32_t ext_cfg_addr, uint8_t enable);

/**
 * @brief This function enables error interrupts from Accel IP once the core has booted
 *
 * @param accel_type Type of Accel IP (SDM / CDED)
 * @retval void
 */
void accel_intr_scp_err_intr_enable(ACCEL_ID accel_type);

/**
 * @brief This function returns if CD collection needs to be skipped
 *
 * @param accel_type
 *
 * @retval true/false
 */
bool accel_intr_get_cd_skip(ACCEL_ID accel_type);
