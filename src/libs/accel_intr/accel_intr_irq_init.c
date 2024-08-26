//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr_init.c
 * This file provides interface to initializes Accelerator IP(s) interrupts
 * that are handled in SCP and provides misc functions
 *
 */

/*-------------------------------- Features ---------------------------------*/

/*-------------------------------- Includes ---------------------------------*/
#include "accel_intr.h"
#include "accel_intr_priv.h"

#include <DfwkDriver.h>     // for DfwkInterfaceInitialize, DfwkQueueInitia...
#include <DfwkHost.h>       // for DfwkDeviceInitialize
#include <FPFwInterrupts.h> // for FPFwCoreInterruptEnableVector,...
#include <FpFwUtils.h>      // for FPFW_UNUSED
#include <accel_intr_service.h>
#include <accelerator_ip.h> // for ACCELERATOR_CDEDSS, ACCELERATOR_SDMSS
#include <bitops.h>
#include <cortex_m7_atomics.h> // for cortex_m7_atomic_call_data_memory_barrier
#include <kng_atu_mappings.h>  // for ATU_MAPPING_CDEDSS_BASE, ATU...
#include <nvic.h>
#include <sdm_ext_cfg_regs.h> // for _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_BPE_SB_TEL_ECC_CNTR_ADDRESS
#include <stdbool.h>          // for false
#include <string.h>           // for memset

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

// Each bit represents one address set
// bit 0 : 0x164 bit 1 : 0x170 bit 2 : 0x17C
#define ACCEL_INTR_ECC_ERR_GET_CNTR_REG_FROM_ERR_TYPE(err_type) \
    (_ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_BPE_SB_TEL_ECC_CNTR_ADDRESS + ((err_type << 2) + (err_type << 3)))

/**
 * Reset values for these counters are max_value - 1
 * This will make sure interrupt is triggered for all UE errors
 */
#define ACCEL_INTR_UE_CNTR_RESET_VALUE                                  \
    (_ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_BPE_SB_TEL_ECC_CNTR_UE_MASK - \
     (0x1 << _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_BPE_SB_TEL_ECC_CNTR_UE_LSB))
#define ACCEL_CNTR_UE_CNTR_RESET_VALUE_MASK (_ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_BPE_SB_TEL_ECC_CNTR_UE_MASK)

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/**
 * @brief Clear and enable interrupt in NVIC
 *
 * @param[in] p_accelip_intr_cntxt : Accel IP context to choose between SDMCC / CDEDSS IRQ number
 *
 * @retval uint32_t
 * On Success : ACCEL_INTR_RET_SUCCESS
 * On Failure : ACCEL_INTR_RET_FAIL_INTR_NVIC
 *
 */
static uint32_t accel_intr_nvic_init(eACCELERATOR_TYPE accel_type)
{
    uint32_t IRQnum = GET_IRQ_NUMBER_FROM_ACCEL_TYPE(accel_type);

    // Register ISR
    nvic_status_t status =
        FPFwCoreInterruptRegisterCallback(IRQnum, (isr_callback_fn_with_params_t)accel_intr_isr, (void*)IRQnum);

    if (status != NVIC_STATUS_SUCCESS)
    {
        critical_print("AccelIp Interrupt Init : NVIC Set ISR failed\n");
        return ACCEL_INTR_RET_FAIL_INTR_NVIC;
    }

    // Clear pending and Enable IRQ in NVIC, always returns SUCCESS
    FPFwCoreInterruptEnableVector(IRQnum);

    return ACCEL_INTR_RET_SUCCESS;
}

/*----------------------------- Global Functions ----------------------------*/

void accel_intr_emcpu_wdt_control(uint32_t ext_cfg_addr, uint8_t enable)
{
    _addressblock_0x100000_emcpu_cfg_wdt_ctrl emcpu_cfg_wdt_ctrl;
    emcpu_cfg_wdt_ctrl.as_uint32 = 0x0;
    emcpu_cfg_wdt_ctrl.en = enable;

    uint32_t wdt_ctrl_reg_addr =
        ACCEL_INTR_GET_DERIVED_ADDR(_ADDRESSBLOCK_0X100000_EMCPU_CFG_WDT_CTRL_ADDRESS, ext_cfg_addr);
    MMIO_WRITE32(wdt_ctrl_reg_addr, emcpu_cfg_wdt_ctrl.as_uint32);

    cortex_m7_atomic_call_data_memory_barrier();
}

