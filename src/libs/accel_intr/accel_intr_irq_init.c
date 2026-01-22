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
#include "accel_intr_events.h"
#include "accel_intr_priv.h"
#include "accel_intr_virt_irq.h"
#include "bug_check.h"

#include <DfwkDriver.h>     // for DfwkInterfaceInitialize, DfwkQueueInitia...
#include <DfwkHost.h>       // for DfwkDeviceInitialize
#include <FPFwInterrupts.h> // for FPFwCoreInterruptEnableVector,...
#include <FpFwUtils.h>      // for FPFW_UNUSED
#include <accel_intr_service.h>
#include <accelip_id.h> // for ACCEL_ID_CDED, ACCEL_ID_SDM
#include <atu_init.h>   // for atu_svc_accel_atu_addr
#include <bitops.h>
#include <cortex_m7_atomics.h> // for cortex_m7_atomic_call_data_memory_barrier
#include <idsw.h>
#include <idsw_kng.h>
#include <kng_atu_mappings.h> // for ATU_MAPPING_CDEDSS_BASE, ATU...
#include <nvic.h>
#include <sdm_ext_cfg_regs.h> // for _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_BPE_SB_TEL_ECC_CNTR_ADDRESS
#include <stdbool.h>          // for false
#include <string.h>           // for memset
#include <system_info.h>      // for system_info_is_warm_start

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

#define DISABLE_INTERRUPT (true)  // Disable level-1 and level-2 interrupts
#define ENABLE_INTERRUPT  (false) // Enable level-1 and level-2 interrupts

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

static uint32_t accel_nvic_int_num[NUM_VALID_ACCEL_ID] = {0, 0};

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

static uint32_t accel_intr_get_domain_num_from_accel_type(ACCEL_ID accel_type)
{

    switch (accel_type)
    {
    case ACCEL_ID_CDED:
        if (idsw_get_cpu_type() == CPU_SCP)
        {
            return SCP_CDED_SDM_DOMAIN;
        }
        else
        {
            return MCP_CDED_SDM_DOMAIN;
        }
    case ACCEL_ID_SDM:
        if (idsw_get_cpu_type() == CPU_SCP)
        {
            return SCP_SDM_DOMAIN;
        }
        else
        {
            return MCP_SDM_DOMAIN;
        }
    default:
        BUG_ASSERT_PARAM(false, accel_type, NUM_VALID_ACCEL_ID);
        // Added to prevent compilation errors in unit tests
        return accel_nvic_int_num[ACCEL_ID_SDM];
        ;
    }
}

/*----------------------------- Global Functions ----------------------------*/

// TODO ADO: 2617263 Make default type as error
void accel_intr_set_irq_num_for_accel(ACCEL_ID accel_type, uint32_t irq_num)
{
    switch (accel_type)
    {
    case ACCEL_ID_CDED:
        accel_nvic_int_num[ACCEL_ID_CDED] = irq_num;
        break;
    case ACCEL_ID_SDM:
    default:
        accel_nvic_int_num[ACCEL_ID_SDM] = irq_num;
        break;
    }
}

uint32_t accel_intr_get_irq_num_from_accel_type(ACCEL_ID accel_type)
{
    switch (accel_type)
    {
    case ACCEL_ID_CDED:
        return accel_nvic_int_num[ACCEL_ID_CDED];
    case ACCEL_ID_SDM:
        return accel_nvic_int_num[ACCEL_ID_SDM];
    default:
        BUG_ASSERT_PARAM(false, accel_type, NUM_VALID_ACCEL_ID);
        // Added to prevent compilation errors in unit tests
        return accel_nvic_int_num[ACCEL_ID_SDM];
    }
}

// TODO ADO: 2617263 Make default type as error
ACCEL_ID accel_intr_get_accel_type_from_irq_num(uint32_t IRQnum)
{
    switch (IRQnum)
    {
    case CDEDSS_IRQ_NUMBER:
        return ACCEL_ID_CDED;
    case SDMSS_IRQ_NUMBER:
    default:
        return ACCEL_ID_SDM;
    }
}

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

void accel_intr_unmask_interrupt_level_1(uint32_t ext_cfg_addr, SDM_EXT_INTERRUPT_NUMBER bit_index)
{
    uint32_t interrupt_mask = SL_GET_SINGLE_BIT_MASK(bit_index);

    sdm_ext_int_mask_enable(ext_cfg_addr, SDM_EXT_CATEGORY_ID_EXT_INTR, interrupt_mask);

    // Add memory barrier to make sure interrupt is disabled before ISR / Handler returns
    cortex_m7_atomic_call_data_memory_barrier();
}

