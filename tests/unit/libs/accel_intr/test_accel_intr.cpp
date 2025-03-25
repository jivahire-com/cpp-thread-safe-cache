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
#include "accel_intr.h"           // for accel_scp_intr_init, ACCEL_INTR_RET_SUC...
#include "accel_intr_priv.h"      // for accel_intr_get_accel_type_from_irq_num, SDMSS_I...
#include "fpfw_status.h"          // for FPFW_STATUS_SUCCESS
#include "mock.h"                 // for set_int_status
#include "silibs_platform_mock.h" // for mmio_set_mock_data

#include <accelerator_ip.h>
#include <accelip_id.h> // for ACCEL_ID_CDED, ACCEL_ID_SDM
#include <cdedss_config_regs.h>
#include <idsw_kng.h>      // for KNG_PLAT_ID
#include <nvic.h>          // for NVIC_STATUS_SUCCESS
#include <silibs_status.h> // for SILIBS_SUCCESS
#include <stdint.h>        // for uint32_t
#include <stdio.h>
#include <virt_irq.h> // for virt irq macros

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

static uint32_t irq_num = SDMSS_IRQ_NUMBER;

/*--------------------------------- Externs ---------------------------------*/

extern uint32_t accel_intr_atu_map_address[NUM_VALID_ACCEL_ID];

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/

/*****************************
 * MOCKS
 *****************************/

/**
 * @brief : Mock function for fatal_intr_async_request
 *
 * @param[in] accel_type : accel_type SDM / CDED
 *
 */
void __wrap_send_fatal_intr_async_request(ACCEL_ID accel_type)
{
    check_expected(accel_type);
}

/**
 * @brief : Mock function for Mailbox request
 *
 * @param[in] accel_type : accel_type SDM / CDED
 *
 */
void __wrap_send_mailbox_async_request(ACCEL_ID accel_type)
{
    check_expected(accel_type);
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

void __wrap_crash_dump_bug_check_external()
{
    function_called();
}

uint32_t __wrap_atu_svc_accel_atu_addr(ACCEL_ID accel_id)
{
    FPFW_UNUSED(accel_id);

    return mock_type(uint32_t);
}

bool __wrap_crash_dump_is_accel_cd_complete(ACCEL_ID accel_type)
{
    FPFW_UNUSED(accel_type);

    return mock_type(bool);
}

} // extern "c"

/**
 * @brief : Tests accel_scp_intr_init with all passing on SDM
 */
TEST_FUNCTION(test_accel_scp_intr_init_pass_sdm, nullptr, nullptr)
{
    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    expect_any(__wrap_fpfw_timer_create, timer);
    expect_any(__wrap_fpfw_timer_create, cb);
    will_return_always(__wrap_fpfw_timer_create, FPFW_STATUS_SUCCESS);
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);

    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    will_return_always(__wrap_set_ext_int_sub_system, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_update32, addr);
    expect_any_always(__wrap_mmio_update32, data);
    expect_any_always(__wrap_mmio_update32, mask);

    FPFW_UNUSED(irq_num);

    // FPFwCoreInterruptRegisterCallback
    expect_value_count(__wrap_virt_irq_set_isr_with_param, irq_num, SDMSS_IRQ_NUMBER, 9);
    expect_value_count(__wrap_virt_irq_set_isr_with_param, isr, accel_intr_isr_scp, 9);
    expect_value_count(__wrap_virt_irq_set_isr_with_param, parameter, (void*)SDMSS_IRQ_NUMBER, 9);
    will_return_always(__wrap_virt_irq_set_isr_with_param, NVIC_STATUS_SUCCESS);

    // Need to set the global SDM and CDED irq num which is usually set as part of init
    accel_intr_set_irq_num_for_accel(ACCEL_ID_SDM, SDMSS_IRQ_NUMBER);

    assert_int_equal(accel_scp_intr_init(accel_intr_get_accel_type_from_irq_num(SDMSS_IRQ_NUMBER)), ACCEL_INTR_RET_SUCCESS);
}

/**
 * @brief : Tests accel_scp_intr_init with all passing on CDED
 */
