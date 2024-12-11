//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr_irq_isr.c
 * This file contains ISRs for Accel IP interrupts and other validation functions
 *
 */

/*-------------------------------- Features ---------------------------------*/

/*-------------------------------- Includes ---------------------------------*/
#include "accel_intr.h"
#include "accel_intr_events.h"
#include "accel_intr_priv.h"

#include <DfwkDriver.h>     // for DfwkInterfaceInitialize, DfwkQueueInitia...
#include <DfwkHost.h>       // for DfwkDeviceInitialize
#include <FPFwInterrupts.h> // for FPFwCoreInterruptDisableVector,...
#include <FpFwAssert.h>     // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>      // for FPFW_UNUSED
#include <accel_intr_client.h>
#include <accel_intr_service.h>
#include <accelerator_ip.h> // for ACCELERATOR_CDEDSS, ACCELERATOR_SDMSS
#include <bitops.h>
#include <cortex_m7_atomics.h> // for cortex_m7_atomic_call_data_memory_barrier
#include <nvic.h>
#include <sdm_ext_cfg_regs.h>
#include <stdbool.h> // for false
#include <string.h>  // for memset

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/**
 * @brief Handle misc tasks for FATAL Interrupts
 *
 * @details
 * This function is called once we have verified a valid FATAL interrupt is received.
 * Actions taken are
 * 1. Disable IRQ
 * 2. Disable Watchdog timer
 * 3. Send ASYNC message for further interrupt processing
 *
 * @param[in] IRQnum : IRQ number on which this interrupt is received.
 *            Determines if this is from CDED ACCEL IP / Generic ACCEL IP
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 *
 * @retval void
 *
 */
static void accel_intr_fatal_isr(uint32_t IRQnum, uint32_t ext_cfg_addr)
{
    // FATAL interrupt is received. Need to disable IRQ
    // Disable IRQ in NVIC, always returns SUCCESS
    FPFwCoreInterruptDisableVector(IRQnum);

    // Disable SDM Watchdog timer to avoid receiving FATAL interrupt from watchdog expiry
    // When FATAL interrupt is received, chances of watchdog expiry are very high
    // Since handling of all FATAL interrupt is similar we would like to avoid receiving new interrupt until first has been handled.
    accel_intr_emcpu_wdt_control(ext_cfg_addr, ACCEL_INTR_DISABLE_ACCEL_EMCPU_WDT);

    // Send ASYNC message
    send_fatal_intr_async_request(IRQnum);
}

/**
 * @brief Handle misc tasks for Doorbell interrupt SDM_MSG0_INTR
 *
 * @details
 * This function is called to check if Doorbell interrupt is received and
 * take appropriate action
 * 1. Clear interrupt
 * 2. Send ASYNC message for further interrupt processing
 *
 * @param[in] IRQnum : IRQ number on which this interrupt is received.
 *            Determines if this is from CDED ACCEL IP / Generic ACCEL IP
 * @param[in] ext_cfg_addr : sdm_ext_cfg offset after ATU Map
 *
 * @retval void
 *
 */
static void accel_intr_sys_msg_isr(uint32_t IRQnum, uint32_t ext_cfg_addr)
{
    // Doorbell interrupt is received.
    bool irq_status = false;

    (void)is_sdm_ext_int_status_set(ext_cfg_addr, SDM_EXT_SYS_MSG0_INTR_VECTOR, &irq_status);

    if (irq_status == true)
    {
        FPFW_ET_LOG(AccelIntr, IRQnum, SDM_EXT_SDM_MSG0_INTR);

        // Clear interrupt at level 1 since we are done handling
        accel_intr_clear_interrupt_level_1(ext_cfg_addr, SDM_EXT_SDM_MSG0_INTR);

        /**
         * We enable SDM_MSG0_INTR at level 1 only when needed.
         * This will be done as part of crash dump collection request.
         * Hence disabling it here after interrupt has been handled.
         */
        sdm_ext_int_disable(ext_cfg_addr, SDM_EXT_SDM_MSG0_INTR);

        // Add memory barrier to make sure interrupt is disabled before ISR returns
        cortex_m7_atomic_call_data_memory_barrier();

        // Send ASYNC message that Doorbell interrupt is received
        send_sdm_msg_async_request(IRQnum);
    }
}