void accel_intr_clear_disable_irq_in_sdm_intr_tree(uint32_t ext_cfg_addr,
                                                   SDM_EXT_INT_CATEGORY category_id,
                                                   uint32_t interrupt_mask,
                                                   sdm_ext_int_t parent_vector)
{
    // Disable
    sdm_ext_int_mask_disable(ext_cfg_addr, category_id, interrupt_mask);
    // Clear status
    sdm_ext_int_mask_status_clear(ext_cfg_addr, category_id, interrupt_mask);

    cortex_m7_atomic_call_data_memory_barrier();

    // If parent_vector is valid then clear and disable at parent level as well
    if (parent_vector != SDM_EXT_INVALID_INTERRUPT_INPUT)
    {
        sdm_ext_int_disable(ext_cfg_addr, parent_vector);

        sdm_ext_int_status_clear(ext_cfg_addr, parent_vector);
    }

    // Add memory barrier to make sure interrupt is cleared before it is enabled
    cortex_m7_atomic_call_data_memory_barrier();
}

void accel_intr_enable_irq_in_sdm_intr_tree(uint32_t ext_cfg_addr, SDM_EXT_INT_CATEGORY category_id, uint32_t interrupt_mask)
{
    // Enable
    sdm_ext_int_mask_enable(ext_cfg_addr, category_id, interrupt_mask);
}

uint32_t accel_intr_get_interrupt_status_data(uint32_t ext_cfg_addr, uint32_t category, uint32_t start_bit, uint32_t end_bit)
{
    uint32_t interrupt_reg_address = sdm_ext_get_category_status_reg_addr(ext_cfg_addr, category);

    uint32_t interrupt_value = 0x0;

    if (interrupt_reg_address != SDM_EXT_INVALID_INTERRUPT_INPUT)
    {
        interrupt_value = MMIO_READ32(interrupt_reg_address);
    }

    interrupt_value = (interrupt_value & SL_GET_BIT_MASK_RANGE(start_bit, end_bit)) >> start_bit;

    return interrupt_value;
}

void accel_intr_clear_interrupt_level_1(uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index)
{
    uint32_t interrupt_mask = SL_GET_SINGLE_BIT_MASK(bit_index);

    sdm_ext_int_mask_status_clear(ext_cfg_addr, SDM_EXT_CATEGORY_ID_EXT_INTR, interrupt_mask);

    // Add memory barrier to make sure interrupt is cleared before it is enabled again
    cortex_m7_atomic_call_data_memory_barrier();
}

void accel_intr_mask_interrupt_level_1(uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index)
{
    uint32_t interrupt_mask = SL_GET_SINGLE_BIT_MASK(bit_index);

    sdm_ext_int_mask_disable(ext_cfg_addr, SDM_EXT_CATEGORY_ID_EXT_INTR, interrupt_mask);

    // Add memory barrier to make sure interrupt is disabled before ISR / Handler returns
    cortex_m7_atomic_call_data_memory_barrier();
}

