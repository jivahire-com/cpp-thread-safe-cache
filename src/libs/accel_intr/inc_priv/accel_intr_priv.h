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
    ACCEL_SCP_INTR_EMCPU_WDT_ERR = 0x0,     // Maps to SDM_EXT_EMCPU_WDT_ERR_INTR
    ACCEL_SCP_INTR_CP_FATAL_ERR,            // Maps to SDM_EXT_CP_FATAL_ERR_INTR
    ACCEL_SCP_INTR_UE_ECC_ERR,              // Maps to SDM_EXT_UE_ECC_ERR_INTR
    ACCEL_SCP_INTR_CSR_PARITY_ERR,          // Maps to SDM_EXT_CSR_PARITY_ERR_INTR
    ACCEL_SCP_INTR_SDM_WDT_ERR,             // Maps to SDM_EXT_SDM_WDT_ERR_INTR
    ACCEL_SCP_INTR_FAB_WDT_ERR,             // Maps to SDM_EXT_FAB_WDT_ERR_INTR
    ACCEL_SCP_INTR_AXI_UNSUPP_INTR_STATUS,  // Maps to SDM_EXT_AXI_UNSUPP_INTR_STATUS_INTR
    ACCEL_SCP_INTR_STYRESE_REQ_ERR,         // Maps to SDM_EXT_STYRESE_REQ_ERR_INTR
    ACCEL_SCP_INTR_MBX_I2E,                 // Maps to SDM_EXT_MBX_I2E_INTR
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
 * ACCEL EmCPU WDT Enable / Disable
 */
#define ACCEL_INTR_DISABLE_ACCEL_EMCPU_WDT          (0x0)
#define ACCEL_INTR_ENABLE_ACCEL_EMCPU_WDT           (0x1)

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
    uint32_t (*accel_irq_init_fn)(ACCEL_ID, SDM_EXT_INTERRUPT_NUMBER, uint32_t);   // Interrupt specific init code.
    uint32_t (*accel_irq_fn)(uint32_t, uint32_t, SDM_EXT_INTERRUPT_NUMBER, bool);  // Interrupt specific ISR handler code
} accel_intr_irq_data_t, *paccel_intr_irq_data_t;

// Structure to store data that is passed with timer
typedef struct
{
    ACCEL_ID    accel_type;                 // Accel type (SDM /CDED)
    uint32_t    timeout_count;              // Timeout count. This is needed to take different action based on timeout
    bool        is_soc_reset;               // Indicate if this is SoC reset 
    bool        is_collecting_crashdump;    // Indicates if crash dump collection has started
} accel_intr_crash_dump_collection_timer_data_t, *paccel_intr_crash_dump_collection_timer_data_t;

/**
 * Meta data of CDED CP level 2 interrupts
 */
typedef struct
{
    uint8_t bit_index;
    char brief[CDED_CP_INT_TRACE_STR_LEN];
    const void *l3_irq_data;
    uint8_t l3_irq_data_count;
} cded_cp_l2_irq_data_t;

/**
 * Union that holds ISR of CDED CP level 3 interrupts.
 */
typedef union
{
    uint32_t ccmp_isr[CDED_REGS_CCMP_CFG_REGS_ARRAY_COUNT];
    uint32_t dcmp_isr[CDED_REGS_DCMP_CFG_REGS_ARRAY_COUNT];
    uint32_t aes_isr[1]; /* Only once array count for AES engines */
} cded_cp_l3_isr_data_t;

/*------------------- Declarations (Statics and globals) --------------------*/
/**
 * This array will store following data per interrupt bit
 * accel_irq_bit : Corresponding bit number from sdm_ext_cfg_regs.misc.sys_ext_intr2
 * accel_irq_init_fn : Init function for this bit of interrupt 
 * accel_irq_fn : Validate the IRQ in SDM IP interrupt tree or process if specified by parameter
 */
extern const accel_intr_irq_data_t accel_irq_scp_data[ACCEL_SCP_INTR_MAX];
extern const accel_intr_irq_data_t accel_irq_mcp_data[ACCEL_MCP_INTR_MAX];