static void accel_intr_mbox_isr(uint32_t IRQnum, uint32_t ext_cfg_addr)
{
    // Mailbox interrupt is received.
    bool irq_status = false;

    (void)is_sdm_ext_int_status_set(ext_cfg_addr, SDM_EXT_MBX_I2E_INTR_VECTOR, &irq_status);

    if (irq_status == true)
    {
        FPFW_ET_LOG(AccelIntr, IRQnum, SDM_EXT_MBX_I2E_INTR);

        // Mask interrupt at level 1 to avoid re-trigger
        accel_intr_mask_interrupt_level_1(ext_cfg_addr, SDM_EXT_MBX_I2E_INTR);

        // Send ASYNC message that Mailbox interrupt is received
        send_mailbox_async_request(IRQnum);
    }
}

/*----------------------------- Global Functions ----------------------------*/

uint32_t accel_intr_emcpu_wdt_err_fn(uint32_t IRQnum, uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index, bool process_this_fatal_intr)
{
    uint32_t ret_val = 0x0;

    // Level 1 will be checked in caller ISR
    // Only check Level 2
    bool irq_level2_status = false;
    (void)is_sdm_ext_int_status_set(ext_cfg_addr, SDM_EXT_EMCPU_WDT_FATAL_INTR_VECTOR, &irq_level2_status);

    if (irq_level2_status == true)
    {
        if (process_this_fatal_intr == true)
        {
            FPFW_ET_LOG(AccelIntr, IRQnum, bit_index);

            // Set to reset Accel emCPU
            ret_val = ACCEL_INTR_SET_RESET_ACCEL_EMCPU(ret_val);

            // Mask interrupt at level 1 to avoid re-trigger
            accel_intr_mask_interrupt_level_1(ext_cfg_addr, bit_index);
        }

        ret_val = ACCEL_INTR_SET_INTERRUPT_VALID(ret_val);
    }
    else
    {
        // Clear interrupt at level 1 since this is a spurious interrupt
        accel_intr_clear_interrupt_level_1(ext_cfg_addr, bit_index);
    }

    return ret_val;
}

uint32_t accel_intr_single_level_irq_fn(uint32_t IRQnum, uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index, bool process_this_fatal_intr)
{
    uint32_t ret_val = 0x0;

    if (process_this_fatal_intr == true)
    {
        // Level 1 will be checked in caller ISR. Only log here
        FPFW_ET_LOG(AccelIntr, IRQnum, bit_index);

        // Set to reset SoC
        ret_val = ACCEL_INTR_SET_RESET_SOC(ret_val);

        // Mask interrupt at level 1 to avoid re-trigger
        accel_intr_mask_interrupt_level_1(ext_cfg_addr, bit_index);
    }

    ret_val = ACCEL_INTR_SET_INTERRUPT_VALID(ret_val);

    return ret_val;
}

