//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_accel_virt_irq.cpp
 * Unit test file for accel_intr lib
 */

/*-------------------------------- Includes ---------------------------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep

extern "C" {
#include "accel_intr_priv.h"
#include "accel_intr_virt_irq.h"
#include "accelip_id.h"
#include "virt_irq.h"

#include <FPFwInterrupts.h>
#include <FpFwUtils.h>
#include <assert.h>
#include <bitops.h>
#include <boot_status.h>
#include <fpfw_init.h> // for fpfw_init_component_id_t, fpfw_...
#include <idsw_kng.h>
#include <nvic.h>
#include <sdm_ext_interrupts.h>

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

// static virt_irq_plat_cb_t* global_plat_info;

/*--------------------------------- Externs ---------------------------------*/

extern fpfw_init_component_t _fpfw_component_virt_irq;

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/

/*****************************
 * MOCKS
 *****************************/

/**
 * @brief sdm_ext_int_mask_enable - Mock function
 *
 * @param[in] ext_cfg_addr : Ext CFG address offset
 * @param[in] category_id : Category of interrupt
 * @param[in] interrupt_mask : Interrupt Mask register indicate bits to enable
 *
 *  @return
 *      int
 */
int __wrap_sdm_ext_int_mask_enable(uintptr_t ext_cfg_addr, SDM_EXT_INT_CATEGORY category_id, uint32_t interrupt_mask)
{
    check_expected(ext_cfg_addr);
    check_expected(category_id);
    check_expected(interrupt_mask);

    return mock_type(int);
}

/**
 * @brief sdm_ext_int_mask_disable - Mock function
 *
 * @param[in] ext_cfg_addr : Ext CFG address offset
 * @param[in] category_id : Category of interrupt
 * @param[in] interrupt_mask : Interrupt Mask register  indicate bits to disable
 *
 *  @return
 *      int
 */
int __wrap_sdm_ext_int_mask_disable(uintptr_t ext_cfg_addr, SDM_EXT_INT_CATEGORY category_id, uint32_t interrupt_mask)
{
    check_expected(ext_cfg_addr);
    check_expected(category_id);
    check_expected(interrupt_mask);

    return mock_type(int);
}

/**
 * @brief sdm_ext_int_mask_status_clear - Mock function
 *
 * @param[in] ext_cfg_addr : Ext CFG address offset
 * @param[in] category_id : Category of interrupt
 * @param[in] interrupt_mask : Interrupt Mask register indicate bits to clear
 *
 *  @return
 *      int
 */
int __wrap_sdm_ext_int_mask_status_clear(uintptr_t ext_cfg_addr, SDM_EXT_INT_CATEGORY category_id, uint32_t interrupt_mask)
{
    check_expected(ext_cfg_addr);
    check_expected(category_id);
    check_expected(interrupt_mask);

    return mock_type(int);
}

/**
 * @brief is_sdm_ext_int_mask_status_clear - Mock function
 *
 * @param[in] ext_cfg_addr : Ext CFG address offset
 * @param[in] category_id : Category of interrupt
 *
 *  @return
 *      int
 */
uint32_t __wrap_sdm_ext_get_category_status_reg_addr(uintptr_t ext_cfg_addr, SDM_EXT_INT_CATEGORY category_id)
{
    check_expected(ext_cfg_addr);
    check_expected(category_id);

    return mock_type(uint32_t);
}

/**
 * @brief sdm_ext_get_category_mask_reg_addr - Mock function
 *
 * @param[in] ext_cfg_addr : Ext CFG address offset
 * @param[in] category_id : Category of interrupt
 *
 *  @return
 *      int
 */
uint32_t __wrap_sdm_ext_get_category_mask_reg_addr(uintptr_t ext_cfg_addr, SDM_EXT_INT_CATEGORY category_id)
{
    check_expected(ext_cfg_addr);
    check_expected(category_id);

    return mock_type(uint32_t);
}

/**
 * @brief accelerator_ip_get_atu_mapped_cfg_address - Mock function
 *
 * @param accel_type Accelerator type
 * @return uint32_t Mock value stored in cmocka stack
 */
uint32_t __wrap_accelerator_ip_get_atu_mapped_cfg_address(ACCEL_ID accel_type)
{
    FPFW_UNUSED(accel_type);
    return mock_type(uint32_t);
}

