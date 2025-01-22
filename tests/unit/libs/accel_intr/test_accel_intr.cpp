//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_accel_intr.cpp
 * Unit test file for accel_intr lib
 */

/*-------------------------------- Includes ---------------------------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep

extern "C" {
#include "accel_intr.h"      // for accel_scp_intr_init, ACCEL_INTR_RET_SUC...
#include "accel_intr_priv.h" // for accel_intr_get_accel_type_from_irq_num, SDMSS_I...
#include "fpfw_status.h"     // for FPFW_STATUS_SUCCESS
#include "mock.h"            // for mmio_set_mock_data...

#include <accelerator_ip.h>
#include <accelip_id.h> // for ACCEL_ID_CDED, ACCEL_ID_SDM
#include <cdedss_config_regs.h>
#include <idsw_kng.h>      // for KNG_PLAT_ID
#include <nvic.h>          // for NVIC_STATUS_SUCCESS
#include <silibs_status.h> // for SILIBS_SUCCESS
#include <stdint.h>        // for uint32_t
#include <stdio.h>

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

static uint32_t irq_num = SDMSS_IRQ_NUMBER;

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/

/*****************************
 * MOCKS
 *****************************/

/**
 * @brief : Mock function for send_sdm_msg_async_request
 *
 * @param[in] IRQnum : IRQNum based on SDM / CDED
 *
 */
void __wrap_send_sdm_msg_async_request(uint32_t IRQnum)
{
    check_expected(IRQnum);
}

/**
 * @brief : Mock function for fatal_intr_async_request
 *
 * @param[in] IRQnum : IRQNum based on SDM / CDED
 *
 */
void __wrap_send_fatal_intr_async_request(uint32_t IRQnum)
{
    check_expected(IRQnum);
}

/**
 * @brief : Mock function for Mailbox request
 *
 * @param[in] IRQnum : IRQNum based on SDM / CDED
 *
 */
void __wrap_send_mailbox_async_request(uint32_t IRQnum)
{
    check_expected(IRQnum);
}

/**
 * @brief : Mock function for idsw_get_platform_sdv
 *
 */
KNG_PLAT_ID __wrap_idsw_get_platform_sdv()
{
    return mock_type(KNG_PLAT_ID);
}

void __wrap_accel_mbox_sw_intr_cb(void* ctx)
{
    check_expected(ctx);
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}

void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    check_expected(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);
}

} // extern "c"

/**
 * @brief : Tests accel_scp_intr_init with all passing on SDM
 */
TEST_FUNCTION(test_accel_scp_intr_init_pass_sdm, nullptr, nullptr)
{
    expect_any(__wrap_fpfw_timer_create, timer);
    expect_any(__wrap_fpfw_timer_create, cb);
    will_return_always(__wrap_fpfw_timer_create, FPFW_STATUS_SUCCESS);

    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    will_return_always(__wrap_set_ext_int_sub_system, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_update32, addr);
    expect_any_always(__wrap_mmio_update32, data);
    expect_any_always(__wrap_mmio_update32, mask);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, SDMSS_IRQ_NUMBER);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, accel_intr_isr_scp);
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, (void*)SDMSS_IRQ_NUMBER);
    will_return_always(__wrap_nvic_irq_set_isr_with_param, NVIC_STATUS_SUCCESS);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, SDMSS_IRQ_NUMBER);
    expect_value(__wrap_nvic_irq_enable, irq_num, SDMSS_IRQ_NUMBER);

    assert_int_equal(accel_scp_intr_init(accel_intr_get_accel_type_from_irq_num(SDMSS_IRQ_NUMBER)), ACCEL_INTR_RET_SUCCESS);
}

/**
 * @brief : Tests accel_scp_intr_init with all passing on CDED
 */
TEST_FUNCTION(test_accel_scp_intr_init_pass_cded, nullptr, nullptr)
{
    expect_any(__wrap_fpfw_timer_create, timer);
    expect_any(__wrap_fpfw_timer_create, cb);
    will_return_always(__wrap_fpfw_timer_create, FPFW_STATUS_SUCCESS);

    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    will_return_always(__wrap_set_ext_int_sub_system, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_update32, addr);
    expect_any_always(__wrap_mmio_update32, data);
    expect_any_always(__wrap_mmio_update32, mask);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, CDEDSS_IRQ_NUMBER);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, accel_intr_isr_scp);
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, (void*)CDEDSS_IRQ_NUMBER);
    will_return_always(__wrap_nvic_irq_set_isr_with_param, NVIC_STATUS_SUCCESS);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, CDEDSS_IRQ_NUMBER);
    expect_value(__wrap_nvic_irq_enable, irq_num, CDEDSS_IRQ_NUMBER);

    assert_int_equal(accel_scp_intr_init(accel_intr_get_accel_type_from_irq_num(CDEDSS_IRQ_NUMBER)), ACCEL_INTR_RET_SUCCESS);
}