int accel_intr_irq_init(eACCELERATOR_TYPE accel_type)
{
    if (accel_type >= MAX_ACCELERATOR_TYPES)
    {
        critical_print("Accelerator type out of bounds : %d\n", accel_type);
        return ACCEL_INTR_RET_FAIL_INTR_INIT;
    }

    uint32_t ret = ACCEL_INTR_RET_SUCCESS;

    /**
     * Create timers used in Crash dump collection handshake
     */
    ret = accel_intr_crash_dump_collection_timer_init(accel_type);

    if (ret != ACCEL_INTR_RET_SUCCESS)
    {
        critical_print("AccelIp Interrupt Timer create failed\n");
        return ret;
    }

    /**
     * Set subsystem in sdm_ext_interrupts.
     * This is later used to select between misc.sys_ext_intr0/1/2
     */
    (void)set_ext_int_sub_system(SDM_EXT_SCP_SUBSYSTEM);

    // Mask all interrupts in Level 1 register first
    uint32_t interrupt_mask = SL_GET_BIT_MASK_RANGE(SDM_EXT_EMCPU_WDT_ERR_INTR, SDM_EXT_SDM_MSG3_INTR);
    sdm_ext_int_mask_disable(accelerator_ip_get_atu_mapped_cfg_address(accel_type), SDM_EXT_CATEGORY_ID_EXT_INTR, interrupt_mask);

    // For each individual interrupt, clear and unmask at level 1 and level 2
    for (eACCEL_INTR idx = ACCEL_INTR_EMCPU_WDT_ERR; idx < ACCEL_INTR_MAX; idx++)
    {
        if (accel_intr_irq_data[idx].accel_irq_init_fn != NULL)
        {
            // Call init function for that IRQ, always returns SUCCESS
            (void)accel_intr_irq_data[idx].accel_irq_init_fn(accelerator_ip_get_atu_mapped_cfg_address(accel_type),
                                                             accel_intr_irq_data[idx].accel_irq_bit);
        }
    }

    // Register ISR for handling the interrupt
    // Clear and Enable interrupt in NVIC
    ret = accel_intr_nvic_init(accel_type);

    if (ret != ACCEL_INTR_RET_SUCCESS)
    {
        critical_print("AccelIp Interrupt NVIC init failed\n");
        return ret;
    }

    return ACCEL_INTR_RET_SUCCESS;
}

uint32_t accel_intr_emcpu_wdt_err_init(uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index)
{
    FPFW_UNUSED(bit_index);

    uint32_t interrupt_mask = SL_GET_SINGLE_BIT_MASK(SDM_EXT_EMCPU_WDT_FATAL_INTR);

    accel_intr_clear_disable_irq_in_sdm_intr_tree(ext_cfg_addr, SDM_EXT_CATEGORY_ID_EMCPU_WATCHDOG, interrupt_mask, SDM_EXT_EMCPU_WDT_ERR_INTR_VECTOR);
    accel_intr_enable_irq_in_sdm_intr_tree(ext_cfg_addr, SDM_EXT_CATEGORY_ID_EMCPU_WATCHDOG, interrupt_mask);

    return ACCEL_INTR_RET_SUCCESS;
}

/**
 * TODO: Task 1982366: [SCP] Accel IP Fatal Interrupt Cleanup Tasks / Comments
 * Combine various interrupt inits in single silibs call
 */
uint32_t accel_intr_ue_ecc_err_init(uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index)
{
    FPFW_UNUSED(bit_index);

    // Clear and enable TEL_ECC errors
    uint32_t interrupt_mask = SL_GET_BIT_MASK_RANGE(SDM_EXT_BPE_SCRATCH_ERR_INTR, SDM_EXT_LSTRG_ERR_INTR);

    /**
     * TODO: Task 1982366: [SCP] Accel IP Fatal Interrupt Cleanup Tasks / Comments
     * Move reset counter logic to Silibs
     * @note: We set TEL_ECC error counters to MAX - 1.
     * This needs to be done to make sure every UE error results in an interrupt.
     * This is done when initialising interrupts for first time and
     * when re-initialising them on next consecutive cycles.
     */
    for (uint32_t err_type = SDM_EXT_BPE_SCRATCH_ERR_INTR; err_type <= SDM_EXT_LSTRG_ERR_INTR; err_type++)
    {
        uint32_t reg_addr =
            ACCEL_INTR_GET_DERIVED_ADDR(ACCEL_INTR_ECC_ERR_GET_CNTR_REG_FROM_ERR_TYPE(err_type), ext_cfg_addr);

        // Based on error no set the corresponding bits
        // For all error type same mapping is used for CE / UE error.
        MMIO_UPDATE32(reg_addr, ACCEL_INTR_UE_CNTR_RESET_VALUE, ACCEL_CNTR_UE_CNTR_RESET_VALUE_MASK);

        cortex_m7_atomic_call_data_memory_barrier();
    }

    // Clear and disable TEL_ECC errors and SDM_EXT_TCM_UE_ECC_ERR_INTR
    accel_intr_clear_disable_irq_in_sdm_intr_tree(ext_cfg_addr, SDM_EXT_CATEGORY_ID_UN_CORREC_ECC, interrupt_mask, SDM_EXT_TCM_UE_ECC_ERR_INTR);

    // Clear and disable FAB_ECC errors and SDM_EXT_UE_ECC_ERR_INTR_VECTOR
    accel_intr_clear_disable_irq_in_sdm_intr_tree(ext_cfg_addr,
                                                  SDM_EXT_CATEGORY_ID_FABRIC_ERROR,
                                                  FAB_UN_CORREC_ERR_BIT_MASK,
                                                  SDM_EXT_UE_ECC_ERR_INTR_VECTOR);

    /**
     * TODO: Task 1982366: [SCP] Accel IP Fatal Interrupt Cleanup Tasks / Comments
     * Combine FAB_ECC and TEL_ECC errors for UE errors under one category
     */
    // Enable the interrupts
    accel_intr_enable_irq_in_sdm_intr_tree(ext_cfg_addr, SDM_EXT_CATEGORY_ID_UN_CORREC_ECC, interrupt_mask);
    accel_intr_enable_irq_in_sdm_intr_tree(ext_cfg_addr, SDM_EXT_CATEGORY_ID_FABRIC_ERROR, FAB_UN_CORREC_ERR_BIT_MASK);

    /**
     * TODO: FW Task 779516: sdm_ext_interrupts and sdm_em_interrupts : Create separate category for TCM_UE_ECC_ERR
     * ITCM, DxTCM UE ECC errors are logged as UE_ECC_ERR and TCM_UE_ECC_ERR.
     * We handle all ECC errors as part of UE_ECC_ERR, but Silibs code is currently handling TCM_UE_ECC_ERR as part of UE_ECC_ERR.
     * When enabling UE_ECC for TCMs also enables TCM_UE_ECC. To avoid that, we need to explicitly disable TCM_UE_ECC_ERR
     */
    accel_intr_mask_interrupt_level_1(ext_cfg_addr, SDM_EXT_TCM_UE_ECC_ERR_INTR);

    return ACCEL_INTR_RET_SUCCESS;
}

