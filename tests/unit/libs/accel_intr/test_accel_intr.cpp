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
#include <accelerator_knobs.h> // for scp_download_accel_knobs
#include <accelip_id.h>        // for ACCEL_ID_CDED, ACCEL_ID_SDM
#include <bitops.h>            // for SL_GET_SINGLE_BIT_MASK, SL_GET_BIT_MASK_RANGE
#include <cdedss_config_regs.h>
#include <idsw.h>             // for CPU_SCP / CPU_MCP types
#include <idsw_kng.h>         // for KNG_PLAT_ID
#include <nvic.h>             // for NVIC_STATUS_SUCCESS
#include <sdm_ext_cfg_regs.h> // for SDM_EXT_INVALID_INTERRUPT_INPUT and ext interrupt macros
#include <silibs_status.h>    // for SILIBS_SUCCESS
#include <stdint.h>           // for uint32_t
#include <stdio.h>
#include <virt_irq.h> // for virt irq macros

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

// static uint32_t irq_num = SDMSS_IRQ_NUMBER;

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

uint32_t __wrap_crash_dump_transfer_accel_cd_to_BMC(ACCEL_ID accel_type)
{
    check_expected(accel_type);
    function_called();

    return 0; // Return success for the mock
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

void __wrap_accel_core_warm_reset(ACCEL_ID accel_type)
{
    check_expected(accel_type);
    function_called();
}

knob_transfer_status_t __wrap_scp_download_accel_knobs(ACCEL_ID accel_type)
{
    check_expected(accel_type);

    // This is a mock function, so we don't actually download anything
    return mock_type(knob_transfer_status_t);
}

bool __wrap_system_info_is_warm_start()
{
    return mock_type(bool);
}

} // extern "C"

/*****************************
 * Additional Mocks Required
 *****************************/
extern "C" {

// Timer init mock
int32_t __wrap_accel_intr_crash_dump_collection_timer_init(ACCEL_ID accel_type)
{
    FPFW_UNUSED(accel_type);
    return (int32_t)mock_type(uint32_t);
}

// MMIO read/write & barrier
uint32_t __wrap_MMIO_READ32(uint32_t addr)
{
    FPFW_UNUSED(addr);
    return mock_type(uint32_t);
}

void __wrap_MMIO_WRITE32(uint32_t addr, uint32_t value)
{
    check_expected(addr);
    check_expected(value);
}

void __wrap_cortex_m7_atomic_call_data_memory_barrier(void)
{
    function_called();
}

void __wrap_critical_print(const char* fmt, ...)
{
    FPFW_UNUSED(fmt);
    function_called();
}

bool __wrap_hm_collect_accel_fatal_cper(uint32_t accel_id)
{
    FPFW_UNUSED(accel_id);
    return mock_type(bool);
}

// Helper functions
static int irq_init_setup_fn(void** state)
{
    FPFW_UNUSED(state);
    accel_intr_set_irq_num_for_accel(ACCEL_ID_SDM, SDMSS_IRQ_NUMBER);
    accel_intr_set_irq_num_for_accel(ACCEL_ID_CDED, CDEDSS_IRQ_NUMBER);
    return 0;
}

static void accel_init_func(ACCEL_ID accel_id)
{
    will_return(__wrap_atu_svc_accel_atu_addr, 0x0);

    expect_any(__wrap_fpfw_timer_create, timer);
    expect_any(__wrap_fpfw_timer_create, cb);

    will_return(__wrap_fpfw_timer_create, FPFW_STATUS_SUCCESS);

    will_return(__wrap_set_ext_int_sub_system, SILIBS_SUCCESS);
    will_return(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    will_return(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);

    expect_value(__wrap_FPFwCoreInterruptEnableVector, irqnum, 0);
    will_return(__wrap_FPFwCoreInterruptEnableVector, NVIC_STATUS_SUCCESS);

    fpfw_mock_set_active_accel_type(accel_id);

    int32_t ret = accel_scp_intr_init(accel_id);
    assert_int_equal(ret, ACCEL_INTR_RET_SUCCESS);
}

} // extern "C"

TEST_FUNCTION(accel_scp_intr_init, nullptr, nullptr)
{
    accel_init_func(ACCEL_ID_SDM);
    accel_init_func(ACCEL_ID_CDED);
}

TEST_FUNCTION(accel_scp_err_intr_init, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);

    expect_any_always(__wrap_FPFwCoreInterruptRegisterCallback, handler);
    expect_any_always(__wrap_FPFwCoreInterruptRegisterCallback, arg);
    expect_any_always(__wrap_FPFwCoreInterruptRegisterCallback, irqnum);
    will_return_always(__wrap_FPFwCoreInterruptRegisterCallback, NVIC_STATUS_SUCCESS);

    expect_any_always(__wrap_FPFwCoreInterruptEnableVector, irqnum);
    will_return_always(__wrap_FPFwCoreInterruptEnableVector, NVIC_STATUS_SUCCESS);

    accel_intr_scp_err_intr_enable(ACCEL_ID_SDM);
}