uint32_t accel_intr_ue_ecc_err_fn(uint32_t IRQnum, uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index, bool process_this_fatal_intr)
{
    uint32_t ret_val = 0x0;

    // Level 1 will be checked in caller ISR
    // Only check Level 2

    // TEL_ECC
    bool tel_ecc_irq_level2_clear = false;
    uint32_t interrupt_mask = SL_GET_BIT_MASK_RANGE(SDM_EXT_BPE_SCRATCH_ERR_INTR, SDM_EXT_LSTRG_ERR_INTR);

    (void)is_sdm_ext_int_mask_status_clear(ext_cfg_addr, SDM_EXT_CATEGORY_ID_UN_CORREC_ECC, interrupt_mask, &tel_ecc_irq_level2_clear);

    if (tel_ecc_irq_level2_clear == false)
    {
        ret_val = ACCEL_INTR_SET_INTERRUPT_VALID(ret_val);

        if (process_this_fatal_intr == true)
        {
            uint32_t interrupt_status = accel_intr_get_interrupt_status_data(ext_cfg_addr,
                                                                             SDM_EXT_CATEGORY_ID_UN_CORREC_ECC,
                                                                             SDM_EXT_BPE_SCRATCH_ERR_INTR,
                                                                             SDM_EXT_LSTRG_ERR_INTR);

            FPFW_ET_LOG(AccelIntrWithCategoryLevel2Bits, IRQnum, bit_index, SDM_EXT_CATEGORY_ID_UN_CORREC_ECC, interrupt_status);

            // Set reset flags based on interrupt status, for interrupts other than ITCM, D0TCM and D1TCM, SOC reset is needed
            if (BITWISE_AND(interrupt_status, SL_GET_BIT_MASK_RANGE(SDM_EXT_ITCM_ERR_INTR, SDM_EXT_DTCM1_ERR_INTR)) != 0x0)
            {
                ret_val = ACCEL_INTR_SET_RESET_ACCEL_EMCPU(ret_val);
            }

            // Set reset flags based on interrupt status, for interrupts other than ITCM, D0TCM and D1TCM, SOC reset is needed
            if (BITWISE_AND(interrupt_status,
                            BITWISE_INVERT(SL_GET_BIT_MASK_RANGE(SDM_EXT_ITCM_ERR_INTR, SDM_EXT_DTCM1_ERR_INTR))) != 0x0)
            {
                ret_val = ACCEL_INTR_SET_RESET_SOC(ret_val);
            }
        }
        else
        {
            // When not processing we only need to find one valid interrupt
            return ret_val;
        }
    }

    // FAB
    bool fab_irq_level2_clear = false;
    interrupt_mask = SL_GET_BIT_MASK_RANGE(SDM_EXT_BP_INT_AXI_ECC_UNCORREC_ERR_INTR,
                                           SDM_EXT_EMCPUINT_INT_AXI_POISON_UNCORREC_ERR_INTR);

    (void)is_sdm_ext_int_mask_status_clear(ext_cfg_addr, SDM_EXT_CATEGORY_ID_FABRIC_ERROR, interrupt_mask, &fab_irq_level2_clear);

    if (fab_irq_level2_clear == false)
    {
        ret_val = ACCEL_INTR_SET_INTERRUPT_VALID(ret_val);

        if (process_this_fatal_intr == true)
        {
            FPFW_ET_LOG(AccelIntrWithCategoryLevel2Bits,
                        IRQnum,
                        bit_index,
                        SDM_EXT_CATEGORY_ID_FABRIC_ERROR,
                        accel_intr_get_interrupt_status_data(ext_cfg_addr,
                                                             SDM_EXT_CATEGORY_ID_FABRIC_ERROR,
                                                             SDM_EXT_BP_INT_AXI_ECC_UNCORREC_ERR_INTR,
                                                             SDM_EXT_EMCPUINT_INT_AXI_POISON_UNCORREC_ERR_INTR));

            // Set to reset SoC
            ret_val = ACCEL_INTR_SET_RESET_SOC(ret_val);
        }
        else
        {
            // When not processing we only need to find one valid interrupt
            return ret_val;
        }
    }

    // Spurious interrupt so clear at level 1
    if (fab_irq_level2_clear && tel_ecc_irq_level2_clear)
    {
        // Clear interrupt
        accel_intr_clear_interrupt_level_1(ext_cfg_addr, bit_index);
    }
    // We come here only when proccess_interrupt is true and valid interrupt is received.
    else
    {
        // Mask interrupt at level 1 to avoid re-trigger
        accel_intr_mask_interrupt_level_1(ext_cfg_addr, bit_index);
    }
    return ret_val;
}

