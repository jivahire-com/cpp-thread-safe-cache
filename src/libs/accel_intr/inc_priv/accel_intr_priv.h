//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr_priv.h
 * Private header file for Accel Interrupt data types, variables and functions
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include "accel_intr.h"

#include <accelip_id.h>              // for ACCEL_ID_CDED, ACCEL_ID_SDM
#include <cded_interrupts.h>
#include <cded_regs_regs_regs.h>
#include <bitops.h>
#include <fpfw_timer_port.h>         // for _fpfw_timer_t
#include <sdm_ext_interrupts.h>
#include <stdint.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/**
 * List of sdm_ext interrupts that are handled in SCP
 */
typedef enum {
    ACCEL_SCP_INTR_EMCPU_WDT_ERR = 0x0,             // Maps to SDM_EXT_WDT_ERR_INTR
    ACCEL_SCP_INTR_TCM_UE_ECC_ERR,                  // Maps to SDM_EXT_TCM_UE_ECC_ERR_INTR
    ACCEL_SCP_INTR_EMCPU_LOCKUP_ERR,                // Maps to SDM_EXT_EMCPU_LOCKUP_ERR_INTR
    ACCEL_SCP_INTR_SDM_MSG0_INTR,                   // Maps to SDM_EXT_SDM_MSG0_INTR
    ACCEL_SCP_INTR_MAX
} eACCEL_SCP_INTR;

/**
 * List of sdm_ext interrupts that are handled in MCP
 */
typedef enum {
    ACCEL_MCP_INTR_FIRST,
    ACCEL_MCP_INTR_MBX_I2E = ACCEL_MCP_INTR_FIRST,      // Maps to SDM_EXT_MBX_I2E_INTR
    ACCEL_MCP_INTR_MAX,
} eACCEL_MCP_INTR;

#define ACCEL_INTR_GET_DERIVED_ADDR(addr, ext_addr) \
    (addr + SDM_EXT_CFG__ADDRESSBLOCK_0X100000_ADDRESS + ext_addr)

/**
 * Bit position for value returned from accel_irq_fn
 */
#define ACCEL_INTR_VALID_IRQ_BIT_POS              (0x1)
#define ACCEL_INTR_RESET_SOC_BIT_POS              (0x2)
#define ACCEL_INTR_RESET_ACCEL_EMCPU_POS          (0x3)

/**
 * Set value for given function
 */
#define ACCEL_INTR_SET_INTERRUPT_VALID(val)         (SET_BIT(val, ACCEL_INTR_VALID_IRQ_BIT_POS))
#define ACCEL_INTR_SET_RESET_SOC(val)               (SET_BIT(val, ACCEL_INTR_RESET_SOC_BIT_POS))
#define ACCEL_INTR_SET_RESET_ACCEL_EMCPU(val)       (SET_BIT(val, ACCEL_INTR_RESET_ACCEL_EMCPU_POS))

/**
 * Check if given value is set
 */
#define ACCEL_INTR_IS_INTERRUPT_VALID_SET(val)      (BITWISE_AND(val, SET_BIT_MASK(ACCEL_INTR_VALID_IRQ_BIT_POS)) != 0x0)
#define ACCEL_INTR_IS_RESET_SOC_SET(val)            (BITWISE_AND(val, SET_BIT_MASK(ACCEL_INTR_RESET_SOC_BIT_POS)) != 0x0)
#define ACCEL_INTR_IS_RESET_ACCEL_EMCPU_SET(val)    (BITWISE_AND(val, SET_BIT_MASK(ACCEL_INTR_RESET_ACCEL_EMCPU_POS)) != 0x0)

/**
 * Macro to indicate if an interrupt needs to be handled / processed
 */
#define ACCEL_INTR_PROCESS_INTR_IN_BOTTOM_HALF       (true)
#define ACCEL_INTR_PROCESS_INTR_IN_TOP_HALF          (false)

/**
 * Buffer length to store CDED CP interrupts brief strings
 */
#define CDED_CP_INT_TRACE_STR_LEN                       32

/*-------------------------------- Typedefs ---------------------------------*/

// Structure to store interrupt specific data and functions
// These values will be used during init and thread code.
typedef struct
{
    SDM_EXT_INTERRUPT_NUMBER accel_irq_bit;
    uint32_t (*accel_irq_init_fn)(ACCEL_ID, SDM_EXT_INTERRUPT_NUMBER, uint32_t, E_ACCEL_INTR_INIT_CONFIG);   // Interrupt specific init code.
    void (*accel_irq_fn)(void *);  // Interrupt specific ISR handler code
} accel_intr_irq_data_t, *paccel_intr_irq_data_t;

// Structure to store data that is passed with timer
typedef struct
{
    ACCEL_ID    accel_type;                 // Accel type (SDM /CDED)
    uint32_t    ext_cfg_addr;               // sdm_ext_cfg offset after ATU Map
    uint32_t    retry_count;                // Retry count for crash dump collection
    bool        skip_cper;                  // Skip CPER collection, used when CPER collection fails or tcm ue/lockup
    bool        skip_cd;                    // Skip CPER collection, used when CPER collection fails or tcm ue/lockup
    bool        has_crashed;                // To indicate if this accel has crashed
    bool        cper_collected;             // To indicate if cper has been collected
} accel_intr_crash_dump_collection_timer_data_t, *paccel_intr_crash_dump_collection_timer_data_t;