TEST_FUNCTION(accel_mcp_intr_init, nullptr, nullptr)
{
    will_return(__wrap_atu_svc_accel_atu_addr, 0x0);

    will_return(__wrap_set_ext_int_sub_system, SILIBS_SUCCESS);
    will_return(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);

    expect_any_always(__wrap_FPFwCoreInterruptEnableVector, irqnum);
    will_return_always(__wrap_FPFwCoreInterruptEnableVector, NVIC_STATUS_SUCCESS);

    accel_mcp_intr_init(ACCEL_ID_SDM);
}

// CPER collection failure path
// Iterate 4 times
TEST_FUNCTION(test_accel_fatal_cb_cper_collect_fail, nullptr, nullptr)
{
    fpfw_timer_callback cb = fpfw_mock_get_timer_cb(ACCEL_ID_SDM);
    void* ctx = fpfw_mock_get_timer_ctx(ACCEL_ID_SDM);

    will_return_always(__wrap_sdm_ext_get_category_status_reg_addr, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_get_category_mask_reg_addr, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, 0x1);        // WDT Error
    will_return(__wrap_mmio_read32, 0xFFFFFFFE); // Inverted IRQ mask

    will_return_always(__wrap_hm_collect_accel_fatal_cper, false);

    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);

    expect_function_call(__wrap_crash_dump_bug_check_external);

    expect_any_always(__wrap_fpfw_timer_enable, timer);
    will_return_always(__wrap_fpfw_timer_enable, FPFW_STATUS_SUCCESS);

    cb(ctx, NULL);

    will_return(__wrap_mmio_read32, 0x1);        // WDT Error
    will_return(__wrap_mmio_read32, 0xFFFFFFFE); // Inverted IRQ mask

    cb(ctx, NULL);

    will_return(__wrap_mmio_read32, 0x1);        // WDT Error
    will_return(__wrap_mmio_read32, 0xFFFFFFFE); // Inverted IRQ mask

    cb(ctx, NULL);

    will_return(__wrap_mmio_read32, 0x1);        // WDT Error
    will_return(__wrap_mmio_read32, 0xFFFFFFFE); // Inverted IRQ mask

    cb(ctx, NULL);

    will_return(__wrap_mmio_read32, 0x1);        // WDT Error
    will_return(__wrap_mmio_read32, 0xFFFFFFFE); // Inverted IRQ mask

    cb(ctx, NULL);
}

TEST_FUNCTION(test_accel_fatal_cb, nullptr, nullptr)
{
    fpfw_timer_callback cb = fpfw_mock_get_timer_cb(ACCEL_ID_CDED);
    void* ctx = fpfw_mock_get_timer_ctx(ACCEL_ID_CDED);

    will_return(__wrap_sdm_ext_get_category_status_reg_addr, SILIBS_SUCCESS);
    will_return(__wrap_sdm_ext_get_category_mask_reg_addr, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, 0x1);        // WDT Error
    will_return(__wrap_mmio_read32, 0xFFFFFFFE); // Inverted IRQ mask

    will_return(__wrap_hm_collect_accel_fatal_cper, true);

    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);

    expect_function_call(__wrap_crash_dump_bug_check_external);

    cb(ctx, NULL);
}