extern const cded_cp_l2_irq_data_t cded_cp_fatal_intr[];

/*--------------------------- Function Prototypes ---------------------------*/
/**
 * @brief Helper function to get eACCEL_SCP_INTR enum 
 *
 * \b Description:
 * This function will have mapping of eACCEL_SCP_INTR to sdm_ext_cfg_regs.misc.sys_ext_intr2 bit
 * Based on input bit index eACCEL_SCP_INTR will be returned
 *
 * @param[in] irq_bit : sdm_ext_cfg_regs.misc.sys_ext_intr2 bit
 *
 * @retval
 *  On success, ACCEL_SCP_INTR_*
 *  On failure, ACCEL_SCP_INTR_MAX
 */
eACCEL_SCP_INTR accel_scp_get_intr_enum(SDM_EXT_INTERRUPT_NUMBER irq_bit);

/*************************************************************************
 * Misc Functions
 *************************************************************************/

/**
 * @brief Enable / Disable ACCEL emCPU Watchdog 
 *
 *
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 * @param[in] enable : 0x1 for enable and 0x0 for disable
 *
 * @retval 
 * void
 */
void accel_intr_emcpu_wdt_control(uint32_t ext_cfg_addr, uint8_t enable);

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

/**
 * @brief Initialize SDM/CDED interrupts for SCP core
 * 
 * @param[in] accel_type : Accel type (SDM /CDED)
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 */
void accel_intr_scp_init(ACCEL_ID accel_type, uint32_t ext_cfg_addr);

/*************************************************************************
 * IRQ Init functions
 *************************************************************************/

/**
 * @brief Initialize EMCPU_WDT_ERR Interrupt
 * 
 * \b Description:
 * Clear and unmask interrupt in SDM interrupt tree
 *
 * @param[in] accel_type : Accel type (SDM /CDED)
 * @param[in] bit_index  : Bit number in sdm_ext_cfg_regs.misc.sys_ext_intr2
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 *
 * @retval
 *  Always returns, ACCEL_RET_SUCCESS
 */
uint32_t accel_intr_emcpu_wdt_err_init(ACCEL_ID accel_type, SDM_EXT_INTERRUPT_NUMBER bit_index, uint32_t ext_cfg_addr);

/**
 * @brief Initialize UE_ECC_ERR Interrupt
 * 
 * \b Description:
 * Clear and unmask interrupt in SDM interrupt tree
 *
 * @param[in] accel_type : Accel type (SDM /CDED)
 * @param[in] bit_index  : Bit number in sdm_ext_cfg_regs.misc.sys_ext_intr2
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 *
 * @retval
 *  Always returns, ACCEL_RET_SUCCESS
 */
uint32_t accel_intr_ue_ecc_err_init(ACCEL_ID accel_type, SDM_EXT_INTERRUPT_NUMBER bit_index, uint32_t ext_cfg_addr);

/**
 * @brief Initialize all ACCEL interrupts that have only level 1 register
 * 
 * \b Description:
 * Clear and unmask interrupt in SDM interrupt tree
 *
 * @param[in] accel_type : Accel type (SDM /CDED)
 * @param[in] bit_index  : Bit number in sdm_ext_cfg_regs.misc.sys_ext_intr2
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 *
 * @retval
 *  Always returns, ACCEL_RET_SUCCESS
 */
uint32_t accel_intr_single_level_irq_init(ACCEL_ID accel_type, SDM_EXT_INTERRUPT_NUMBER bit_index, uint32_t ext_cfg_addr);

/**
 * @brief Initialize SDM_WDT_ERR Interrupt
 * 
 * \b Description:
 * Clear and unmask interrupt in SDM interrupt tree
 *
 * @param[in] accel_type : Accel type (SDM /CDED)
 * @param[in] bit_index  : Bit number in sdm_ext_cfg_regs.misc.sys_ext_intr2
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 *
 * @retval
 *  Always returns, ACCEL_RET_SUCCESS
 */
uint32_t accel_intr_sdm_wdt_err_init(ACCEL_ID accel_type, SDM_EXT_INTERRUPT_NUMBER bit_index, uint32_t ext_cfg_addr);