/**
 * @brief : Tests accel_mcp_intr_init with all passing on SDM
 */
TEST_FUNCTION(test_accel_mcp_intr_init_pass_sdm, nullptr, nullptr)
{
    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    will_return_always(__wrap_set_ext_int_sub_system, SILIBS_SUCCESS);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, SDMSS_IRQ_NUMBER);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, accel_intr_isr_mcp);
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, (void*)SDMSS_IRQ_NUMBER);
    will_return_always(__wrap_nvic_irq_set_isr_with_param, NVIC_STATUS_SUCCESS);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, SDMSS_IRQ_NUMBER);
    expect_value(__wrap_nvic_irq_enable, irq_num, SDMSS_IRQ_NUMBER);

    assert_int_equal(accel_mcp_intr_init(accel_intr_get_accel_type_from_irq_num(SDMSS_IRQ_NUMBER)), ACCEL_INTR_RET_SUCCESS);
}

/**
 * @brief : Tests accel_mcp_intr_init with all passing on CDED
 */
TEST_FUNCTION(test_accel_mcp_intr_init_pass_cded, nullptr, nullptr)
{
    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    will_return_always(__wrap_set_ext_int_sub_system, SILIBS_SUCCESS);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, CDEDSS_IRQ_NUMBER);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, accel_intr_isr_mcp);
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, (void*)CDEDSS_IRQ_NUMBER);
    will_return_always(__wrap_nvic_irq_set_isr_with_param, NVIC_STATUS_SUCCESS);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, CDEDSS_IRQ_NUMBER);
    expect_value(__wrap_nvic_irq_enable, irq_num, CDEDSS_IRQ_NUMBER);

    assert_int_equal(accel_mcp_intr_init(accel_intr_get_accel_type_from_irq_num(CDEDSS_IRQ_NUMBER)), ACCEL_INTR_RET_SUCCESS);
}

/**
 * @brief : Tests accel_scp_intr_init with failure in timer init
 */
TEST_FUNCTION(test_accel_scp_intr_init_fail_timer_init, nullptr, nullptr)
{
    expect_any(__wrap_fpfw_timer_create, timer);
    expect_any(__wrap_fpfw_timer_create, cb);
    will_return_always(__wrap_fpfw_timer_create, FPFW_STATUS_INVALID_ARGS);

    assert_int_equal(accel_scp_intr_init(accel_intr_get_accel_type_from_irq_num(SDMSS_IRQ_NUMBER)),
                     ACCEL_INTR_RET_FAIL_TIMER_CREATE);
}

/**
 * @brief : Tests accel_scp_intr_init with failure in nvic init
 */
TEST_FUNCTION(test_accel_scp_intr_init_fail_nvic_init, nullptr, nullptr)
{
    expect_any(__wrap_fpfw_timer_create, timer);
    expect_any(__wrap_fpfw_timer_create, cb);
    will_return_always(__wrap_fpfw_timer_create, FPFW_STATUS_SUCCESS);

    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    will_return_always(__wrap_set_ext_int_sub_system, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_update32, addr);
    expect_any_always(__wrap_mmio_update32, data);
    expect_any_always(__wrap_mmio_update32, mask);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, SDMSS_IRQ_NUMBER);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, accel_intr_isr_scp);
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, (void*)SDMSS_IRQ_NUMBER);
    will_return_always(__wrap_nvic_irq_set_isr_with_param, NVIC_STATUS_ERROR);

    assert_int_equal(accel_scp_intr_init(accel_intr_get_accel_type_from_irq_num(SDMSS_IRQ_NUMBER)),
                     ACCEL_INTR_RET_FAIL_INTR_NVIC);
}

/**
 * @brief : Tests accel_scp_intr_init with failure in timer init
 */
TEST_FUNCTION(test_accel_scp_intr_init_fail_invalid_arg1, nullptr, nullptr)
{
    assert_int_equal(accel_scp_intr_init(NUM_VALID_ACCEL_ID), ACCEL_INTR_RET_FAIL_INTR_INIT);
}

/**
 * @brief : Tests accel_mcp_intr_init with failure in timer init
 */
TEST_FUNCTION(test_accel_mcp_intr_init_fail_invalid_arg1, nullptr, nullptr)
{
    assert_int_equal(accel_mcp_intr_init(NUM_VALID_ACCEL_ID), ACCEL_INTR_RET_FAIL_INTR_INIT);
}

/**
 * @brief : Tests accel_mcp_intr_init with failure in nvic init
 */