// TCM UE error detected in the cb path
TEST_FUNCTION(test_accel_fatal_cb_tcm_ue, nullptr, nullptr)
{
    fpfw_timer_callback cb = fpfw_mock_get_timer_cb(ACCEL_ID_SDM);
    void* ctx = fpfw_mock_get_timer_ctx(ACCEL_ID_SDM);

    will_return_always(__wrap_sdm_ext_get_category_status_reg_addr, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_get_category_mask_reg_addr, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, 0x8000);     // TCM UE error
    will_return(__wrap_mmio_read32, 0xFFFF7FFF); // Inverted IRQ mask

    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);

    expect_function_call(__wrap_crash_dump_bug_check_external);

    cb(ctx, NULL);
}

// Doorbell IRQ (IP fatal) error detected in the cb path
TEST_FUNCTION(test_accel_fatal_cb_doorbell, nullptr, nullptr)
{
    fpfw_timer_callback cb = fpfw_mock_get_timer_cb(ACCEL_ID_SDM);
    void* ctx = fpfw_mock_get_timer_ctx(ACCEL_ID_SDM);

    will_return_always(__wrap_sdm_ext_get_category_status_reg_addr, SILIBS_SUCCESS);
    will_return_always(__wrap_sdm_ext_get_category_mask_reg_addr, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, 0x2000000); // TCM UE error
    will_return(__wrap_mmio_read32, 0xDFFFFFF); // Inverted IRQ mask

    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);

    expect_function_call(__wrap_crash_dump_bug_check_external);

    cb(ctx, NULL);
}

/**
 * This test needs to be called after callback testing
 * as the tcm_ue error will set CPER and CD skip flags to true
 */
TEST_FUNCTION(test_accel_isrs, irq_init_setup_fn, nullptr)
{
    // The TCM UE ISR invokes the fatal isr, hence enough to write only one test
    uint32_t virt_irq = GET_VIRTUAL_IRQ(SDMSS_IRQ_NUMBER, 0, 0);

    will_return(__wrap_atu_svc_accel_atu_addr, 0x0);

    expect_any(__wrap_nvic_irq_disable, irq_num);

    expect_any_always(__wrap_mmio_write32, addr);
    expect_any_always(__wrap_mmio_write32, data);

    expect_any(__wrap_send_fatal_intr_async_request, accel_type);

    accel_intr_tcm_ue_emcpu_lockcup_isr((void*)virt_irq);
}

// This should test the CD and CPER skip paths for the callback
TEST_FUNCTION(test_accel_fatal_cb_cd_cper_skip, nullptr, nullptr)
{
    fpfw_timer_callback cb = fpfw_mock_get_timer_cb(ACCEL_ID_SDM);
    void* ctx = fpfw_mock_get_timer_ctx(ACCEL_ID_SDM);

    will_return(__wrap_sdm_ext_get_category_status_reg_addr, SILIBS_SUCCESS);
    will_return(__wrap_sdm_ext_get_category_mask_reg_addr, SILIBS_SUCCESS);

    expect_any_always(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, 0x1);        // WDT Error
    will_return(__wrap_mmio_read32, 0xFFFFFFFE); // Intr mask

    will_return_always(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);

    expect_function_call(__wrap_crash_dump_bug_check_external);

    cb(ctx, NULL);
}

TEST_FUNCTION(test_accel_fatal_err_received, nullptr, nullptr)
{
    will_return(__wrap_atu_svc_accel_atu_addr, 0x0);

    // Simulate fatal error received for SDM
    expect_any(__wrap_fpfw_timer_enable, timer);
    will_return(__wrap_fpfw_timer_enable, FPFW_STATUS_SUCCESS);

    accel_intr_handle_fatal_intr_recvd(ACCEL_ID_SDM);
}