uint32_t accel_intr_sdm_wdt_err_fn(uint32_t IRQnum, uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index, bool process_this_fatal_intr)
{
    uint32_t ret_val = 0x0;

    // Level 1 will be checked in caller ISR
    // Only check Level 2
    bool irq_level2_clear = false;
    uint32_t interrupt_mask = SL_GET_BIT_MASK_RANGE(SDM_EXT_SDM_WDT_BP_INTR, SDM_EXT_SDM_WDT_MSI_INTR);

    (void)is_sdm_ext_int_mask_status_clear(ext_cfg_addr, SDM_EXT_CATEGORY_ID_SDM_WATCHDOG, interrupt_mask, &irq_level2_clear);

    if (irq_level2_clear == false)
    {
        ret_val = ACCEL_INTR_SET_INTERRUPT_VALID(ret_val);

        if (process_this_fatal_intr == true)
        {
            FPFW_ET_LOG(AccelIntrWithLevel2Bits,
                        IRQnum,
                        bit_index,
                        accel_intr_get_interrupt_status_data(ext_cfg_addr,
                                                             SDM_EXT_CATEGORY_ID_SDM_WATCHDOG,
                                                             SDM_EXT_SDM_WDT_BP_INTR,
                                                             SDM_EXT_SDM_WDT_MSI_INTR));

            // Set to reset SoC
            ret_val = ACCEL_INTR_SET_RESET_SOC(ret_val);

            // Mask interrupt at level 1 to avoid re-trigger
            accel_intr_mask_interrupt_level_1(ext_cfg_addr, bit_index);
        }
    }
    else
    {
        // Clear interrupt at level 1 since this is a spurious interrupt
        accel_intr_clear_interrupt_level_1(ext_cfg_addr, bit_index);
    }

    return ret_val;
}

uint32_t accel_intr_fab_wdt_err_fn(uint32_t IRQnum, uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index, bool process_this_fatal_intr)
{
    uint32_t ret_val = 0x0;

    // Level 1 will be checked in caller ISR
    // Only check Level 2
    bool irq_level2_clear = false;
    uint32_t interrupt_mask = SL_GET_BIT_MASK_RANGE(SDM_EXT_FAB_BOOT_CONFIG_WDT, SDM_EXT_MISC_WDT);

    (void)is_sdm_ext_int_mask_status_clear(ext_cfg_addr, SDM_EXT_CATEGORY_ID_FABRIC_WATCHDOG, interrupt_mask, &irq_level2_clear);

    if (irq_level2_clear == false)
    {
        ret_val = ACCEL_INTR_SET_INTERRUPT_VALID(ret_val);

        if (process_this_fatal_intr == true)
        {
            FPFW_ET_LOG(AccelIntrWithLevel2Bits,
                        IRQnum,
                        bit_index,
                        accel_intr_get_interrupt_status_data(ext_cfg_addr,
                                                             SDM_EXT_CATEGORY_ID_FABRIC_WATCHDOG,
                                                             SDM_EXT_FAB_BOOT_CONFIG_WDT,
                                                             SDM_EXT_MISC_WDT));

            // Set to reset SoC
            ret_val = ACCEL_INTR_SET_RESET_SOC(ret_val);

            // Mask interrupt at level 1 to avoid re-trigger
            accel_intr_mask_interrupt_level_1(ext_cfg_addr, bit_index);
        }
    }
    else
    {
        // Clear interrupt at level 1 since this is a spurious interrupt
        accel_intr_clear_interrupt_level_1(ext_cfg_addr, bit_index);
    }
    return ret_val;
}

