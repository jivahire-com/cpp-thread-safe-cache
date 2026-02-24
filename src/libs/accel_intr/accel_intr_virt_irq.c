//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr_virt_irq.c
 * This file implements the platform APIs for the accelerator modules to
 * allow for virtualization of the accelerator interrupt lines using the virtual
 * interrupt framework
 *
 */

/*-------------------------------- Features ---------------------------------*/

/*-------------------------------- Includes ---------------------------------*/

#include <FPFwInterrupts.h>
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <accel_intr.h>
#include <accel_intr_events.h>
#include <accel_intr_virt_irq.h>
#include <accelerator_ip.h>
#include <atu_init.h>
#include <bitops.h>
#include <bug_check.h>
#include <cortex_m7_atomics.h> // for cortex_m7_atomic_call_data_memory_barrier
#include <idsw.h>
#include <idsw_kng.h>
#include <sdm_ext_cfg_regs.h>
#include <stddef.h>
#include <stdint.h>
#include <virt_irq.h>

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

#define SIZE_OF_UINT32_T_IN_BITS 32

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

static nvic_status_t accel_intr_virt_irq_register(uint32_t virt_irq_num, isr_callback_fn_with_params_t isr, void* p_parameter);
static nvic_status_t accel_intr_virt_irq_enable(uint32_t virt_irq_num);
static nvic_status_t accel_intr_virt_irq_disable(uint32_t virt_irq_num);
static nvic_status_t accel_intr_virt_irq_is_enabled(uint32_t virt_irq_num, bool* p_enabled);

/*------------------- Declarations (Statics and globals) --------------------*/

static virt_irq_callback_t accel_intr_vect_table[NUM_VALID_ACCEL_ID][SDM_EXT_SDM_MAX_CNT];
static virt_irq_plat_cb_t accel_virt_irq_plat_info[NUM_VALID_ACCEL_ID] = {
    {.platform_register_virtual_irq = accel_intr_virt_irq_register,
     .platform_enable_virtual_irq = accel_intr_virt_irq_enable,
     .platform_disable_virtual_irq = accel_intr_virt_irq_disable,
     .platform_is_irq_enabled = accel_intr_virt_irq_is_enabled},
    {.platform_register_virtual_irq = accel_intr_virt_irq_register,
     .platform_enable_virtual_irq = accel_intr_virt_irq_enable,
     .platform_disable_virtual_irq = accel_intr_virt_irq_disable,
     .platform_is_irq_enabled = accel_intr_virt_irq_is_enabled},
};
static uint32_t s_accel_irq_th_val[NUM_VALID_ACCEL_ID] = {0};
static uint32_t s_accel_irq_mask_th_val[NUM_VALID_ACCEL_ID] = {0};

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/**
 * @brief Fetch the ATU address for accelerator space based on NVIC IRQ Number
 *
 * @param nvic_irq_num NVIC IRQ of the accelerator
 * @return uint32_t ATU mapped address of the accelerator
 */
static uint32_t accel_intr_virt_irq_get_ext_cfg_addr(uint32_t nvic_irq_num)
{
    ACCEL_ID accel = accel_intr_get_accel_type_from_irq_num(nvic_irq_num);

    return atu_svc_accel_atu_addr(accel);
}

/**
 * @brief Wrapper ISR that is the entry point for interrupt from the accelarators.
 * This function will then parse the virtual interrupt table and call the corresponding
 * virtual interrupt handler
 *
 * @param irq_num This would be the NVIC IRQ number of the accelerator
 */