/**
 * @brief Initialize FAB_WDT_ERR Interrupt
 * 
 *
 * \b Description:
 * Clear and unmask interrupt in SDM interrupt tree
 *
 * @param[in] accel_type : Accel type (SDM /CDED)
 * @param[in] bit_index  : Bit number in sdm_ext_cfg_regs.misc.sys_ext_intr2
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 *
 * @retval
 *  Always returns, ACCEL_RET_SUCCESS
 */
uint32_t accel_intr_fab_wdt_err_init(ACCEL_ID accel_type, SDM_EXT_INTERRUPT_NUMBER bit_index, uint32_t ext_cfg_addr);

/**
 * @brief Initialize CDED_CP_FATAL_ERR Interrupt
 * 
 *
 * \b Description:
 * Clear and unmask interrupt in SDM interrupt tree only for CDEDSS
 *
 * @param[in] accel_type : Accel type (SDM /CDED)
 * @param[in] bit_index  : Bit number in sdm_ext_cfg_regs.misc.sys_ext_intr2
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 *
 * @retval
 *  Always returns, ACCEL_RET_SUCCESS
 */
uint32_t accel_intr_cded_cp_err_init(ACCEL_ID accel_type, SDM_EXT_INTERRUPT_NUMBER bit_index, uint32_t ext_cfg_addr);

/*************************************************************************
 * IRQ Validation and Handler functions
 *************************************************************************/

/**
 * @brief EMCPU_WDT_ERR Interrupt : Validate and process if requested
 *
 * \b Description:
 * Validate interrupt at all levels of ISR.
 * Level 1 is checked in caller so only level 2 is checked in this function
 * Process the interrupt if requested by caller
 *
 * @param[in] IRQnum       : IRQ number for SDM / CDED
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 * @param[in] bit_index    : Bit number in sdm_ext_cfg_regs.misc.sys_ext_intr2
 * @param[in] process_this_fatal_intr  : Process the IRQ if this flag is set
 *
 * @retval uint32_t : Sets bits as follows
 * Bit 1 : Valid interrupt detected
 * Bit 2 : SoC reset is needed
 * Bit 3 : ACCEL emCPU reset is needed
 * 
 */
uint32_t accel_intr_emcpu_wdt_err_fn(uint32_t IRQnum, uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index, bool process_this_fatal_intr);

/**
 * @brief Interrupts that have only level 1 : Validate and process if requested
 *
 * \b Description:
 * Level 1 is checked in caller so no validation is needed.
 * Process the interrupt if requested by caller
 *
 * @param[in] IRQnum       : IRQ number for SDM / CDED
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 * @param[in] bit_index    : Bit number in sdm_ext_cfg_regs.misc.sys_ext_intr2
 * @param[in] process_this_fatal_intr  : Process the IRQ if this flag is set
 *
 * @retval uint32_t : Sets bits as follows
 * Bit 1 : Valid interrupt detected
 * Bit 2 : SoC reset is needed
 * Bit 3 : ACCEL emCPU reset is needed
 * 
 */
uint32_t accel_intr_single_level_irq_fn(uint32_t IRQnum, uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index, bool process_this_fatal_intr);

/**
 * @brief UE_ECC_ERR Interrupt : Validate and process if requested
 *
 * \b Description:
 * Validate interrupt at all levels of ISR.
 * Level 1 is checked in caller so only level 2 is checked in this function
 * Process the interrupt if requested by caller
 *
 * @param[in] IRQnum       : IRQ number for SDM / CDED
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 * @param[in] bit_index    : Bit number in sdm_ext_cfg_regs.misc.sys_ext_intr2
 * @param[in] process_this_fatal_intr  : Process the IRQ if this flag is set
 *
 * @retval uint32_t : Sets bits as follows
 * Bit 1 : Valid interrupt detected
 * Bit 2 : SoC reset is needed
 * Bit 3 : ACCEL emCPU reset is needed
 * 
 */
uint32_t accel_intr_ue_ecc_err_fn(uint32_t IRQnum, uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index, bool process_this_fatal_intr);