TEST_FUNCTION(test_accel_mcp_intr_init_fail_nvic_init, nullptr, nullptr)
{
    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    will_return_always(__wrap_set_ext_int_sub_system, SILIBS_SUCCESS);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, SDMSS_IRQ_NUMBER);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, accel_intr_isr_mcp);
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, (void*)SDMSS_IRQ_NUMBER);
    will_return_always(__wrap_nvic_irq_set_isr_with_param, NVIC_STATUS_ERROR);

    assert_int_equal(accel_mcp_intr_init(accel_intr_get_accel_type_from_irq_num(SDMSS_IRQ_NUMBER)),
                     ACCEL_INTR_RET_FAIL_INTR_NVIC);
}

/**
 * @brief : Tests accel_intr_isr_scp_pass with all passing and both FATAL and SDM_MSG interrupt present
 */
TEST_FUNCTION(test_accel_intr_isr_scp_pass, nullptr, nullptr)
{
    mmio_set_mock_data(0x12345678, 0xFFFFFFFF);
    mmio_set_mock_data(0xABCDEF12, 0x0);

    set_int_status(true);

    will_return_always(__wrap_is_sdm_ext_int_status_set, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return_always(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_read32, addr);
    expect_any_always(__wrap_mmio_write32, addr);
    expect_any_always(__wrap_mmio_write32, data);

    expect_value(__wrap_send_sdm_msg_async_request, IRQnum, irq_num);

    // accel_intr_mbox_isr()
    will_return(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    expect_value(__wrap_send_mailbox_async_request, IRQnum, irq_num);

    expect_value(__wrap_send_fatal_intr_async_request, IRQnum, irq_num);

    expect_value(__wrap_nvic_irq_disable, irq_num, irq_num);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);

    accel_intr_isr_scp((void*)irq_num);
}

/**
 * @brief : Tests accel_intr_isr_scp_pass with all passing but no interrupt at level 2
 */
TEST_FUNCTION(test_sdm_intr_isr_scp_pass_no_level2_intr, nullptr, nullptr)
{
    mmio_set_mock_data(0x12345678, 0xFFFFFFFF);
    mmio_set_mock_data(0xABCDEF12, 0x0);

    set_int_status(false);

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    will_return_always(__wrap_is_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    will_return_always(__wrap_is_sdm_ext_int_status_set, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return_always(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_read32, addr);
    expect_any_always(__wrap_mmio_write32, addr);
    expect_any_always(__wrap_mmio_write32, data);

    expect_value(__wrap_send_fatal_intr_async_request, IRQnum, irq_num);

    expect_value(__wrap_nvic_irq_disable, irq_num, irq_num);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);

    accel_intr_isr_scp((void*)irq_num);
}

/**
 * @brief : Tests accel_intr_isr_scp_pass with all passing but no valid interrupt
 */
TEST_FUNCTION(test_accel_intr_isr_scp_pass_no_interrupt, nullptr, nullptr)
{
    mmio_set_mock_data(0x12345678, 0x0);
    mmio_set_mock_data(0xABCDEF12, 0xFFFFFFFF);

    set_int_status(true);

    will_return_always(__wrap_is_sdm_ext_int_status_set, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return_always(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_read32, addr);

    expect_value(__wrap_send_sdm_msg_async_request, IRQnum, irq_num);

    // accel_intr_mbox_isr()
    will_return(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    expect_value(__wrap_send_mailbox_async_request, IRQnum, irq_num);

    accel_intr_isr_scp((void*)irq_num);
}

/**
 * @brief : Tests accel_intr_handle_fatal_intr_recvd with all passing and FATAL interrupt present
 */
TEST_FUNCTION(test_accel_intr_handle_fatal_intr_recvd_pass, NULL, NULL)
{
    mmio_set_mock_data(0x12345678, 0xFFFFFFFF);
    mmio_set_mock_data(0xABCDEF12, 0x0);

    set_int_status(true);

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    will_return_always(__wrap_is_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    will_return_always(__wrap_is_sdm_ext_int_status_set, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return_always(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_read32, addr);
    expect_any_always(__wrap_mmio_write32, addr);
    expect_any_always(__wrap_mmio_write32, data);

    expect_any(__wrap_fpfw_timer_enable, timer);
    will_return_always(__wrap_fpfw_timer_enable, FPFW_STATUS_SUCCESS);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);

    accel_intr_handle_fatal_intr_recvd(SDMSS_IRQ_NUMBER);

    expect_any(__wrap_fpfw_timer_reset, timer);
    will_return_always(__wrap_fpfw_timer_reset, FPFW_STATUS_SUCCESS);

    expect_any_always(__wrap_mmio_update32, addr);
    expect_any_always(__wrap_mmio_update32, data);
    expect_any_always(__wrap_mmio_update32, mask);

    accel_intr_handle_sdm_msg_recvd(SDMSS_IRQ_NUMBER);
}

/**
 * @brief : Tests accel_intr_handle_fatal_intr_recvd with all passing and only EMCPU_WDT interrupt present.
 * This will take path for Accel emcpu reset
 */
TEST_FUNCTION(test_accel_intr_handle_fatal_intr_recvd_pass_accel_emcpu_reset, NULL, NULL)
{
    mmio_set_mock_data(0x12345678, 0x3);
    mmio_set_mock_data(0xABCDEF12, 0x0);

    set_int_status(true);

    will_return_always(__wrap_is_sdm_ext_int_status_set, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return_always(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_read32, addr);
    expect_any_always(__wrap_mmio_write32, addr);
    expect_any_always(__wrap_mmio_write32, data);

    expect_any(__wrap_fpfw_timer_enable, timer);
    will_return_always(__wrap_fpfw_timer_enable, FPFW_STATUS_SUCCESS);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);

    accel_intr_handle_fatal_intr_recvd(CDEDSS_IRQ_NUMBER);

    expect_any(__wrap_fpfw_timer_reset, timer);
    will_return_always(__wrap_fpfw_timer_reset, FPFW_STATUS_SUCCESS);

    expect_any_always(__wrap_mmio_update32, addr);
    expect_any_always(__wrap_mmio_update32, data);
    expect_any_always(__wrap_mmio_update32, mask);

    accel_intr_handle_sdm_msg_recvd(CDEDSS_IRQ_NUMBER);
}

/**
 * @brief : Tests accel_intr_handle_fatal_intr_recvd with all passing and but no interrupt at level 2
 */
TEST_FUNCTION(test_accel_intr_handle_fatal_intr_recvd_pass_no_level2_intr, NULL, NULL)
{
    mmio_set_mock_data(0x12345678, 0xFFFFFFFF);
    mmio_set_mock_data(0xABCDEF12, 0x0);

    set_int_status(false);

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    will_return_always(__wrap_is_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    will_return_always(__wrap_is_sdm_ext_int_status_set, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return_always(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_read32, addr);
    expect_any_always(__wrap_mmio_write32, addr);
    expect_any_always(__wrap_mmio_write32, data);

    expect_any(__wrap_fpfw_timer_enable, timer);
    will_return_always(__wrap_fpfw_timer_enable, FPFW_STATUS_SUCCESS);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);

    accel_intr_handle_fatal_intr_recvd(SDMSS_IRQ_NUMBER);

    accel_intr_handle_fatal_intr_recvd(SDMSS_IRQ_NUMBER);

    expect_any(__wrap_fpfw_timer_reset, timer);
    will_return_always(__wrap_fpfw_timer_reset, FPFW_STATUS_SUCCESS);

    expect_any_always(__wrap_mmio_update32, addr);
    expect_any_always(__wrap_mmio_update32, data);
    expect_any_always(__wrap_mmio_update32, mask);

    accel_intr_handle_sdm_msg_recvd(SDMSS_IRQ_NUMBER);
}

/**
 * @brief : Tests accel_intr_handle_fatal_intr_recvd with all passing but no valid interrupt present
 */
TEST_FUNCTION(test_accel_intr_handle_fatal_intr_recvd_pass_no_interrupt, NULL, NULL)
{
    mmio_set_mock_data(0x12345678, 0x0);
    mmio_set_mock_data(0xABCDEF12, 0x0);

    set_int_status(false);

    will_return_always(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return_always(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_read32, addr);
    expect_any_always(__wrap_mmio_update32, addr);
    expect_any_always(__wrap_mmio_update32, data);
    expect_any_always(__wrap_mmio_update32, mask);

    accel_intr_handle_fatal_intr_recvd(SDMSS_IRQ_NUMBER);
}

/**
 * @brief : Tests accel_intr_handle_sdm_msg_recvd with all passing
 */
TEST_FUNCTION(test_accel_intr_handle_sdm_msg_recvd, NULL, NULL)
{
    accel_intr_handle_sdm_msg_recvd(SDMSS_IRQ_NUMBER);
}

/**
 * @brief : Tests accel_intr_handle_sdm_msg_recvd_timeout : Check 1st timeout path
 */
TEST_FUNCTION(test_accel_intr_handle_sdm_msg_recv_timeout_count_0, NULL, NULL)
{
    accel_intr_crash_dump_collection_timer_data_t accel_intr_crash_dump_collection_timer_data = {};
    accel_intr_crash_dump_collection_timer_data.IRQnum = SDMSS_IRQ_NUMBER;
    accel_intr_crash_dump_collection_timer_data.timeout_count = 0x0;

    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_write32, addr);
    expect_any_always(__wrap_mmio_write32, data);

    expect_any_always(__wrap_mmio_update32, addr);
    expect_any_always(__wrap_mmio_update32, data);
    expect_any_always(__wrap_mmio_update32, mask);

    expect_any(__wrap_fpfw_timer_enable, timer);
    will_return_always(__wrap_fpfw_timer_enable, FPFW_STATUS_SUCCESS);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);

    fpfw_timer_callback timer_cb = fpfw_mock_get_timer_cb();

    assert_non_null(timer_cb);
    timer_cb(&accel_intr_crash_dump_collection_timer_data, 0x0);
}

/**
 * @brief : Tests accel_intr_handle_sdm_msg_recvd_timeout : Check 2nd timeout path
 */
TEST_FUNCTION(test_accel_intr_handle_sdm_msg_recv_timeout_count_1, NULL, NULL)
{
    accel_intr_crash_dump_collection_timer_data_t accel_intr_crash_dump_collection_timer_data = {};
    accel_intr_crash_dump_collection_timer_data.IRQnum = CDEDSS_IRQ_NUMBER;
    accel_intr_crash_dump_collection_timer_data.timeout_count = 0x1;

    fpfw_timer_callback timer_cb = fpfw_mock_get_timer_cb();

    assert_non_null(timer_cb);
    timer_cb(&accel_intr_crash_dump_collection_timer_data, 0x0);
}

TEST_FUNCTION(test_accel_intr_handle_mbox_recvd_sdm__pass, NULL, NULL)
{
    void* cb_ctx = (void*)0x12345678;

    accel_intr_set_mbx_ctx(ACCEL_ID_SDM, (void*)cb_ctx);
    expect_value(__wrap_accel_mbox_sw_intr_cb, ctx, cb_ctx);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    will_return(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);

    accel_intr_handle_mbox_recvd(SDMSS_IRQ_NUMBER);
}

TEST_FUNCTION(test_accel_intr_handle_mbox_recvd_sdm__fail1, NULL, NULL)
{
    accel_intr_set_mbx_ctx(ACCEL_ID_SDM, (void*)NULL);

    accel_intr_handle_mbox_recvd(SDMSS_IRQ_NUMBER);
}

TEST_FUNCTION(test_accel_intr_init_sdm_scp__pass, NULL, NULL)
{
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, SDMSS_IRQ_NUMBER);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, accel_intr_isr_scp);
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, (void*)SDMSS_IRQ_NUMBER);
    will_return_always(__wrap_nvic_irq_set_isr_with_param, NVIC_STATUS_SUCCESS);

    assert_int_equal(accel_intr_init(ACCEL_ID_SDM), ACCEL_INTR_RET_SUCCESS);
}

TEST_FUNCTION(test_accel_intr_init_sdm_mcp__pass, NULL, NULL)
{
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, SDMSS_IRQ_NUMBER);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, accel_intr_isr_mcp);
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, (void*)SDMSS_IRQ_NUMBER);
    will_return_always(__wrap_nvic_irq_set_isr_with_param, NVIC_STATUS_SUCCESS);

    assert_int_equal(accel_intr_init(ACCEL_ID_SDM), ACCEL_INTR_RET_SUCCESS);
}