static void accel_intr_virt_irq_level1_wrapper_isr(void* irq_num)
{
    // irq_num will be the physical NVIC IRQ num
    uint32_t ext_cfg_addr = accel_intr_virt_irq_get_ext_cfg_addr((uint32_t)irq_num);

    // Read Level 1 status register address
    uint32_t interrupt_reg_addr = sdm_ext_get_category_status_reg_addr(ext_cfg_addr, SDM_EXT_CATEGORY_ID_EXT_INTR);
    BUG_ASSERT_PARAM(interrupt_reg_addr != SDM_EXT_INVALID_INTERRUPT_INPUT, interrupt_reg_addr, 0);

    // Read Level 1 mask/control register values
    uint32_t interrupt_mask_addr = sdm_ext_get_category_mask_reg_addr(ext_cfg_addr, SDM_EXT_CATEGORY_ID_EXT_INTR);
    BUG_ASSERT_PARAM(interrupt_mask_addr != SDM_EXT_INVALID_INTERRUPT_INPUT, interrupt_mask_addr, 0);

    uint32_t isr_val = (uint32_t)MMIO_READ32(interrupt_reg_addr);
    uint32_t mask_val = (uint32_t)MMIO_READ32(interrupt_mask_addr);

    // Mask interrupt value with mask bits
    uint32_t interrupt_reg_value = BITWISE_AND(isr_val, BITWISE_INVERT(MMIO_READ32(interrupt_mask_addr)));

    // CDED IRQ is 118 and SDM is 119, subtracting CDED IRQ enum from irq_num will give a value of 0/1 to index the array
    uint32_t virt_irq_table_index =
        (uint32_t)((uint32_t)irq_num - accel_intr_get_irq_num_from_accel_type(ACCEL_ID_CDED));

    ACCEL_ID accel_type = accel_intr_get_accel_type_from_irq_num((uint32_t)irq_num);

    s_accel_irq_th_val[accel_type] = isr_val;
    s_accel_irq_mask_th_val[accel_type] = mask_val;

    FPFW_ET_LOG(AccelIntr, isr_val, interrupt_reg_value);

    for (uint32_t i = 0; i < SIZE_OF_UINT32_T_IN_BITS; i++)
    {
        if (interrupt_reg_value & SET_BIT_MASK(i))
        {
            if (accel_intr_vect_table[virt_irq_table_index][i].irq_callback_fn != NULL)
            {
                accel_intr_vect_table[virt_irq_table_index][i].irq_callback_fn(
                    accel_intr_vect_table[virt_irq_table_index][i].callback_param);
            }
            else
            {
                // Mask interrupt off as there is no ISR registered
                sdm_ext_int_mask_disable(ext_cfg_addr, SDM_EXT_CATEGORY_ID_EXT_INTR, SET_BIT_MASK(i));
            }
            sdm_ext_int_mask_status_clear(ext_cfg_addr, SDM_EXT_CATEGORY_ID_EXT_INTR, SET_BIT_MASK(i));
        }
    }
}

/**
 * @brief This API stores the callback into the virtual interrupt table
 *
 * @param virt_irq_num Generated virtual interrupt number
 * @param isr Callback function for the corresponding virtual interrupt
 * @param p_parameter Void pointer to be passed to the virtual isr
 * @return NVIC_STATUS_SUCCESS/NVIC_STATUS_INVALID_PARAM
 */
static nvic_status_t accel_intr_virt_irq_register(uint32_t virt_irq_num, isr_callback_fn_with_params_t isr, void* p_parameter)
{
    // Obtain the irq offset from the virtual irq to index into the accel_intr_vect_table
    uint32_t irq_offset = GET_IRQ_OFFSET_FROM_VIRTUAL_IRQ(virt_irq_num);

    if (irq_offset >= SDM_EXT_SDM_MAX_CNT)
    {
        return NVIC_STATUS_INVALID_PARAM;
    }

    uint8_t irq_idx = GET_ARRAY_INDEX_FROM_IRQ(virt_irq_num);

    accel_intr_vect_table[irq_idx][irq_offset].irq_callback_fn = isr;
    accel_intr_vect_table[irq_idx][irq_offset].callback_param = p_parameter;

    return NVIC_STATUS_SUCCESS;
}

/**
 * @brief This function sets the corresponding bit at the accelerators level-1 interrupt register
 *
 * @param virt_irq_num Virtual interrupt number that internally contains the bit index to be set
 * @return NVIC_STATUS_SUCCESS/NVIC_STATUS_INVALID_PARAM
 */
static nvic_status_t accel_intr_virt_irq_enable(uint32_t virt_irq_num)
{
    uint32_t irq_offset = GET_IRQ_OFFSET_FROM_VIRTUAL_IRQ(virt_irq_num);
    uint32_t nvic_irq = GET_PHYSICAL_IRQ_FROM_VIRTUAL_IRQ(virt_irq_num);
    uint32_t ext_cfg_addr = accel_intr_virt_irq_get_ext_cfg_addr(nvic_irq);

    if (irq_offset >= SDM_EXT_SDM_MAX_CNT)
    {
        return NVIC_STATUS_INVALID_PARAM;
    }

    // Set the bit in the interrupt_mask and write it back
    sdm_ext_int_mask_enable(ext_cfg_addr, SDM_EXT_CATEGORY_ID_EXT_INTR, SET_BIT_MASK(irq_offset));

    return NVIC_STATUS_SUCCESS;
}

/**
 * @brief This function Clears the corresponding bit at the accelerators level-1 interrupt register
 *
 * @param virt_irq_num Virtual interrupt number that internally contains the bit index to be cleared
 * @return NVIC_STATUS_SUCCESS/NVIC_STATUS_INVALID_PARAM
 */