uint32_t accel_intr_process_fatal_interrupts(uint32_t IRQnum, uint32_t ext_cfg_addr, bool process_this_fatal_intr)
{
    // Read Level 1 status register address
    uint32_t interrupt_reg_addr = sdm_ext_get_category_status_reg_addr(ext_cfg_addr, SDM_EXT_CATEGORY_ID_EXT_INTR);
    FPFW_RUNTIME_ASSERT(interrupt_reg_addr != SDM_EXT_INVALID_INTERRUPT_INPUT);

    // Read Level 1 mask/control register values
    uint32_t interrupt_mask_addr = sdm_ext_get_category_mask_reg_addr(ext_cfg_addr, SDM_EXT_CATEGORY_ID_EXT_INTR);
    FPFW_RUNTIME_ASSERT(interrupt_mask_addr != SDM_EXT_INVALID_INTERRUPT_INPUT);

    // Mask interrupt value with mask bits
    uint32_t interrupt_reg_value =
        BITWISE_AND(MMIO_READ32(interrupt_reg_addr), BITWISE_INVERT(MMIO_READ32(interrupt_mask_addr)));

    // Check if we have received valid FATAL IRQ
    uint32_t validate_irq_status = 0x0;

    // When process_this_fatal_intr is false, Run this only till first FATAL interrupt is identified.
    // When process_this_fatal_intr is true, Run the complete loop to process all interrupts
    while (interrupt_reg_value != 0x0 &&
           (ACCEL_INTR_IS_INTERRUPT_VALID_SET(validate_irq_status) != true || process_this_fatal_intr == true))
    {
        // Get bit that is set
        uint32_t bit_index = GET_LOWEST_SET_BIT_INDEX(interrupt_reg_value);

        eACCEL_SCP_INTR accel_intr = accel_scp_get_intr_enum(bit_index);

        if (accel_intr < ACCEL_SCP_INTR_MAX && accel_irq_scp_data[accel_intr].accel_irq_fn != NULL)
        {
            validate_irq_status |=
                accel_irq_scp_data[accel_intr].accel_irq_fn(IRQnum, ext_cfg_addr, bit_index, process_this_fatal_intr);
        }

        // Clear interrupt_reg_value bit
        interrupt_reg_value = interrupt_reg_value ^ SL_GET_SINGLE_BIT_MASK(bit_index);
    }

    return validate_irq_status;
}

void accel_intr_isr_scp(void* callback_param)
{
    uint32_t IRQnum = (uint32_t)callback_param;

    // Based on ATU MAP get sdm_ext_cfg base address
    uint32_t ext_cfg_addr = accelerator_ip_get_atu_mapped_cfg_address(accel_intr_get_accel_type_from_irq_num(IRQnum));

    uint32_t validate_irq_status =
        accel_intr_process_fatal_interrupts(IRQnum, ext_cfg_addr, ACCEL_INTR_PROCESS_INTR_IN_TOP_HALF);

    // Check if SDM_MSG0_INTR i.e. doorbell interrupt is received.
    // Incase loop ends
    accel_intr_sys_msg_isr(IRQnum, ext_cfg_addr);

    accel_intr_mbox_isr(IRQnum, ext_cfg_addr);

    if (ACCEL_INTR_IS_INTERRUPT_VALID_SET(validate_irq_status))
    {
        accel_intr_fatal_isr(IRQnum, ext_cfg_addr);
    }

    /**
     * Interrupt from NVIC->ISPR is cleared in NVIC when ISR is called
     * Interrupt from NVIC->IABR will be cleared in NVIC when ISR exits.
     */
    /**
     * TODO: Task 1982366: [SCP] Accel IP Fatal Interrupt Cleanup Tasks / Comments
     * Get interrupts raised using single call and then use switch statements to handle the interrupt
     */
}

void accel_intr_isr_mcp(void* callback_param)
{
    uint32_t IRQnum = (uint32_t)callback_param;

    // Based on ATU MAP get sdm_ext_cfg base address
    uint32_t ext_cfg_addr = accelerator_ip_get_atu_mapped_cfg_address(accel_intr_get_accel_type_from_irq_num(IRQnum));

    accel_intr_mbox_isr(IRQnum, ext_cfg_addr);
}
