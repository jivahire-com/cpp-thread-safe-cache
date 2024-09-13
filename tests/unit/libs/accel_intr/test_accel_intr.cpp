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
#include "accel_intr.h"      // for accel_intr_irq_init, ACCEL_INTR_RET_SUC...
#include "accel_intr_priv.h" // for accel_intr_get_accel_type_from_irq_num, SDMSS_I...
#include "fpfw_status.h"     // for FPFW_STATUS_SUCCESS
#include "mock.h"            // for mmio_set_mock_data...

#include <idsw_kng.h>      // for KNG_PLAT_ID
#include <nvic.h>          // for NVIC_STATUS_SUCCESS
#include <silibs_status.h> // for SILIBS_SUCCESS
#include <stdint.h>        // for uint32_t

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
 * @brief : Mock function for idsw_get_platform_sdv
 *
 */
KNG_PLAT_ID __wrap_idsw_get_platform_sdv()
{
    return mock_type(KNG_PLAT_ID);
}

} // extern "c"

/**
 * @brief : Tests accel_intr_irq_init with all passing on SDM
 */
TEST_FUNCTION(test_accel_intr_irq_init_pass_sdm, nullptr, nullptr)
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
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, accel_intr_isr);
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, (void*)SDMSS_IRQ_NUMBER);
    will_return_always(__wrap_nvic_irq_set_isr_with_param, NVIC_STATUS_SUCCESS);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, SDMSS_IRQ_NUMBER);
    expect_value(__wrap_nvic_irq_enable, irq_num, SDMSS_IRQ_NUMBER);

    assert_int_equal(accel_intr_irq_init(accel_intr_get_accel_type_from_irq_num(SDMSS_IRQ_NUMBER)), ACCEL_INTR_RET_SUCCESS);
}

/**
 * @brief : Tests accel_intr_irq_init with all passing on CDED
 */
TEST_FUNCTION(test_accel_intr_irq_init_pass_cded, nullptr, nullptr)
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
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, accel_intr_isr);
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, (void*)CDEDSS_IRQ_NUMBER);
    will_return_always(__wrap_nvic_irq_set_isr_with_param, NVIC_STATUS_SUCCESS);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, CDEDSS_IRQ_NUMBER);
    expect_value(__wrap_nvic_irq_enable, irq_num, CDEDSS_IRQ_NUMBER);

    assert_int_equal(accel_intr_irq_init(accel_intr_get_accel_type_from_irq_num(CDEDSS_IRQ_NUMBER)), ACCEL_INTR_RET_SUCCESS);
}

/**
 * @brief : Tests accel_intr_irq_init with failure in timer init
 */
TEST_FUNCTION(test_accel_intr_irq_init_fail_timer_init, nullptr, nullptr)
{
    expect_any(__wrap_fpfw_timer_create, timer);
    expect_any(__wrap_fpfw_timer_create, cb);
    will_return_always(__wrap_fpfw_timer_create, FPFW_STATUS_INVALID_ARGS);

    assert_int_equal(accel_intr_irq_init(accel_intr_get_accel_type_from_irq_num(SDMSS_IRQ_NUMBER)),
                     ACCEL_INTR_RET_FAIL_TIMER_CREATE);
}

/**
 * @brief : Tests accel_intr_irq_init with failure in nvic init
 */
TEST_FUNCTION(test_accel_intr_irq_init_fail_nvic_init, nullptr, nullptr)
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
    expect_value(__wrap_nvic_irq_set_isr_with_param, isr, accel_intr_isr);
    expect_value(__wrap_nvic_irq_set_isr_with_param, parameter, (void*)SDMSS_IRQ_NUMBER);
    will_return_always(__wrap_nvic_irq_set_isr_with_param, NVIC_STATUS_ERROR);

    assert_int_equal(accel_intr_irq_init(accel_intr_get_accel_type_from_irq_num(SDMSS_IRQ_NUMBER)), ACCEL_INTR_RET_FAIL_INTR_NVIC);
}

/**
 * @brief : Tests accel_intr_isr_pass with all passing and both FATAL and SDM_MSG interrupt present
 */
TEST_FUNCTION(test_accel_intr_isr_pass, nullptr, nullptr)
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
    expect_value(__wrap_send_fatal_intr_async_request, IRQnum, irq_num);

    expect_value(__wrap_nvic_irq_disable, irq_num, irq_num);

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);

    accel_intr_isr((void*)irq_num);
}

/**
 * @brief : Tests accel_intr_isr_pass with all passing but no interrupt at level 2
 */
TEST_FUNCTION(test_accel_intr_isr_pass_no_level2_intr, nullptr, nullptr)
{
    mmio_set_mock_data(0x12345678, 0xFFFFFFFF);
    mmio_set_mock_data(0xABCDEF12, 0x0);

    set_int_status(false);

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

    accel_intr_isr((void*)irq_num);
}

/**
 * @brief : Tests accel_intr_isr_pass with all passing but no valid interrupt
 */
TEST_FUNCTION(test_accel_intr_isr_pass_no_interrupt, nullptr, nullptr)
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

    accel_intr_isr((void*)irq_num);
}

/**
 * @brief : Tests accel_intr_handle_fatal_intr_recvd with all passing and FATAL interrupt present
 */
TEST_FUNCTION(test_accel_intr_handle_fatal_intr_recvd_pass, NULL, NULL)
{
    mmio_set_mock_data(0x12345678, 0xFFFFFFFF);
    mmio_set_mock_data(0xABCDEF12, 0x0);

    set_int_status(true);

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