/*------------------- Declarations (Statics and globals) --------------------*/
/**
 * This array will store following data per interrupt bit
 * accel_irq_bit : Corresponding bit number from sdm_ext_cfg_regs.misc.sys_ext_intr2
 * accel_irq_init_fn : Init function for this bit of interrupt
 * accel_irq_fn : Validate the IRQ in SDM IP interrupt tree or process if specified by parameter
 */
extern const accel_intr_irq_data_t accel_irq_scp_data[ACCEL_SCP_INTR_MAX];

/*--------------------------- Function Prototypes ---------------------------*/

/*************************************************************************
 * Misc Functions
 *************************************************************************/

/**
 * @brief Clear and disable interrupt in given interrupt tree
 *
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 * @param[in] category_id : Interrupt category
 *          (this determines mask and interrupt addresses in level 1 and 2)
 * @param[in] interrupt_mask : Bits from the interrupt registers to clear and enable
 * @param[in] parent_vector : Parent vector to make sure interrupt is also cleared at parent level
 *
 * @retval void
 *
 */
void accel_intr_clear_disable_irq_in_sdm_intr_tree(uint32_t ext_cfg_addr, SDM_EXT_INT_CATEGORY category_id, uint32_t interrupt_mask, sdm_ext_int_t parent_vector);

/**
 * @brief Clear and enable interrupt in given interrupt tree,
 * parent vector is not needed since silibs take cares of enabling it at both levels
 *
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 * @param[in] category_id : Interrupt category
 *          (this determines mask and interrupt addresses in level 1 and 2)
 * @param[in] interrupt_mask : Bits from the interrupt registers to clear and enable
 *
 * @retval void
 *
 */
void accel_intr_enable_irq_in_sdm_intr_tree(uint32_t ext_cfg_addr, SDM_EXT_INT_CATEGORY category_id, uint32_t interrupt_mask);

/**
 * @brief Get value of interrupt status register
 *
 * \b Description:
 * Based on the input, read interrupt statius register
 *
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 * @param[in] category    : Category from sdm_ext_interrupts
 * @param[in] start_bit   : Start register bit
 * @param[in] end_bit     : End register bit
 *
 * @retval
 *  On success, Returns the value of register right shifted to align start_bit value to 0 position
 *  On failure, 0x0
 */
uint32_t accel_intr_get_interrupt_status_data(uint32_t ext_cfg_addr, uint32_t category, uint32_t start_bit, uint32_t end_bit);

/**
 * @brief Clear interrupt in level 1 register i.e. sdm_ext_cfg_regs.misc.sys_ext_intr2
 *
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 * @param[in] bit_index   : Bit in sdm_ext_cfg_regs.misc.sys_ext_intr2
 *
 * @retval void
 *
 */
void accel_intr_clear_interrupt_level_1(uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index);

/**
 * @brief Mask interrupt in level 1 register i.e. sdm_ext_cfg_regs.misc.sys_ext_intr2
 *
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 * @param[in] bit_index   : Bit in sdm_ext_cfg_regs.misc.sys_ext_intr2
 *
 * @retval void
 *
 */
void accel_intr_mask_interrupt_level_1(uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index);

/**
 * @brief Unmask interrupt in level 1 register i.e. sdm_ext_cfg_regs.misc.sys_ext_intr2
 *
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 * @param[in] bit_index   : Bit in sdm_ext_cfg_regs.misc.sys_ext_intr2
 *
 * @retval void
 *
 */
void accel_intr_unmask_interrupt_level_1(uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index);

/*************************************************************************
 * IRQ Init functions
 *************************************************************************/

/*************************************************************************
 * IRQ Validation and Handler functions
 *************************************************************************/

/**
 * @brief This is the fatal ISR function for ACCEL IP interrupt on SCP core
 *
 * @param callback_param This is the virtual IRQ number (Combination of NVIC IRQ, domain and bit index)
 *
 * @retval void
 */
void accel_intr_fatal_isr(void* callback_param);

/**
 * @brief ISR for TCM_UE_ECC_ERR and EMCPU_LOCKUP_ERR interrupts
 *
 *
 * @param callback_param This is the virtual IRQ number (Combination of NVIC IRQ, domain and bit index)
 *
 * @retval void
 */
void accel_intr_tcm_ue_emcpu_lockcup_isr(void* p_callback_param);

/*************************************************************************
 * IRQ DFWK Handler & General Helper functions
 *************************************************************************/

/**
 * @brief Create timer used during crash dump collection handshake between Accel emCPU and SCP
 *
 * @param[in] accel_type : Accelerator type SDM / CDED
 *
 * @retval
 * ACCEL_INTR_RET_SUCCESS : On Success
 * ACCEL_INTR_RET_FAIL_TIMER_CREATE : On Failure
 *
 */
uint32_t accel_intr_crash_dump_collection_timer_init(ACCEL_ID accel_type);

/**
 * @brief This function sets skip_cper and skip_cd to true for the given accelerator
 *
 * @param accel_type SDM/CDED
 *
 * @retval void
 */
void accel_intr_set_cper_cd_skip(void *callback_param);