/**
 * @brief  mmio_read32 - Mock Function
 *
 * @param addr Address to read
 * @return Mock return value
 */
uint32_t __wrap_mmio_read32(volatile uint32_t* addr)
{
    FPFW_UNUSED(addr);
    return mock_type(uint32_t);
}

uint32_t __wrap_atu_svc_accel_atu_addr(ACCEL_ID accel_id)
{
    FPFW_UNUSED(accel_id);

    return mock_type(uint32_t);
}

bool __wrap_accel_is_isolation_enabled(ACCEL_ID accel_type)
{
    FPFW_UNUSED(accel_type);
    return mock_type(bool);
}

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

void __wrap_boot_status_notify_extd(boot_status_req_t* p_req_mem, uint32_t boot_status, uint32_t boot_status_ex)
{
    check_expected(boot_status);
    assert_non_null(p_req_mem);
    check_expected(boot_status_ex);

    function_called();
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}
}

static void sdm_virt_isr_fun(void* args)
{
    FPFW_UNUSED(args);
}

/**
 * @brief : Tests accel_scp_intr_init with all passing on SDM
 */
TEST_FUNCTION(test_accel_virt_irq_test, nullptr, nullptr)
{
    isr_callback_fn_with_params_t isr_fun;

    accel_intr_get_virt_isr_cb(&isr_fun);
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_SDM_NVIC_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, isr_fun);
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, (void*)HW_INT_SDM_NVIC_INT);

    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, HW_INT_CDED_SDM_NVIC_INT);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, isr_fun);
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, (void*)HW_INT_CDED_SDM_NVIC_INT);

    will_return_always(__wrap_nvic_irq_set_isr_with_param, NVIC_STATUS_ERROR);
    will_return_always(__wrap_accel_is_isolation_enabled, false);

    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);

    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_VIRT_IRQ_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    _fpfw_component_virt_irq.init_fn();

    // Set CDED and SDM IRQ numbers
    accel_intr_set_irq_num_for_accel(ACCEL_ID_SDM, HW_INT_SDM_NVIC_INT);
    accel_intr_set_irq_num_for_accel(ACCEL_ID_CDED, HW_INT_CDED_SDM_NVIC_INT);
}

TEST_FUNCTION(test_accel_virt_irq_register_success, nullptr, nullptr)
{
    assert(FPFwCoreInterruptRegisterCallback(GET_VIRTUAL_IRQ(HW_INT_SDM_NVIC_INT, SDM_EXT_EMCPU_WDT_ERR_INTR, SCP_SDM_DOMAIN),
                                             sdm_virt_isr_fun,
                                             (void*)SCP_SDM_DOMAIN) == (uint32_t)NVIC_STATUS_SUCCESS);
}

TEST_FUNCTION(test_accel_virt_irq_register_fail, nullptr, nullptr)
{
    assert(FPFwCoreInterruptRegisterCallback(GET_VIRTUAL_IRQ(HW_INT_SDM_NVIC_INT, SDM_EXT_SDM_MAX_CNT, SCP_SDM_DOMAIN),
                                             sdm_virt_isr_fun,
                                             (void*)SCP_SDM_DOMAIN) == (uint32_t)NVIC_STATUS_INVALID_PARAM);
}

TEST_FUNCTION(test_accel_virt_irq_enable_success, nullptr, nullptr)
{
    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x1234ABCD);

    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SDM_NVIC_INT);
    expect_value(__wrap_sdm_ext_int_mask_enable, ext_cfg_addr, 0x1234ABCD);
    expect_value(__wrap_sdm_ext_int_mask_enable, category_id, SDM_EXT_CATEGORY_ID_EXT_INTR);
    expect_value(__wrap_sdm_ext_int_mask_enable, interrupt_mask, SET_BIT_MASK(SDM_EXT_EMCPU_WDT_ERR_INTR));
    will_return(__wrap_sdm_ext_int_mask_enable, 0x0);
    // will_return(__wrap_accelerator_ip_get_atu_mapped_cfg_address, 0x1234ABCD);

    assert(FPFwCoreInterruptEnableVector(GET_VIRTUAL_IRQ(HW_INT_SDM_NVIC_INT, SDM_EXT_EMCPU_WDT_ERR_INTR, SCP_SDM_DOMAIN)) ==
           (uint32_t)NVIC_STATUS_SUCCESS);
}