int32_t accel_scp_intr_init(ACCEL_ID accel_type)
{
    if (accel_type >= NUM_VALID_ACCEL_ID)
    {
        FPFW_ET_LOG(AccelIntrIrqInitError, accel_type, 0, __LINE__);
        return ACCEL_INTR_RET_FAIL_INTR_INIT;
    }

    uint32_t ret = ACCEL_INTR_RET_SUCCESS;
    uint32_t ext_cfg_addr = atu_svc_accel_atu_addr(accel_type);

    /**
     * Create timers used in Crash dump collection handshake
     */
    ret = accel_intr_crash_dump_collection_timer_init(accel_type);

    if (ret != ACCEL_INTR_RET_SUCCESS)
    {
        FPFW_ET_LOG(AccelIntrIrqInitError, accel_type, ret, __LINE__);
        return ret;
    }

    /**
     * Set subsystem in sdm_ext_interrupts.
     * This is later used to select between misc.sys_ext_intr0/1/2
     */
    (void)set_ext_int_sub_system(SDM_EXT_SCP_SUBSYSTEM);

    // Mask all interrupts in Level 1 register first
    uint32_t interrupt_mask = SL_GET_BIT_MASK_RANGE(SDM_EXT_EMCPU_WDT_ERR_INTR, SDM_EXT_SDM_MSG3_INTR);
    sdm_ext_int_mask_disable(ext_cfg_addr, SDM_EXT_CATEGORY_ID_EXT_INTR, interrupt_mask);
    // Clear all level-1 interrupts as well
    sdm_ext_int_mask_status_clear(ext_cfg_addr, SDM_EXT_CATEGORY_ID_EXT_INTR, interrupt_mask);

    // Accel composite interrupt enabled to receive mailbox messages
    FPFwCoreInterruptEnableVector(accel_intr_get_irq_num_from_accel_type(accel_type));

    return ACCEL_INTR_RET_SUCCESS;
}

void accel_intr_scp_err_intr_enable(ACCEL_ID accel_type)
{
    uint32_t nvic_irq_num = accel_intr_get_irq_num_from_accel_type(accel_type);
    uint32_t domain_num = accel_intr_get_domain_num_from_accel_type(accel_type);
    uint32_t virt_irq_num = 0x0;

    /**
     * 1. Enable level-1 interrupt for each interrupt in eACCEL_SCP_INTR
     * 2. If an irq handler is present, register with virtual irq framework
     */

    for (eACCEL_SCP_INTR idx = ACCEL_SCP_INTR_EMCPU_WDT_ERR; idx < ACCEL_SCP_INTR_MAX; idx++)
    {
        virt_irq_num = GET_VIRTUAL_IRQ(nvic_irq_num, accel_irq_scp_data[idx].accel_irq_bit, domain_num);
        // Register the irq handler as a virtual irq handler
        FPFwCoreInterruptRegisterCallback(virt_irq_num, accel_irq_scp_data[idx].accel_irq_fn, (void*)virt_irq_num);
        // Enable using virtual irq framework
        FPFwCoreInterruptEnableVector(virt_irq_num);
    }
}

/* TODO 2228988: Create separate Lib for SCP & MCP specific functionalities */

int32_t accel_mcp_intr_init(ACCEL_ID accel_type)
{
    if (accel_type >= NUM_VALID_ACCEL_ID)
    {
        FPFW_ET_LOG(AccelIntrIrqInitError, accel_type, 0, __LINE__);
        return ACCEL_INTR_RET_FAIL_INTR_INIT;
    }

    /**
     * Set subsystem in sdm_ext_interrupts.
     * This is later used to select between misc.sys_ext_intr0/1/2
     */
    (void)set_ext_int_sub_system(SDM_EXT_MCP_SUBSYSTEM);

    // ATU MAP base address for given accel device
    uint32_t ext_cfg_addr = atu_svc_accel_atu_addr(accel_type);
    // Mask all interrupts in Level 1 register first
    sdm_ext_int_mask_disable(ext_cfg_addr,
                             SDM_EXT_CATEGORY_ID_EXT_INTR,
                             SL_GET_BIT_MASK_RANGE(SDM_EXT_EMCPU_WDT_ERR_INTR, SDM_EXT_SDM_MSG3_INTR));

    // Enabled interrupts in NVIC
    FPFwCoreInterruptEnableVector(accel_intr_get_irq_num_from_accel_type(accel_type));

    return ACCEL_INTR_RET_SUCCESS;
}