TEST_FUNCTION(test_accel_scp_intr_init_pass_cded, nullptr, nullptr)
{
    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    expect_any(__wrap_fpfw_timer_create, timer);
    expect_any(__wrap_fpfw_timer_create, cb);
    will_return_always(__wrap_fpfw_timer_create, FPFW_STATUS_SUCCESS);
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);

    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    will_return_always(__wrap_set_ext_int_sub_system, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_update32, addr);
    expect_any_always(__wrap_mmio_update32, data);
    expect_any_always(__wrap_mmio_update32, mask);

    // FPFwCoreInterruptRegisterCallback
    expect_value_count(__wrap_virt_irq_set_isr_with_param, irq_num, CDEDSS_IRQ_NUMBER, 9);
    expect_value_count(__wrap_virt_irq_set_isr_with_param, isr, accel_intr_isr_scp, 9);
    expect_value_count(__wrap_virt_irq_set_isr_with_param, parameter, (void*)CDEDSS_IRQ_NUMBER, 9);
    will_return_always(__wrap_virt_irq_set_isr_with_param, NVIC_STATUS_SUCCESS);

    accel_intr_set_irq_num_for_accel(ACCEL_ID_CDED, CDEDSS_IRQ_NUMBER);

    assert_int_equal(accel_scp_intr_init(accel_intr_get_accel_type_from_irq_num(CDEDSS_IRQ_NUMBER)), ACCEL_INTR_RET_SUCCESS);
}

/**
 * @brief : Tests accel_mcp_intr_init with all passing on SDM
 */
TEST_FUNCTION(test_accel_mcp_intr_init_pass_sdm, nullptr, nullptr)
{
    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    will_return_always(__wrap_set_ext_int_sub_system, SILIBS_SUCCESS);
    will_return_always(__wrap_idsw_get_cpu_type, CPU_MCP);

    // FPFwCoreInterruptRegisterCallback
    expect_value_count(__wrap_virt_irq_set_isr_with_param, irq_num, SDMSS_IRQ_NUMBER, 1);
    expect_value_count(__wrap_virt_irq_set_isr_with_param, isr, accel_intr_isr_mcp, 1);
    expect_value_count(__wrap_virt_irq_set_isr_with_param, parameter, (void*)SDMSS_IRQ_NUMBER, 1);
    will_return_always(__wrap_virt_irq_set_isr_with_param, NVIC_STATUS_SUCCESS);

    assert_int_equal(accel_mcp_intr_init(accel_intr_get_accel_type_from_irq_num(SDMSS_IRQ_NUMBER)), ACCEL_INTR_RET_SUCCESS);
}

/**
 * @brief : Tests accel_mcp_intr_init with all passing on CDED
 */
TEST_FUNCTION(test_accel_mcp_intr_init_pass_cded, nullptr, nullptr)
{
    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    will_return_always(__wrap_set_ext_int_sub_system, SILIBS_SUCCESS);
    will_return_always(__wrap_idsw_get_cpu_type, CPU_MCP);

    // FPFwCoreInterruptRegisterCallback using virt irq
    expect_value(__wrap_virt_irq_set_isr_with_param, irq_num, CDEDSS_IRQ_NUMBER);
    expect_value(__wrap_virt_irq_set_isr_with_param, isr, accel_intr_isr_mcp);
    expect_value(__wrap_virt_irq_set_isr_with_param, parameter, (void*)CDEDSS_IRQ_NUMBER);
    will_return_always(__wrap_virt_irq_set_isr_with_param, NVIC_STATUS_SUCCESS);

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

    assert_int_equal(accel_scp_intr_init(ACCEL_ID_SDM), ACCEL_INTR_RET_FAIL_TIMER_CREATE);
}

/**
 * @brief : Tests accel_scp_intr_init with failure in nvic init
 */
TEST_FUNCTION(test_accel_scp_intr_init_fail_virt_irq_init, nullptr, nullptr)
{
    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    expect_any(__wrap_fpfw_timer_create, timer);
    expect_any(__wrap_fpfw_timer_create, cb);
    will_return_always(__wrap_fpfw_timer_create, FPFW_STATUS_SUCCESS);
    will_return_always(__wrap_idsw_get_cpu_type, CPU_SCP);

    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_set_ext_int_sub_system, SILIBS_SUCCESS);

    // FPFwCoreInterruptRegisterCallback
    expect_value_count(__wrap_virt_irq_set_isr_with_param, irq_num, SDMSS_IRQ_NUMBER, 9);
    expect_value_count(__wrap_virt_irq_set_isr_with_param, isr, accel_intr_isr_scp, 9);
    expect_value_count(__wrap_virt_irq_set_isr_with_param, parameter, (void*)SDMSS_IRQ_NUMBER, 9);
    will_return_always(__wrap_virt_irq_set_isr_with_param, NVIC_STATUS_ERROR);

    accel_scp_intr_init(accel_intr_get_accel_type_from_irq_num(SDMSS_IRQ_NUMBER));
}