TEST_FUNCTION(test_accel_intr_init_sdm_scp__fail1, NULL, NULL)
{
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, SDMSS_IRQ_NUMBER);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, accel_intr_isr_scp);
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, (void*)SDMSS_IRQ_NUMBER);
    will_return_always(__wrap_nvic_irq_set_isr_with_param, NVIC_STATUS_ERROR);

    assert_int_equal(accel_intr_init(ACCEL_ID_SDM), ACCEL_INTR_RET_FAIL_INTR_NVIC);
}

TEST_FUNCTION(test_accel_intr_init_sdm_mcp__fail1, NULL, NULL)
{
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);

    // FPFwCoreInterruptRegisterCallback
    expect_value(__wrap_nvic_irq_set_isr_with_param, irq_num, SDMSS_IRQ_NUMBER);
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, accel_intr_isr_mcp);
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, (void*)SDMSS_IRQ_NUMBER);
    will_return_always(__wrap_nvic_irq_set_isr_with_param, NVIC_STATUS_ERROR);

    assert_int_equal(accel_intr_init(ACCEL_ID_SDM), ACCEL_INTR_RET_FAIL_INTR_NVIC);
}

/**
 * @brief : SDM receive CDED CP fatal error
 */
TEST_FUNCTION(test_sdm_intr_handle_cded_cp_fatal_intr_recvd__pass1, NULL, NULL)
{
    mmio_set_mock_data(0x12345678, 1 << SDM_EXT_CP_FATAL_ERR_INTR);
    mmio_set_mock_data(0xABCDEF12, 0x0);

    set_int_status(false);

    // accel_intr_process_fatal_interrupts()
    will_return(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    expect_any_always(__wrap_mmio_read32, addr);

    // accel_intr_cded_cp_err_fn()
    expect_any_always(__wrap_crash_dump_bug_check, errorCode);

    // accel_intr_clear_and_unmask_interrupts()
    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    expect_any_always(__wrap_mmio_update32, addr);
    expect_any_always(__wrap_mmio_update32, data);
    expect_any_always(__wrap_mmio_update32, mask);

    accel_intr_handle_fatal_intr_recvd(SDMSS_IRQ_NUMBER);
}

/**
 * @brief : CDED receive CDED CP fatal error IC_XB_PERR
 */
TEST_FUNCTION(test_cded_intr_handle_cded_cp_fatal_intr_recvd__pass2, NULL, NULL)
{
    uint32_t coproc_apb_addr = accelerator_ip_get_atu_mapped_cfg_address(ACCEL_ID_CDED) + CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS;
    uint32_t cded_cfg_fatal_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_CDED_CFG_FATAL, coproc_apb_addr);
    mmio_set_mock_data(0x12345678, 1 << SDM_EXT_CP_FATAL_ERR_INTR);
    mmio_set_mock_data(0xABCDEF12, 0x0);
    mmio_set_mock_data(coproc_apb_addr + cded_cfg_fatal_offset, 1 << IC_XB_PERR);

    set_int_status(false);

    // accel_intr_process_fatal_interrupts()
    will_return(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    expect_any_always(__wrap_mmio_read32, addr);

    // accel_intr_cded_cp_err_fn()
    will_return(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    //  handle_cded_cp_level2_intr()
    will_return(__wrap_cded_int_mask_disable, SILIBS_SUCCESS);

    // accel_intr_request_crash_dump_collection()
    expect_any(__wrap_fpfw_timer_enable, timer);
    will_return(__wrap_fpfw_timer_enable, FPFW_STATUS_SUCCESS);
    //      accel_intr_single_level_irq_init()
    will_return(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    will_return(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    //      accel_intr_handle_sdm_msg_send()
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    expect_any_always(__wrap_mmio_write32, addr);
    expect_any_always(__wrap_mmio_write32, data);

    accel_intr_handle_fatal_intr_recvd(CDEDSS_IRQ_NUMBER);
}

/**
 * @brief : CDED receive CDED CP fatal error CCMP_FATAL_INT
 */
TEST_FUNCTION(test_cded_intr_handle_cded_cp_fatal_intr_recvd__pass3, NULL, NULL)
{
    uint32_t coproc_apb_addr = accelerator_ip_get_atu_mapped_cfg_address(ACCEL_ID_CDED) + CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS;
    uint32_t cded_cfg_fatal_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_CDED_CFG_FATAL, coproc_apb_addr);
    uint32_t cded_cfg_ccmp0_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_COMPRESSOR0_FATAL, coproc_apb_addr);
    mmio_set_mock_data(0x12345678, 1 << SDM_EXT_CP_FATAL_ERR_INTR);
    mmio_set_mock_data(0xABCDEF12, 0x0);
    mmio_set_mock_data(coproc_apb_addr + cded_cfg_fatal_offset, 1 << CCMP_FATAL_INT);
    mmio_set_mock_data(coproc_apb_addr + cded_cfg_ccmp0_offset, 1 << CCMP_IB_PERR);

    set_int_status(false);

    // accel_intr_process_fatal_interrupts()
    will_return(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    expect_any_always(__wrap_mmio_read32, addr);

    // accel_intr_cded_cp_err_fn()
    will_return(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    //  handle_cded_cp_level2_intr()
    will_return(__wrap_cded_int_mask_disable, SILIBS_SUCCESS);

    accel_intr_handle_fatal_intr_recvd(CDEDSS_IRQ_NUMBER);
}

/**
 * @brief : CDED receive CDED CP fatal error DCMP_FATAL_INT
 */
TEST_FUNCTION(test_cded_intr_handle_cded_cp_fatal_intr_recvd__pass4, NULL, NULL)
{
    uint32_t coproc_apb_addr = accelerator_ip_get_atu_mapped_cfg_address(ACCEL_ID_CDED) + CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS;
    uint32_t cded_cfg_fatal_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_CDED_CFG_FATAL, coproc_apb_addr);
    uint32_t cded_cfg_dcmp0_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_DECOMPRESSOR0_FATAL, coproc_apb_addr);
    mmio_set_mock_data(0x12345678, 1 << SDM_EXT_CP_FATAL_ERR_INTR);
    mmio_set_mock_data(0xABCDEF12, 0x0);
    mmio_set_mock_data(coproc_apb_addr + cded_cfg_fatal_offset, 1 << DCMP_FATAL_INT);
    mmio_set_mock_data(coproc_apb_addr + cded_cfg_dcmp0_offset, 1 << DCMP_IB_PERR);

    set_int_status(false);

    // accel_intr_process_fatal_interrupts()
    will_return(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    expect_any_always(__wrap_mmio_read32, addr);

    // accel_intr_cded_cp_err_fn()
    will_return(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    //  handle_cded_cp_level2_intr()
    will_return(__wrap_cded_int_mask_disable, SILIBS_SUCCESS);

    accel_intr_handle_fatal_intr_recvd(CDEDSS_IRQ_NUMBER);
}

/**
 * @brief : CDED receive CDED CP fatal error AES_FATAL_INT
 */
TEST_FUNCTION(test_cded_intr_handle_cded_cp_fatal_intr_recvd__pass5, NULL, NULL)
{
    uint32_t coproc_apb_addr = accelerator_ip_get_atu_mapped_cfg_address(ACCEL_ID_CDED) + CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS;
    uint32_t cded_cfg_fatal_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_CDED_CFG_FATAL, coproc_apb_addr);
    uint32_t cded_cfg_aes0_offset = cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_AES_FATAL, coproc_apb_addr);
    mmio_set_mock_data(0x12345678, 1 << SDM_EXT_CP_FATAL_ERR_INTR);
    mmio_set_mock_data(0xABCDEF12, 0x0);
    mmio_set_mock_data(coproc_apb_addr + cded_cfg_fatal_offset, 1 << AES_FATAL_INT);
    mmio_set_mock_data(coproc_apb_addr + cded_cfg_aes0_offset, 1 << CDED_AES_PL_PERR);

    set_int_status(false);

    // accel_intr_process_fatal_interrupts()
    will_return(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    expect_any_always(__wrap_mmio_read32, addr);

    // accel_intr_cded_cp_err_fn()
    will_return(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    //  handle_cded_cp_level2_intr()
    will_return(__wrap_cded_int_mask_disable, SILIBS_SUCCESS);

    accel_intr_handle_fatal_intr_recvd(CDEDSS_IRQ_NUMBER);
}

/**
 * @brief : CDED receive CDED CP fatal error
 * level 2 status register is zero. Spurious interrupt
 */
TEST_FUNCTION(test_cded_intr_handle_cded_cp_fatal_intr_recvd__fail1, NULL, NULL)
{
    uint32_t coproc_apb_addr = accelerator_ip_get_atu_mapped_cfg_address(ACCEL_ID_CDED) + CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS;
    uint32_t cded_cfg_fatal_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_CDED_CFG_FATAL, coproc_apb_addr);
    mmio_set_mock_data(0x12345678, 1 << SDM_EXT_CP_FATAL_ERR_INTR);
    mmio_set_mock_data(0xABCDEF12, 0x0);
    mmio_set_mock_data(coproc_apb_addr + cded_cfg_fatal_offset, 0x0);

    set_int_status(false);

    // accel_intr_process_fatal_interrupts()
    will_return(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    expect_any_always(__wrap_mmio_read32, addr);

    // accel_intr_cded_cp_err_fn()
    will_return(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);

    // accel_intr_clear_and_unmask_interrupts()
    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    expect_any_always(__wrap_mmio_update32, addr);
    expect_any_always(__wrap_mmio_update32, data);
    expect_any_always(__wrap_mmio_update32, mask);

    accel_intr_handle_fatal_intr_recvd(CDEDSS_IRQ_NUMBER);
}

/**
 * @brief : CDED receive CDED CP fatal error
 * level 3 status register is zero. Spurious interrupt
 */
TEST_FUNCTION(test_cded_intr_handle_cded_cp_fatal_intr_recvd__fail2, NULL, NULL)
{
    uint32_t coproc_apb_addr = accelerator_ip_get_atu_mapped_cfg_address(ACCEL_ID_CDED) + CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS;
    uint32_t cded_cfg_fatal_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_CDED_CFG_FATAL, coproc_apb_addr);
    uint32_t cded_cfg_ccmp0_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_COMPRESSOR0_FATAL, coproc_apb_addr);
    mmio_set_mock_data(0x12345678, 1 << SDM_EXT_CP_FATAL_ERR_INTR);
    mmio_set_mock_data(0xABCDEF12, 0x0);
    mmio_set_mock_data(coproc_apb_addr + cded_cfg_fatal_offset, 1 << CCMP_FATAL_INT);
    mmio_set_mock_data(coproc_apb_addr + cded_cfg_ccmp0_offset, 0x0);

    set_int_status(false);

    // accel_intr_process_fatal_interrupts()
    will_return(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    expect_any_always(__wrap_mmio_read32, addr);

    // accel_intr_cded_cp_err_fn()
    will_return(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    //  handle_cded_cp_level2_intr()
    will_return(__wrap_cded_int_mask_disable, SILIBS_SUCCESS);
    will_return(__wrap_cded_clear_trigger_int_mask, SILIBS_SUCCESS);
    will_return(__wrap_cded_int_mask_status_clear, SILIBS_SUCCESS);
    will_return(__wrap_cded_int_mask_enable, SILIBS_SUCCESS);

    // accel_intr_clear_and_unmask_interrupts()
    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    expect_any_always(__wrap_mmio_update32, addr);
    expect_any_always(__wrap_mmio_update32, data);
    expect_any_always(__wrap_mmio_update32, mask);

    accel_intr_handle_fatal_intr_recvd(CDEDSS_IRQ_NUMBER);
}

/**
 * @brief : CDED receive CDED CP fatal error
 * level 3 status register is zero. Spurious interrupt
 */
TEST_FUNCTION(test_cded_intr_handle_cded_cp_fatal_intr_recvd__fail3, NULL, NULL)
{
    uint32_t coproc_apb_addr = accelerator_ip_get_atu_mapped_cfg_address(ACCEL_ID_CDED) + CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS;
    uint32_t cded_cfg_fatal_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_CDED_CFG_FATAL, coproc_apb_addr);
    uint32_t cded_cfg_dcmp0_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_DECOMPRESSOR0_FATAL, coproc_apb_addr);
    mmio_set_mock_data(0x12345678, 1 << SDM_EXT_CP_FATAL_ERR_INTR);
    mmio_set_mock_data(0xABCDEF12, 0x0);
    mmio_set_mock_data(coproc_apb_addr + cded_cfg_fatal_offset, 1 << DCMP_FATAL_INT);
    mmio_set_mock_data(coproc_apb_addr + cded_cfg_dcmp0_offset, 0x0);

    set_int_status(false);

    // accel_intr_process_fatal_interrupts()
    will_return(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    expect_any_always(__wrap_mmio_read32, addr);

    // accel_intr_cded_cp_err_fn()
    will_return(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    //  handle_cded_cp_level2_intr()
    will_return(__wrap_cded_int_mask_disable, SILIBS_SUCCESS);
    will_return(__wrap_cded_clear_trigger_int_mask, SILIBS_SUCCESS);
    will_return(__wrap_cded_int_mask_status_clear, SILIBS_SUCCESS);
    will_return(__wrap_cded_int_mask_enable, SILIBS_SUCCESS);

    // accel_intr_clear_and_unmask_interrupts()
    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    expect_any_always(__wrap_mmio_update32, addr);
    expect_any_always(__wrap_mmio_update32, data);
    expect_any_always(__wrap_mmio_update32, mask);

    accel_intr_handle_fatal_intr_recvd(CDEDSS_IRQ_NUMBER);
}

/**
 * @brief : CDED receive CDED CP fatal error
 * level 3 status register is zero. Spurious interrupt
 */
TEST_FUNCTION(test_cded_intr_handle_cded_cp_fatal_intr_recvd__fail4, NULL, NULL)
{
    uint32_t coproc_apb_addr = accelerator_ip_get_atu_mapped_cfg_address(ACCEL_ID_CDED) + CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS;
    uint32_t cded_cfg_fatal_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_CDED_CFG_FATAL, coproc_apb_addr);
    uint32_t cded_cfg_aes_offset = cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_AES_FATAL, coproc_apb_addr);
    mmio_set_mock_data(0x12345678, 1 << SDM_EXT_CP_FATAL_ERR_INTR);
    mmio_set_mock_data(0xABCDEF12, 0x0);
    mmio_set_mock_data(coproc_apb_addr + cded_cfg_fatal_offset, 1 << AES_FATAL_INT);
    mmio_set_mock_data(coproc_apb_addr + cded_cfg_aes_offset, 0x0);

    set_int_status(false);

    // accel_intr_process_fatal_interrupts()
    will_return(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    expect_any_always(__wrap_mmio_read32, addr);

    // accel_intr_cded_cp_err_fn()
    will_return(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    //  handle_cded_cp_level2_intr()
    will_return(__wrap_cded_int_mask_disable, SILIBS_SUCCESS);
    will_return(__wrap_cded_clear_trigger_int_mask, SILIBS_SUCCESS);
    will_return(__wrap_cded_int_mask_status_clear, SILIBS_SUCCESS);
    will_return(__wrap_cded_int_mask_enable, SILIBS_SUCCESS);

    // accel_intr_clear_and_unmask_interrupts()
    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    expect_any_always(__wrap_mmio_update32, addr);
    expect_any_always(__wrap_mmio_update32, data);
    expect_any_always(__wrap_mmio_update32, mask);

    accel_intr_handle_fatal_intr_recvd(CDEDSS_IRQ_NUMBER);
}