TEST_FUNCTION(test_accel_get_cd_skip, nullptr, nullptr)
{
    accel_intr_get_cd_skip(ACCEL_ID_SDM);
}

TEST_FUNCTION(test_accel_cd_timer_init_fail, nullptr, nullptr)
{
    expect_any(__wrap_fpfw_timer_create, timer);
    expect_any(__wrap_fpfw_timer_create, cb);

    will_return(__wrap_fpfw_timer_create, FPFW_STATUS_FAIL);

    accel_intr_crash_dump_collection_timer_init(ACCEL_ID_SDM);
}

TEST_FUNCTION(test_accel_scp_intr_init_invalid_id, nullptr, nullptr)
{
    int32_t ret = accel_scp_intr_init(NUM_VALID_ACCEL_ID);
    assert_int_equal(ret, ACCEL_INTR_RET_FAIL_INTR_INIT);
}

TEST_FUNCTION(test_accel_scp_intr_timer_create_fail, nullptr, nullptr)
{
    will_return(__wrap_atu_svc_accel_atu_addr, 0x0);

    expect_any(__wrap_fpfw_timer_create, timer);
    expect_any(__wrap_fpfw_timer_create, cb);

    will_return(__wrap_fpfw_timer_create, FPFW_STATUS_FAIL);

    int32_t ret = accel_scp_intr_init(ACCEL_ID_SDM);
    assert_int_equal(ret, ACCEL_INTR_RET_FAIL_TIMER_CREATE);
}

TEST_FUNCTION(test_accel_mcp_intr_init_invalid_id, nullptr, nullptr)
{
    int32_t ret = accel_mcp_intr_init(NUM_VALID_ACCEL_ID);
    assert_int_equal(ret, ACCEL_INTR_RET_FAIL_INTR_INIT);
}

TEST_FUNCTION(test_accel_helper_functions, nullptr, nullptr)
{
    will_return(__wrap_sdm_ext_int_mask_enable, SILIBS_SUCCESS);
    accel_intr_unmask_interrupt_level_1(0, (SDM_EXT_INTERRUPT_NUMBER)0);

    will_return(__wrap_sdm_ext_int_mask_disable, SILIBS_SUCCESS);
    accel_intr_mask_interrupt_level_1(0, (SDM_EXT_INTERRUPT_NUMBER)0);

    will_return(__wrap_sdm_ext_int_mask_status_clear, SILIBS_SUCCESS);
    accel_intr_clear_interrupt_level_1(0, (SDM_EXT_INTERRUPT_NUMBER)0);

    will_return(__wrap_sdm_ext_get_category_status_reg_addr, 0x40bd30c8);
    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, 0x1);

    accel_intr_get_interrupt_status_data(0, SDM_EXT_CATEGORY_ID_EXT_INTR, 0, 0);
}

TEST_FUNCTION(test_accel_cover_domain_helper_api1, nullptr, nullptr)
{
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);

    expect_any_always(__wrap_FPFwCoreInterruptRegisterCallback, handler);
    expect_any_always(__wrap_FPFwCoreInterruptRegisterCallback, arg);
    expect_any_always(__wrap_FPFwCoreInterruptRegisterCallback, irqnum);
    will_return_always(__wrap_FPFwCoreInterruptRegisterCallback, NVIC_STATUS_SUCCESS);

    expect_any_always(__wrap_FPFwCoreInterruptEnableVector, irqnum);
    will_return_always(__wrap_FPFwCoreInterruptEnableVector, NVIC_STATUS_SUCCESS);

    accel_intr_scp_err_intr_enable(ACCEL_ID_CDED);

    will_return_count(__wrap_idsw_get_cpu_type, CPU_MCP, 2);

    accel_intr_scp_err_intr_enable(ACCEL_ID_SDM);
    accel_intr_scp_err_intr_enable(ACCEL_ID_CDED);
}