/**
 * @brief : Tests accel_scp_intr_init with failure in timer init
 */
TEST_FUNCTION(test_accel_scp_intr_init_fail_invalid_arg1, nullptr, nullptr)
{
    assert_int_equal(accel_scp_intr_init(NUM_VALID_ACCEL_ID), ACCEL_INTR_RET_FAIL_INTR_INIT);
}

/**
 * @brief : Validate accel_irq_scp_data & accel_irq_mcp_data for NULL init function
 */
TEST_FUNCTION(test_accel_irq_mscp_data_null_init, nullptr, nullptr)
{
    // SCP core
    for (uint32_t idx = ACCEL_SCP_INTR_EMCPU_WDT_ERR; idx < ACCEL_SCP_INTR_MAX; idx++)
    {
        assert_non_null(accel_irq_scp_data[idx].accel_irq_init_fn);
    }

    // MCP core
    for (uint32_t idx = ACCEL_MCP_INTR_FIRST; idx < ACCEL_MCP_INTR_MAX; idx++)
    {
        assert_non_null(accel_irq_mcp_data[idx].accel_irq_init_fn);
    }
}

/**
 * @brief : Tests accel_mcp_intr_init with failure in timer init
 */
TEST_FUNCTION(test_accel_mcp_intr_init_fail_invalid_arg1, nullptr, nullptr)
{
    assert_int_equal(accel_mcp_intr_init(NUM_VALID_ACCEL_ID), ACCEL_INTR_RET_FAIL_INTR_INIT);
}

/**
 * @brief : Tests accel_intr_isr_scp_pass with all passing and both FATAL and SDM_MSG interrupt present
 */