static nvic_status_t accel_intr_virt_irq_disable(uint32_t virt_irq_num)
{
    uint32_t irq_offset = GET_IRQ_OFFSET_FROM_VIRTUAL_IRQ(virt_irq_num);
    uint32_t nvic_irq = GET_PHYSICAL_IRQ_FROM_VIRTUAL_IRQ(virt_irq_num);
    uint32_t ext_cfg_addr = accel_intr_virt_irq_get_ext_cfg_addr(nvic_irq);

    if (irq_offset >= SDM_EXT_SDM_MAX_CNT)
    {
        return NVIC_STATUS_INVALID_PARAM;
    }

    sdm_ext_int_mask_disable(ext_cfg_addr, SDM_EXT_CATEGORY_ID_EXT_INTR, SET_BIT_MASK(irq_offset));

    return NVIC_STATUS_SUCCESS;
}

/**
 * @brief This function checks to see if the corresponding bit is set in the leve-1
 * interrupt mask register
 *
 * @param virt_irq_num Virutal interrupt number containing the bit index in the register
 * @param p_enabled Pointer to store whether the bit is set or not
 * @return NVIC_STATUS_SUCCESS/NVIC_STATUS_INVALID_PARAM
 */
static nvic_status_t accel_intr_virt_irq_is_enabled(uint32_t virt_irq_num, bool* p_enabled)
{
    uint32_t irq_offset = GET_IRQ_OFFSET_FROM_VIRTUAL_IRQ(virt_irq_num);
    uint32_t nvic_irq = GET_PHYSICAL_IRQ_FROM_VIRTUAL_IRQ(virt_irq_num);
    uint32_t ext_cfg_addr = accel_intr_virt_irq_get_ext_cfg_addr(nvic_irq);

    // Read Level 1 mask/control register values
    uint32_t interrupt_mask_addr = sdm_ext_get_category_mask_reg_addr(ext_cfg_addr, SDM_EXT_CATEGORY_ID_EXT_INTR);
    if (interrupt_mask_addr == SDM_EXT_INVALID_INTERRUPT_INPUT)
    {
        return NVIC_STATUS_INVALID_PARAM;
    }

    // NOTE: Do a bitwise AND of the irq offset with the interrupt mask register to get the mask value for that interrupt bit
    // NOTE: The mask register logic is inverted i.e. if a bit is cleared in the register the interrupt is enabled and if not it is disabled
    // NOTE: We then shift the result by irq_offset to get the mask value to bit-0 and invert it to convert mask value into irq enable value
    *p_enabled = !((((uint32_t)(*((uint32_t*)interrupt_mask_addr)) & SET_BIT_MASK(irq_offset)) >> irq_offset) & FPFW_BIT_0);

    return NVIC_STATUS_SUCCESS;
}

/*----------------------------- Global Functions ----------------------------*/

uint32_t accel_intr_get_virt_irq_plat_info(virt_irq_plat_cb_t** p_plat_info)
{
    BUG_ASSERT(p_plat_info != NULL);
    *p_plat_info = accel_virt_irq_plat_info;

    return NUM_VALID_ACCEL_ID;
}

void accel_intr_virt_irq_register_isr(uint32_t sdm_nvic_int, uint32_t cded_nvic_int)
{
    if (!accel_is_isolation_enabled(ACCEL_ID_SDM))
    {
        FPFwCoreInterruptRegisterCallback(sdm_nvic_int, accel_intr_virt_irq_level1_wrapper_isr, (void*)sdm_nvic_int);
    }
    if (!accel_is_isolation_enabled(ACCEL_ID_CDED))
    {
        FPFwCoreInterruptRegisterCallback(cded_nvic_int, accel_intr_virt_irq_level1_wrapper_isr, (void*)cded_nvic_int);
    }
}

void accel_intr_virt_irq_cd_reg(ACCEL_ID accel_type, uint32_t* irq_th_val_addr, uint32_t* irq_mask_th_val_addr)
{
    *irq_th_val_addr = (uint32_t)&s_accel_irq_th_val[accel_type];
    *irq_mask_th_val_addr = (uint32_t)&s_accel_irq_mask_th_val[accel_type];
}

#ifdef _WIN32
void accel_intr_get_virt_isr_cb(isr_callback_fn_with_params_t* p_cb_fun)
{
    *p_cb_fun = accel_intr_virt_irq_level1_wrapper_isr;
    FPFW_UNUSED(p_cb_fun);
}
#endif