TEST_FUNCTION(test_accel_virt_irq_enable_fail, nullptr, nullptr)
{
    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x1234ABCD);

    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SDM_NVIC_INT);
    // will_return(__wrap_accelerator_ip_get_atu_mapped_cfg_address, 0x1234ABCD);

    assert(FPFwCoreInterruptEnableVector(GET_VIRTUAL_IRQ(HW_INT_SDM_NVIC_INT, SDM_EXT_SDM_MAX_CNT, SCP_SDM_DOMAIN)) ==
           (uint32_t)NVIC_STATUS_INVALID_PARAM);
}

TEST_FUNCTION(test_accel_virt_irq_disable_success, nullptr, nullptr)
{
    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x1234ABCD);

    expect_value(__wrap_sdm_ext_int_mask_disable, ext_cfg_addr, 0x1234ABCD);
    expect_value(__wrap_sdm_ext_int_mask_disable, category_id, SDM_EXT_CATEGORY_ID_EXT_INTR);
    expect_value(__wrap_sdm_ext_int_mask_disable, interrupt_mask, SET_BIT_MASK(SDM_EXT_EMCPU_WDT_ERR_INTR));
    will_return(__wrap_sdm_ext_int_mask_disable, 0x0);
    // will_return(__wrap_accelerator_ip_get_atu_mapped_cfg_address, 0x1234ABCD);

    assert(FPFwCoreInterruptDisableVector(GET_VIRTUAL_IRQ(HW_INT_SDM_NVIC_INT, SDM_EXT_EMCPU_WDT_ERR_INTR, SCP_SDM_DOMAIN)) ==
           (uint32_t)NVIC_STATUS_SUCCESS);
}

TEST_FUNCTION(test_accel_virt_irq_disable_fail, nullptr, nullptr)
{
    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x1234ABCD);

    // will_return(__wrap_accelerator_ip_get_atu_mapped_cfg_address, 0x1234ABCD);

    assert(FPFwCoreInterruptDisableVector(GET_VIRTUAL_IRQ(HW_INT_SDM_NVIC_INT, SDM_EXT_SDM_MAX_CNT, SCP_SDM_DOMAIN)) ==
           (uint32_t)NVIC_STATUS_INVALID_PARAM);
}

TEST_FUNCTION(test_accel_virt_is_irq_enabled_not_enabled, nullptr, nullptr)
{
    uint32_t mask_val = SET_BIT_MASK(SDM_EXT_EMCPU_WDT_ERR_INTR);

    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x1234ABCD);

    expect_value(__wrap_sdm_ext_get_category_mask_reg_addr, ext_cfg_addr, 0x1234ABCD);
    expect_value(__wrap_sdm_ext_get_category_mask_reg_addr, category_id, SDM_EXT_CATEGORY_ID_EXT_INTR);
    will_return(__wrap_sdm_ext_get_category_mask_reg_addr, &mask_val);
    // will_return(__wrap_accelerator_ip_get_atu_mapped_cfg_address, 0x1234ABCD);

    assert(FPFwCoreInterruptIsEnabled(GET_VIRTUAL_IRQ(HW_INT_SDM_NVIC_INT, SDM_EXT_EMCPU_WDT_ERR_INTR, SCP_SDM_DOMAIN)) == false);
}

TEST_FUNCTION(test_accel_virt_is_irq_enabled_is_enabled, nullptr, nullptr)
{
    uint32_t mask_val = 0x0;

    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x1234ABCD);

    expect_value(__wrap_sdm_ext_get_category_mask_reg_addr, ext_cfg_addr, 0x1234ABCD);
    expect_value(__wrap_sdm_ext_get_category_mask_reg_addr, category_id, SDM_EXT_CATEGORY_ID_EXT_INTR);
    will_return(__wrap_sdm_ext_get_category_mask_reg_addr, &mask_val);
    // will_return(__wrap_accelerator_ip_get_atu_mapped_cfg_address, 0x1234ABCD);

    assert(FPFwCoreInterruptIsEnabled(GET_VIRTUAL_IRQ(HW_INT_SDM_NVIC_INT, SDM_EXT_EMCPU_WDT_ERR_INTR, SCP_SDM_DOMAIN)) == true);
}