TEST_FUNCTION(test_accel_intr_isr_scp_pass, nullptr, nullptr)
{
    ACCEL_ID accel_type = accel_intr_get_accel_type_from_irq_num(irq_num);

    mmio_set_mock_data(0x12345678, 0xFFFFFFFF);
    mmio_set_mock_data(0xABCDEF12, 0x0);

    set_int_status(true);

    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    will_return_always(__wrap_is_sdm_ext_int_status_set, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return_always(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);

    expect_any_always(__wrap_mmio_read32, addr);
    expect_any_always(__wrap_mmio_write32, addr);
    expect_any_always(__wrap_mmio_write32, data);

    // accel_intr_mbox_isr()
    will_return(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    expect_value(__wrap_send_mailbox_async_request, accel_type, accel_type);
    expect_value(__wrap_send_fatal_intr_async_request, accel_type, accel_type);

    expect_value(__wrap_nvic_irq_disable, irq_num, irq_num);

    accel_intr_isr_scp((void*)irq_num);
}

/**
 * @brief : Tests accel_intr_isr_mcp_pass with all passing
 */
TEST_FUNCTION(test_accel_intr_isr_mcp_pass, nullptr, nullptr)
{
    ACCEL_ID accel_type = accel_intr_get_accel_type_from_irq_num(irq_num);

    mmio_set_mock_data(0x12345678, 0xFFFFFFFF);
    mmio_set_mock_data(0xABCDEF12, 0x0);

    set_int_status(true);

    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);
    will_return_always(__wrap_is_sdm_ext_int_status_set, SILIBS_SUCCESS);

    // accel_intr_mbox_isr()
    will_return(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    expect_value(__wrap_send_mailbox_async_request, accel_type, accel_type);

    accel_intr_isr_mcp((void*)irq_num);
}

/**
 * @brief : Tests accel_intr_isr_scp_pass with all passing but no interrupt at level 2
 */
TEST_FUNCTION(test_sdm_intr_isr_scp_pass_no_level2_intr, nullptr, nullptr)
{
    ACCEL_ID accel_type = accel_intr_get_accel_type_from_irq_num(irq_num);

    mmio_set_mock_data(0x12345678, 0xFFFFFFFF);
    mmio_set_mock_data(0xABCDEF12, 0x0);

    set_int_status(false);

    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    will_return_always(__wrap_is_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    will_return_always(__wrap_is_sdm_ext_int_status_set, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return_always(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_read32, addr);
    expect_any_always(__wrap_mmio_write32, addr);
    expect_any_always(__wrap_mmio_write32, data);

    expect_value(__wrap_send_fatal_intr_async_request, accel_type, accel_type);

    expect_value(__wrap_nvic_irq_disable, irq_num, irq_num);

    accel_intr_isr_scp((void*)irq_num);
}

/**
 * @brief : Tests accel_intr_isr_scp_pass with all passing but no valid interrupt
 */
TEST_FUNCTION(test_accel_intr_isr_scp_pass_no_interrupt, nullptr, nullptr)
{
    ACCEL_ID accel_type = accel_intr_get_accel_type_from_irq_num(irq_num);

    mmio_set_mock_data(0x12345678, 0x0);
    mmio_set_mock_data(0xABCDEF12, 0xFFFFFFFF);

    set_int_status(true);

    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    will_return_always(__wrap_is_sdm_ext_int_status_set, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return_always(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);

    expect_any_always(__wrap_mmio_read32, addr);

    // accel_intr_mbox_isr()
    will_return(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    expect_value(__wrap_send_mailbox_async_request, accel_type, accel_type);

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

    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    will_return_always(__wrap_is_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    will_return_always(__wrap_is_sdm_ext_int_status_set, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return_always(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_read32, addr);

    expect_any(__wrap_fpfw_timer_enable, timer);
    will_return_always(__wrap_fpfw_timer_enable, FPFW_STATUS_SUCCESS);

    accel_intr_handle_fatal_intr_recvd(ACCEL_ID_SDM);
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

    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    will_return_always(__wrap_is_sdm_ext_int_status_set, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return_always(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_read32, addr);

    expect_any(__wrap_fpfw_timer_enable, timer);
    will_return_always(__wrap_fpfw_timer_enable, FPFW_STATUS_SUCCESS);

    accel_intr_handle_fatal_intr_recvd(ACCEL_ID_CDED);
}

/**
 * @brief : Tests accel_intr_handle_fatal_intr_recvd with all passing and but no interrupt at level 2
 */
TEST_FUNCTION(test_accel_intr_handle_fatal_intr_recvd_pass_no_level2_intr, NULL, NULL)
{
    mmio_set_mock_data(0x12345678, 0xFFFFFFFF);
    mmio_set_mock_data(0xABCDEF12, 0x0);

    set_int_status(false);

    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    will_return_always(__wrap_is_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    will_return_always(__wrap_is_sdm_ext_int_status_set, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return_always(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    expect_any_always(__wrap_mmio_read32, addr);

    accel_intr_handle_fatal_intr_recvd(ACCEL_ID_SDM);
}

/**
 * @brief : Tests accel_intr_handle_fatal_intr_recvd with all passing but no valid interrupt present. Spurious interrupt.
 */
TEST_FUNCTION(test_accel_intr_handle_fatal_intr_recvd_pass_no_interrupt, NULL, NULL)
{
    mmio_set_mock_data(0x12345678, 0x0);
    mmio_set_mock_data(0xABCDEF12, 0x0);

    set_int_status(false);

    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    will_return_count(__wrap_idsw_get_cpu_type, CPU_MCP, 9);
    will_return_always(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return_always(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    will_return_always(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_read32, addr);
    expect_any_always(__wrap_mmio_update32, addr);
    expect_any_always(__wrap_mmio_update32, data);
    expect_any_always(__wrap_mmio_update32, mask);

    expect_value_count(__wrap_virt_irq_set_isr_with_param, irq_num, SDMSS_IRQ_NUMBER, 9);
    expect_value_count(__wrap_virt_irq_set_isr_with_param, isr, accel_intr_isr_mcp, 9);
    expect_value_count(__wrap_virt_irq_set_isr_with_param, parameter, (void*)SDMSS_IRQ_NUMBER, 9);
    will_return_always(__wrap_virt_irq_set_isr_with_param, NVIC_STATUS_SUCCESS);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_FPFwCoreInterruptEnableVector, irqnum, SDMSS_IRQ_NUMBER);
    will_return(__wrap_FPFwCoreInterruptEnableVector, NVIC_STATUS_SUCCESS);

    accel_intr_handle_fatal_intr_recvd(ACCEL_ID_SDM);
}

/**
 * @brief : Tests accel_intr_handle_sdm_msg_recvd_timeout : Check on SDM core
 */
TEST_FUNCTION(test_accel_intr_handle_sdm_msg_recv_timeout_count_sdm, NULL, NULL)
{
    accel_intr_crash_dump_collection_timer_data_t accel_intr_crash_dump_collection_timer_data = {};
    accel_intr_crash_dump_collection_timer_data.accel_type = ACCEL_ID_SDM;

    fpfw_timer_callback timer_cb = fpfw_mock_get_timer_cb();
    assert_non_null(timer_cb);

    will_return(__wrap_crash_dump_is_accel_cd_complete, true);
    expect_function_call(__wrap_crash_dump_bug_check_external);

    timer_cb(&accel_intr_crash_dump_collection_timer_data, 0x0);
}

/**
 * @brief : Tests accel_intr_handle_sdm_msg_recvd_timeout : Check on CDED core
 */
TEST_FUNCTION(test_accel_intr_handle_sdm_msg_recv_timeout_count_cded, NULL, NULL)
{
    accel_intr_crash_dump_collection_timer_data_t accel_intr_crash_dump_collection_timer_data = {};
    accel_intr_crash_dump_collection_timer_data.accel_type = ACCEL_ID_CDED;

    fpfw_timer_callback timer_cb = fpfw_mock_get_timer_cb();
    assert_non_null(timer_cb);

    will_return(__wrap_crash_dump_is_accel_cd_complete, true);
    expect_function_call(__wrap_crash_dump_bug_check_external);

    timer_cb(&accel_intr_crash_dump_collection_timer_data, 0x0);
}

/**
 * @brief : CD collection is not complete. Warm reset.
 */
TEST_FUNCTION(test_accel_intr_handle_sdm_msg_recv_timeout_count_cd_incomplete, NULL, NULL)
{
    accel_intr_crash_dump_collection_timer_data_t accel_intr_crash_dump_collection_timer_data = {};
    accel_intr_crash_dump_collection_timer_data.accel_type = ACCEL_ID_CDED;
    accel_intr_crash_dump_collection_timer_data.is_soc_reset = false;

    fpfw_timer_callback timer_cb = fpfw_mock_get_timer_cb();
    assert_non_null(timer_cb);

    will_return(__wrap_crash_dump_is_accel_cd_complete, false);
    expect_function_call(__wrap_crash_dump_bug_check_external);

    timer_cb(&accel_intr_crash_dump_collection_timer_data, 0x0);
}

/**
 * @brief : CD collection is not complete. SOC reset.
 */
TEST_FUNCTION(test_accel_intr_handle_sdm_msg_recv_timeout_count_cd_soc_reset, NULL, NULL)
{
    accel_intr_crash_dump_collection_timer_data_t accel_intr_crash_dump_collection_timer_data = {};
    accel_intr_crash_dump_collection_timer_data.accel_type = ACCEL_ID_CDED;
    accel_intr_crash_dump_collection_timer_data.is_soc_reset = true;

    fpfw_timer_callback timer_cb = fpfw_mock_get_timer_cb();
    assert_non_null(timer_cb);

    will_return(__wrap_crash_dump_is_accel_cd_complete, false);
    expect_function_call(__wrap_crash_dump_bug_check_external);

    timer_cb(&accel_intr_crash_dump_collection_timer_data, 0x0);
}

TEST_FUNCTION(test_accel_intr_handle_mbox_recvd_sdm__pass, NULL, NULL)
{
    void* cb_ctx = (void*)0x12345678;

    accel_intr_set_mbx_ctx(ACCEL_ID_SDM, (void*)cb_ctx);

    // ATU map address base is always 0
    will_return(__wrap_atu_svc_accel_atu_addr, 0x0);
    expect_value(__wrap_accel_mbox_sw_intr_cb, ctx, cb_ctx);
    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    will_return(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);

    accel_intr_handle_mbox_recvd(ACCEL_ID_SDM);
}

TEST_FUNCTION(test_accel_intr_handle_mbox_recvd_sdm__fail1, NULL, NULL)
{
    accel_intr_set_mbx_ctx(ACCEL_ID_SDM, (void*)NULL);

    accel_intr_handle_mbox_recvd(ACCEL_ID_SDM);
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

    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    // accel_intr_process_fatal_interrupts()
    will_return_count(__wrap_idsw_get_cpu_type, CPU_MCP, 9);
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

    expect_value_count(__wrap_virt_irq_set_isr_with_param, irq_num, SDMSS_IRQ_NUMBER, 9);
    expect_value_count(__wrap_virt_irq_set_isr_with_param, isr, accel_intr_isr_mcp, 9);
    expect_value_count(__wrap_virt_irq_set_isr_with_param, parameter, (void*)SDMSS_IRQ_NUMBER, 9);
    will_return_always(__wrap_virt_irq_set_isr_with_param, NVIC_STATUS_SUCCESS);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_FPFwCoreInterruptEnableVector, irqnum, SDMSS_IRQ_NUMBER);
    will_return(__wrap_FPFwCoreInterruptEnableVector, NVIC_STATUS_SUCCESS);

    accel_intr_handle_fatal_intr_recvd(ACCEL_ID_SDM);
}

/**
 * @brief : CDED receive CDED CP fatal error IC_XB_PERR
 */
TEST_FUNCTION(test_cded_intr_handle_cded_cp_fatal_intr_recvd__pass2, NULL, NULL)
{
    uint32_t coproc_apb_addr = CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS;
    uint32_t cded_cfg_fatal_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_CDED_CFG_FATAL, coproc_apb_addr);
    mmio_set_mock_data(0x12345678, 1 << SDM_EXT_CP_FATAL_ERR_INTR);
    mmio_set_mock_data(0xABCDEF12, 0x0);
    mmio_set_mock_data(cded_cfg_fatal_offset, 1 << IC_XB_PERR);

    set_int_status(false);

    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    // accel_intr_process_fatal_interrupts()
    will_return(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    expect_any_always(__wrap_mmio_read32, addr);

    // accel_intr_cded_cp_err_fn()
    will_return(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    //  handle_cded_cp_level2_intr()
    will_return(__wrap_cded_int_mask_disable, SILIBS_SUCCESS);

    accel_intr_handle_fatal_intr_recvd(ACCEL_ID_CDED);
}

/**
 * @brief : CDED receive CDED CP fatal error CCMP_FATAL_INT
 */
TEST_FUNCTION(test_cded_intr_handle_cded_cp_fatal_intr_recvd__pass3, NULL, NULL)
{
    uint32_t coproc_apb_addr = CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS;
    uint32_t cded_cfg_fatal_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_CDED_CFG_FATAL, coproc_apb_addr);
    uint32_t cded_cfg_ccmp0_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_COMPRESSOR0_FATAL, coproc_apb_addr);
    mmio_set_mock_data(0x12345678, 1 << SDM_EXT_CP_FATAL_ERR_INTR);
    mmio_set_mock_data(0xABCDEF12, 0x0);
    mmio_set_mock_data(cded_cfg_fatal_offset, 1 << CCMP_FATAL_INT);
    mmio_set_mock_data(cded_cfg_ccmp0_offset, 1 << CCMP_IB_PERR);

    set_int_status(false);

    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    // accel_intr_process_fatal_interrupts()
    will_return(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    expect_any_always(__wrap_mmio_read32, addr);

    // accel_intr_cded_cp_err_fn()
    will_return(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    //  handle_cded_cp_level2_intr()
    will_return(__wrap_cded_int_mask_disable, SILIBS_SUCCESS);

    accel_intr_handle_fatal_intr_recvd(ACCEL_ID_CDED);
}

/**
 * @brief : CDED receive CDED CP fatal error DCMP_FATAL_INT
 */
TEST_FUNCTION(test_cded_intr_handle_cded_cp_fatal_intr_recvd__pass4, NULL, NULL)
{
    uint32_t coproc_apb_addr = CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS;
    uint32_t cded_cfg_fatal_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_CDED_CFG_FATAL, coproc_apb_addr);
    uint32_t cded_cfg_dcmp0_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_DECOMPRESSOR0_FATAL, coproc_apb_addr);
    mmio_set_mock_data(0x12345678, 1 << SDM_EXT_CP_FATAL_ERR_INTR);
    mmio_set_mock_data(0xABCDEF12, 0x0);
    mmio_set_mock_data(cded_cfg_fatal_offset, 1 << DCMP_FATAL_INT);
    mmio_set_mock_data(cded_cfg_dcmp0_offset, 1 << DCMP_IB_PERR);

    set_int_status(false);

    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    // accel_intr_process_fatal_interrupts()
    will_return(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    expect_any_always(__wrap_mmio_read32, addr);

    // accel_intr_cded_cp_err_fn()
    will_return(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    //  handle_cded_cp_level2_intr()
    will_return(__wrap_cded_int_mask_disable, SILIBS_SUCCESS);

    accel_intr_handle_fatal_intr_recvd(ACCEL_ID_CDED);
}

/**
 * @brief : CDED receive CDED CP fatal error AES_FATAL_INT
 */
TEST_FUNCTION(test_cded_intr_handle_cded_cp_fatal_intr_recvd__pass5, NULL, NULL)
{
    uint32_t coproc_apb_addr = CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS;
    uint32_t cded_cfg_fatal_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_CDED_CFG_FATAL, coproc_apb_addr);
    uint32_t cded_cfg_aes0_offset = cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_AES_FATAL, coproc_apb_addr);
    mmio_set_mock_data(0x12345678, 1 << SDM_EXT_CP_FATAL_ERR_INTR);
    mmio_set_mock_data(0xABCDEF12, 0x0);
    mmio_set_mock_data(cded_cfg_fatal_offset, 1 << AES_FATAL_INT);
    mmio_set_mock_data(cded_cfg_aes0_offset, 1 << CDED_AES_PL_PERR);

    set_int_status(false);

    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    // accel_intr_process_fatal_interrupts()
    will_return(__wrap_sdm_ext_get_category_mask_reg_addr, 0xABCDEF12);
    will_return(__wrap_sdm_ext_get_category_status_reg_addr, 0x12345678);
    expect_any_always(__wrap_mmio_read32, addr);

    // accel_intr_cded_cp_err_fn()
    will_return(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    //  handle_cded_cp_level2_intr()
    will_return(__wrap_cded_int_mask_disable, SILIBS_SUCCESS);

    accel_intr_handle_fatal_intr_recvd(ACCEL_ID_CDED);
}

/**
 * @brief : CDED receive CDED CP fatal error
 * level 2 status register is zero. Spurious interrupt
 */
TEST_FUNCTION(test_cded_intr_handle_cded_cp_fatal_intr_recvd__fail1, NULL, NULL)
{
    uint32_t coproc_apb_addr = CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS;
    uint32_t cded_cfg_fatal_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_CDED_CFG_FATAL, coproc_apb_addr);
    mmio_set_mock_data(0x12345678, 1 << SDM_EXT_CP_FATAL_ERR_INTR);
    mmio_set_mock_data(0xABCDEF12, 0x0);
    mmio_set_mock_data(cded_cfg_fatal_offset, 0x0);

    set_int_status(false);

    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    // accel_intr_process_fatal_interrupts()
    will_return_count(__wrap_idsw_get_cpu_type, CPU_MCP, 9);
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

    expect_value_count(__wrap_virt_irq_set_isr_with_param, irq_num, CDEDSS_IRQ_NUMBER, 9);
    expect_value_count(__wrap_virt_irq_set_isr_with_param, isr, accel_intr_isr_mcp, 9);
    expect_value_count(__wrap_virt_irq_set_isr_with_param, parameter, (void*)CDEDSS_IRQ_NUMBER, 9);
    will_return_always(__wrap_virt_irq_set_isr_with_param, NVIC_STATUS_SUCCESS);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_FPFwCoreInterruptEnableVector, irqnum, CDEDSS_IRQ_NUMBER);
    will_return(__wrap_FPFwCoreInterruptEnableVector, NVIC_STATUS_SUCCESS);

    accel_intr_handle_fatal_intr_recvd(ACCEL_ID_CDED);
}

/**
 * @brief : CDED receive CDED CP fatal error
 * level 3 status register is zero. Spurious interrupt
 */
TEST_FUNCTION(test_cded_intr_handle_cded_cp_fatal_intr_recvd__fail2, NULL, NULL)
{
    uint32_t coproc_apb_addr = CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS;
    uint32_t cded_cfg_fatal_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_CDED_CFG_FATAL, coproc_apb_addr);
    uint32_t cded_cfg_ccmp0_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_COMPRESSOR0_FATAL, coproc_apb_addr);
    mmio_set_mock_data(0x12345678, 1 << SDM_EXT_CP_FATAL_ERR_INTR);
    mmio_set_mock_data(0xABCDEF12, 0x0);
    mmio_set_mock_data(cded_cfg_fatal_offset, 1 << CCMP_FATAL_INT);
    mmio_set_mock_data(cded_cfg_ccmp0_offset, 0x0);

    set_int_status(false);

    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    will_return_count(__wrap_idsw_get_cpu_type, CPU_MCP, 9);
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

    expect_value_count(__wrap_virt_irq_set_isr_with_param, irq_num, CDEDSS_IRQ_NUMBER, 9);
    expect_value_count(__wrap_virt_irq_set_isr_with_param, isr, accel_intr_isr_mcp, 9);
    expect_value_count(__wrap_virt_irq_set_isr_with_param, parameter, (void*)CDEDSS_IRQ_NUMBER, 9);
    will_return_always(__wrap_virt_irq_set_isr_with_param, NVIC_STATUS_SUCCESS);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_FPFwCoreInterruptEnableVector, irqnum, CDEDSS_IRQ_NUMBER);
    will_return(__wrap_FPFwCoreInterruptEnableVector, NVIC_STATUS_SUCCESS);

    accel_intr_handle_fatal_intr_recvd(ACCEL_ID_CDED);
}

/**
 * @brief : CDED receive CDED CP fatal error
 * level 3 status register is zero. Spurious interrupt
 */
TEST_FUNCTION(test_cded_intr_handle_cded_cp_fatal_intr_recvd__fail3, NULL, NULL)
{
    uint32_t coproc_apb_addr = CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS;
    uint32_t cded_cfg_fatal_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_CDED_CFG_FATAL, coproc_apb_addr);
    uint32_t cded_cfg_dcmp0_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_DECOMPRESSOR0_FATAL, coproc_apb_addr);
    mmio_set_mock_data(0x12345678, 1 << SDM_EXT_CP_FATAL_ERR_INTR);
    mmio_set_mock_data(0xABCDEF12, 0x0);
    mmio_set_mock_data(cded_cfg_fatal_offset, 1 << DCMP_FATAL_INT);
    mmio_set_mock_data(cded_cfg_dcmp0_offset, 0x0);

    set_int_status(false);

    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    will_return_count(__wrap_idsw_get_cpu_type, CPU_MCP, 9);
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

    expect_value_count(__wrap_virt_irq_set_isr_with_param, irq_num, CDEDSS_IRQ_NUMBER, 9);
    expect_value_count(__wrap_virt_irq_set_isr_with_param, isr, accel_intr_isr_mcp, 9);
    expect_value_count(__wrap_virt_irq_set_isr_with_param, parameter, (void*)CDEDSS_IRQ_NUMBER, 9);
    will_return_always(__wrap_virt_irq_set_isr_with_param, NVIC_STATUS_SUCCESS);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_FPFwCoreInterruptEnableVector, irqnum, CDEDSS_IRQ_NUMBER);
    will_return(__wrap_FPFwCoreInterruptEnableVector, NVIC_STATUS_SUCCESS);

    accel_intr_handle_fatal_intr_recvd(ACCEL_ID_CDED);
}

/**
 * @brief : CDED receive CDED CP fatal error
 * level 3 status register is zero. Spurious interrupt
 */
TEST_FUNCTION(test_cded_intr_handle_cded_cp_fatal_intr_recvd__fail4, NULL, NULL)
{
    uint32_t coproc_apb_addr = CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS;
    uint32_t cded_cfg_fatal_offset =
        cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_CDED_CFG_FATAL, coproc_apb_addr);
    uint32_t cded_cfg_aes_offset = cded_int_get_category_status_reg_addr(CDED_CATEGORY_ID_AES_FATAL, coproc_apb_addr);
    mmio_set_mock_data(0x12345678, 1 << SDM_EXT_CP_FATAL_ERR_INTR);
    mmio_set_mock_data(0xABCDEF12, 0x0);
    mmio_set_mock_data(cded_cfg_fatal_offset, 1 << AES_FATAL_INT);
    mmio_set_mock_data(cded_cfg_aes_offset, 0x0);

    set_int_status(false);

    // ATU map address base is always 0
    will_return_always(__wrap_atu_svc_accel_atu_addr, 0x0);

    will_return_count(__wrap_idsw_get_cpu_type, CPU_MCP, 9);
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

    expect_value_count(__wrap_virt_irq_set_isr_with_param, irq_num, CDEDSS_IRQ_NUMBER, 9);
    expect_value_count(__wrap_virt_irq_set_isr_with_param, isr, accel_intr_isr_mcp, 9);
    expect_value_count(__wrap_virt_irq_set_isr_with_param, parameter, (void*)CDEDSS_IRQ_NUMBER, 9);
    will_return_always(__wrap_virt_irq_set_isr_with_param, NVIC_STATUS_SUCCESS);

    // FPFwCoreInterruptEnableVector
    expect_value(__wrap_FPFwCoreInterruptEnableVector, irqnum, CDEDSS_IRQ_NUMBER);
    will_return(__wrap_FPFwCoreInterruptEnableVector, NVIC_STATUS_SUCCESS);

    accel_intr_handle_fatal_intr_recvd(ACCEL_ID_CDED);
}