/**
 * @brief SDM_WDT_ERR Interrupt : Validate and process if requested
 *
 * \b Description:
 * Validate interrupt at all levels of ISR.
 * Level 1 is checked in caller so only level 2 is checked in this function
 * Process the interrupt if requested by caller
 *
 * @param[in] IRQnum       : IRQ number for SDM / CDED
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 * @param[in] bit_index    : Bit number in sdm_ext_cfg_regs.misc.sys_ext_intr2
 * @param[in] process_this_fatal_intr  : Process the IRQ if this flag is set
 *
 * @retval uint32_t : Sets bits as follows
 * Bit 1 : Valid interrupt detected
 * Bit 2 : SoC reset is needed
 * Bit 3 : ACCEL emCPU reset is needed
 * 
 */
uint32_t accel_intr_sdm_wdt_err_fn(uint32_t IRQnum, uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index, bool process_this_fatal_intr);

/**
 * @brief FAB_WDT_ERR Interrupt : Validate and process if requested
 *
 * \b Description:
 * Validate interrupt at all levels of ISR.
 * Level 1 is checked in caller so only level 2 is checked in this function
 * Process the interrupt if requested by caller
 *
 * @param[in] IRQnum       : IRQ number for SDM / CDED
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 * @param[in] bit_index    : Bit number in sdm_ext_cfg_regs.misc.sys_ext_intr2
 * @param[in] process_this_fatal_intr  : Process the IRQ if this flag is set
 *
 * @retval uint32_t : Sets bits as follows
 * Bit 1 : Valid interrupt detected
 * Bit 2 : SoC reset is needed
 * Bit 3 : ACCEL emCPU reset is needed
 * 
 */
uint32_t accel_intr_fab_wdt_err_fn(uint32_t IRQnum, uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index, bool process_this_fatal_intr);

/**
 * @brief ACCEL_SCP_INTR_CP_FATAL_ERR Interrupt : Validate and process if requested
 *
 * \b Description:
 * Validate interrupt at all levels of ISR.
 * Level 1 is checked in caller so only level 2 & 3 is checked in this function
 * Process the interrupt if requested by caller
 *
 * @param[in] IRQnum       : IRQ number for CDED
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 * @param[in] bit_index    : Bit number in sdm_ext_cfg_regs.misc.sys_ext_intr2
 * @param[in] process_this_fatal_intr  : Process the IRQ if this flag is set
 *
 * @retval uint32_t : Sets bits as follows
 * Bit 1 : Valid interrupt detected
 * Bit 2 : SoC reset is needed
 * Bit 3 : ACCEL emCPU reset is needed
 *
 */
uint32_t accel_intr_cded_cp_err_fn(uint32_t IRQnum, uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index, bool process_this_fatal_intr);

/**
 * @brief Loop through all FATAL interrupts.
 * When called from ISR : This will only validate the interrupts
 * When called from handler thread : The interrupts will also be processed
 * 
 * @param[in] IRQnum : IRQnum on which interrupt is received
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 * @param[in] process_this_fatal_intr : Boolean value indicating whether to process or only validate interrupt
 *
 * @retval uint32_t : Sets bits as follows
 * Bit 1 : Valid interrupt detected
 * Bit 2 : SoC reset is needed
 * Bit 3 : ACCEL emCPU reset is needed
 *
 */
uint32_t accel_intr_process_fatal_interrupts(uint32_t IRQnum, uint32_t ext_cfg_addr, bool process_this_fatal_intr);

/**
 * @brief ISR function for ACCEL IP interrupt on SCP core
 * 
 * @param[in] callback_param : This will have IRQNum to identify if interrupt is received for CDED / SDM
 *
 * @retval void
 *
 */
void accel_intr_isr_scp(void* callback_param);

/**
 * @brief ISR function for ACCEL IP interrupt on MCP core
 *
 * @param[in] callback_param : This will have IRQNum to identify if interrupt is received for CDED / SDM
 *
 * @retval void
 *
 */
void accel_intr_isr_mcp(void* callback_param);

/*************************************************************************
 * IRQ DFWK Handler functions
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