uint32_t accel_intr_single_level_irq_init(uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index)
{
    uint32_t interrupt_mask = SL_GET_SINGLE_BIT_MASK(bit_index);

    accel_intr_clear_disable_irq_in_sdm_intr_tree(ext_cfg_addr, SDM_EXT_CATEGORY_ID_EXT_INTR, interrupt_mask, SDM_EXT_INVALID_INTERRUPT_INPUT);
    accel_intr_enable_irq_in_sdm_intr_tree(ext_cfg_addr, SDM_EXT_CATEGORY_ID_EXT_INTR, interrupt_mask);

    return ACCEL_INTR_RET_SUCCESS;
}

uint32_t accel_intr_sdm_wdt_err_init(uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index)
{
    FPFW_UNUSED(bit_index);

    uint32_t interrupt_mask = SL_GET_BIT_MASK_RANGE(SDM_EXT_SDM_WDT_BP_INTR, SDM_EXT_SDM_WDT_MSI_INTR);

    accel_intr_clear_disable_irq_in_sdm_intr_tree(ext_cfg_addr, SDM_EXT_CATEGORY_ID_SDM_WATCHDOG, interrupt_mask, SDM_EXT_SDM_WDT_ERR_INTR_VECTOR);
    accel_intr_enable_irq_in_sdm_intr_tree(ext_cfg_addr, SDM_EXT_CATEGORY_ID_SDM_WATCHDOG, interrupt_mask);

    return ACCEL_INTR_RET_SUCCESS;
}

uint32_t accel_intr_fab_wdt_err_init(uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index)
{
    FPFW_UNUSED(bit_index);

    uint32_t interrupt_mask = SL_GET_BIT_MASK_RANGE(SDM_EXT_FAB_BOOT_CONFIG_WDT, SDM_EXT_MISC_WDT);

    accel_intr_clear_disable_irq_in_sdm_intr_tree(ext_cfg_addr, SDM_EXT_CATEGORY_ID_FABRIC_WATCHDOG, interrupt_mask, SDM_EXT_FAB_WDT_ERR_INTR_VECTOR);
    accel_intr_enable_irq_in_sdm_intr_tree(ext_cfg_addr, SDM_EXT_CATEGORY_ID_FABRIC_WATCHDOG, interrupt_mask);

    return ACCEL_INTR_RET_SUCCESS;
}