TEST_FUNCTION(test_accel_virt_is_irq_enabled_fail, nullptr, nullptr)
{
    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x1234ABCD);

    expect_value(__wrap_sdm_ext_get_category_mask_reg_addr, ext_cfg_addr, 0x1234ABCD);
    expect_value(__wrap_sdm_ext_get_category_mask_reg_addr, category_id, SDM_EXT_CATEGORY_ID_EXT_INTR);
    will_return(__wrap_sdm_ext_get_category_mask_reg_addr, SDM_EXT_INVALID_INTERRUPT_INPUT);
    // will_return(__wrap_accelerator_ip_get_atu_mapped_cfg_address, 0x1234ABCD);

    assert(FPFwCoreInterruptIsEnabled(GET_VIRTUAL_IRQ(HW_INT_SDM_NVIC_INT, SDM_EXT_SDM_MAX_CNT, SCP_SDM_DOMAIN)) == false);
}

TEST_FUNCTION(test_accel_virt_level1_isr, nullptr, nullptr)
{
    uint32_t mask_val = 0x0;
    uint32_t reg_val = 0x1;
    isr_callback_fn_with_params_t cb_fun = NULL;

    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x1234ABCD);

    expect_value_count(__wrap_sdm_ext_get_category_status_reg_addr, ext_cfg_addr, 0x1234ABCD, 2);
    expect_value_count(__wrap_sdm_ext_get_category_status_reg_addr, category_id, SDM_EXT_CATEGORY_ID_EXT_INTR, 2);
    will_return_always(__wrap_sdm_ext_get_category_status_reg_addr, &reg_val);
    expect_value_count(__wrap_sdm_ext_get_category_mask_reg_addr, ext_cfg_addr, 0x1234ABCD, 2);
    expect_value_count(__wrap_sdm_ext_get_category_mask_reg_addr, category_id, SDM_EXT_CATEGORY_ID_EXT_INTR, 2);
    will_return_always(__wrap_sdm_ext_get_category_mask_reg_addr, &mask_val);
    expect_value_count(__wrap_sdm_ext_int_mask_status_clear, ext_cfg_addr, 0x1234ABCD, 2);
    expect_value_count(__wrap_sdm_ext_int_mask_status_clear, category_id, SDM_EXT_CATEGORY_ID_EXT_INTR, 2);
    expect_value(__wrap_sdm_ext_int_mask_status_clear, interrupt_mask, SET_BIT_MASK(SDM_EXT_EMCPU_WDT_ERR_INTR));
    expect_value(__wrap_sdm_ext_int_mask_status_clear, interrupt_mask, SET_BIT_MASK(SDM_EXT_AHBS_WABORT_ERR_INTR));
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, 0x0);
    expect_value(__wrap_sdm_ext_int_mask_disable, ext_cfg_addr, 0x1234ABCD);
    expect_value(__wrap_sdm_ext_int_mask_disable, category_id, SDM_EXT_CATEGORY_ID_EXT_INTR);
    expect_value(__wrap_sdm_ext_int_mask_disable, interrupt_mask, SET_BIT_MASK(SDM_EXT_AHBS_WABORT_ERR_INTR));
    will_return(__wrap_sdm_ext_int_mask_disable, 0x0);
    will_return(__wrap_mmio_read32, SET_BIT_MASK(SDM_EXT_EMCPU_WDT_ERR_INTR)); // Value return for status_register read in first execution
    will_return(__wrap_mmio_read32, 0x0); // Value return for mask_register read in first execution
    will_return(__wrap_mmio_read32, SET_BIT_MASK(SDM_EXT_AHBS_WABORT_ERR_INTR)); // Value return for status_register read in second execution
    will_return(__wrap_mmio_read32, 0x0); // Value return for mask_register read in second execution

    // Get the pointer to ISR function so we can invoke it
    accel_intr_get_virt_isr_cb(&cb_fun);

    cb_fun((void*)HW_INT_SDM_NVIC_INT);
    cb_fun((void*)HW_INT_SDM_NVIC_INT);
}