//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 @file test_err_domain_register_mcp_init.cpp
 * Tests the init of the smcp error domain
 */
/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {

#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_init.h> // for fpfw_init_result_t, fpfw_init_component_t
#include <health_monitor.h>
#include <health_monitor_icc.h>
#include <interrupts.h>
#include <mscp_error_domain.h>
#include <nvic.h> // for nvic_status_t
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <mcp_top_regs.h>
#include <scp_top_regs.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/
#define DTC_RAM_ADDRESS (MCP_TOP_MCP_DATA_RAM_ADDRESS)
#define MCP_TCM_ERRCTRL_REG \
    (MCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_ADDRESS)
#define MCP_TCM_ERRCTRL_REG \
    (MCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_ADDRESS)
#define MCP_TCM_ERRSTATUS_REG \
    (MCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_ADDRESS)
#define MCP_TCM_ERRADDRESS_REG \
    (MCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRADDR_ADDRESS)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void test_hard_fault_handler();
void test_bus_fault_handler();
void test_watchdog_handler();

/*-- Declarations (Statics and globals) --*/
hm_error_injection_cb_t g_err_inject_cb = NULL;
isr_callback_fn_sans_params_t g_tcm_ce_isr = NULL;
isr_callback_fn_sans_params_t g_tcm_ue_isr = NULL;
isr_callback_fn_sans_params_t g_tcm_of_isr = NULL;
mcp_proc_err_type_t g_last_mcp_err_type = MCP_ERROR_TYPE_COUNT;
ras_einj_info_t ras_einj_payload = {};

/*------------- Functions ----------------*/
//
// Mocks
//
fpfw_status_t __wrap_fpfw_icc_base_send(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_send_req_t* params)
{
    FPFW_UNUSED(icc_ctx);

    assert_true(params != NULL);
    assert_true(params->payload_buffer != NULL);
    assert_true(params->buffer_size == sizeof(hm_mhu_error_domain_register_payload_t));

    params->cb(params, FPFW_ICC_BASE_STATUS_SUCCESS);

    function_called();

    return (mock_type(fpfw_status_t));
}

fpfw_status_t __wrap_fpfw_icc_base_recv(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_recv_req_t* params)
{
    FPFW_UNUSED(icc_ctx);
    assert_true(params != NULL);
    assert_true(params->payload_buffer != NULL);
    assert_true(params->recv_cmd_code == ICC_HM_ERROR_INJECTION_MCP);

    if (params->recv_cmd_code == ICC_HM_ERROR_INJECTION_MCP &&
        (g_last_mcp_err_type != ras_einj_payload.param_type.error_type))
    {
        hm_mhu_error_injection_payload_t einj_payload;
        einj_payload.header.msg_header.command = params->recv_cmd_code;
        einj_payload.error_injection_info.param_type.error_type =
            (mcp_proc_err_type_t)ras_einj_payload.param_type.error_type;
        g_last_mcp_err_type = (mcp_proc_err_type_t)ras_einj_payload.param_type.error_type;
        params->cb(&einj_payload, sizeof(einj_payload), FPFW_ICC_BASE_STATUS_SUCCESS);
    }

    function_called();
    return (mock_type(fpfw_status_t));
}

uint32_t __wrap_mmio_read32(volatile uint32_t* addr)
{
    check_expected(addr);
    function_called();
    return mock_type(uint32_t);
}

void __wrap_mmio_write32(volatile uint32_t* addr, uint32_t data)
{
    check_expected(addr);
    check_expected(data);
    function_called();
}

nvic_status_t __wrap_nvic_irq_enable(uint32_t irq_num)
{
    check_expected(irq_num);
    function_called();
    return NVIC_STATUS_SUCCESS;
}

nvic_status_t __wrap_nvic_irq_set_isr(uint32_t irq_num, isr_callback_fn_sans_params_t isr)
{
    assert_non_null(isr);

    switch (irq_num)
    {
    case HW_INT_TCM_ECCCE_INT:
        g_tcm_ce_isr = isr;
        break;
    case HW_INT_TCM_ECCUE_INT:
        g_tcm_ue_isr = isr;
        break;
    case HW_INT_TCM_ECCOF_INT:
        g_tcm_of_isr = isr;
        break;
    case MCP_ERROR_TYPE_WATCHDOG:
        test_watchdog_handler();
        break;
    case MCP_ERROR_TYPE_BUS_FAULT:
        test_bus_fault_handler();
        break;
    default:
        assert_true(false);
        break;
    }

    function_called();
    return NVIC_STATUS_SUCCESS;
}

nvic_status_t __wrap_nvic_irq_clear_pending(uint32_t irq_num)
{
    check_expected(irq_num);
    function_called();
    return NVIC_STATUS_SUCCESS;
}

void __wrap_hm_submit_cper(uint16_t error_domain_idx, acpi_error_severity_t err_severity, void* err_record_section, uint32_t err_record_section_size)
{
    assert_true(error_domain_idx == ACPI_ERROR_DOMAIN_MCP_PROC);
    assert_true(err_record_section != NULL);
    assert_true(err_record_section_size > 0);
    (void)err_severity;

    function_called();
}

void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    FPFW_UNUSED(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);
    function_called();
}

void __wrap_wdog_cmsdk_apb_disable()
{
    function_called();
}

void __wrap_wdog_cmsdk_apb_init(uint32_t reload_timeout, bool reset_enable)
{
    check_expected(reload_timeout);
    assert_true(reset_enable);

    function_called();
}

void __wrap_nvic_global_enable(void)
{
    function_called();
}

void __wrap_nvic_global_disable(void)
{
    function_called();
}
}

//
// Tests
//

TEST_FUNCTION(test_register_mcp_error_domain, nullptr, nullptr)
{
    expect_function_call(__wrap_fpfw_icc_base_send);
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);

    // TCM ECC
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_TCM_ERRCTRL_REG);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);
    expect_value(__wrap_mmio_write32, addr, (uint32_t)MCP_TCM_ERRCTRL_REG);
    expect_value(__wrap_mmio_write32, data, MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_DTCMRAM_ECC_EN_MASK);
    expect_function_call(__wrap_mmio_write32);

    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_TCM_ERRCTRL_REG);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);
    expect_value(__wrap_mmio_write32, addr, (uint32_t)MCP_TCM_ERRCTRL_REG);
    expect_value(__wrap_mmio_write32, data, MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_ITCMRAM_ECC_EN_MASK);
    expect_function_call(__wrap_mmio_write32);

    // TCM ECC
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_TCM_ECCCE_INT
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_TCM_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_TCM_ECCUE_INT
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_TCM_ECCUE_INT);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_TCM_ECCOF_INT
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_TCM_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_enable);

    register_mcp_error_domain((fpfw_icc_base_ctx_t*)1234);
}

static int test_setup(void** ctx)
{
    FPFW_UNUSED(ctx);

    if (g_err_inject_cb == NULL || g_tcm_ce_isr == NULL || g_tcm_ue_isr == NULL || g_tcm_of_isr == NULL)
    {
        test_register_mcp_error_domain(NULL);
    }

    return 0;
}

void test_mcp_error_injection_handler(uint16_t component_group, uint16_t error_type)
{
    if (component_group == ACPI_ERROR_DOMAIN_MCP_PROC)
    {
        ras_einj_payload.component_group = component_group;
        ras_einj_payload.param_type.error_type = error_type;

        switch (error_type)
        {
        case MCP_ERROR_TYPE_TCM_CE:
            expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_TCM_ERRCTRL_REG);
            will_return(__wrap_mmio_read32, 0);
            expect_function_call(__wrap_mmio_read32);
            expect_value(__wrap_mmio_write32, addr, (uint32_t)MCP_TCM_ERRCTRL_REG);
            expect_value(__wrap_mmio_write32, data, 0x80 | 0x20);
            expect_function_call(__wrap_mmio_write32);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)DTC_RAM_ADDRESS);
            will_return(__wrap_mmio_read32, 0);
            expect_function_call(__wrap_mmio_read32);
            break;
        case MCP_ERROR_TYPE_TCM_UE:
            expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_TCM_ERRCTRL_REG);
            will_return(__wrap_mmio_read32, 0);
            expect_function_call(__wrap_mmio_read32);
            expect_value(__wrap_mmio_write32, addr, (uint32_t)MCP_TCM_ERRCTRL_REG);
            expect_value(__wrap_mmio_write32, data, 0x80 | 0x40);
            expect_function_call(__wrap_mmio_write32);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)DTC_RAM_ADDRESS);
            will_return(__wrap_mmio_read32, 0);
            expect_function_call(__wrap_mmio_read32);
            break;
        case MCP_ERROR_TYPE_TCM_OVERFLOW:
            expect_function_call(__wrap_nvic_global_disable);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_TCM_ERRCTRL_REG);
            will_return(__wrap_mmio_read32, 0);
            expect_function_call(__wrap_mmio_read32);
            expect_value(__wrap_mmio_write32, addr, (uint32_t)MCP_TCM_ERRCTRL_REG);
            expect_value(__wrap_mmio_write32, data, 0x80 | 0x20);
            expect_function_call(__wrap_mmio_write32);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)DTC_RAM_ADDRESS);
            will_return(__wrap_mmio_read32, 0);
            expect_function_call(__wrap_mmio_read32);

            expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_TCM_ERRCTRL_REG);
            will_return(__wrap_mmio_read32, 0);
            expect_function_call(__wrap_mmio_read32);
            expect_value(__wrap_mmio_write32, addr, (uint32_t)MCP_TCM_ERRCTRL_REG);
            expect_value(__wrap_mmio_write32, data, 0x80 | 0x20);
            expect_function_call(__wrap_mmio_write32);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)DTC_RAM_ADDRESS);
            will_return(__wrap_mmio_read32, 0);
            expect_function_call(__wrap_mmio_read32);
            expect_function_call(__wrap_nvic_global_enable);
            break;
        case MCP_ERROR_TYPE_HARD_FAULT:
            test_hard_fault_handler();
            break;
        case MCP_ERROR_TYPE_BUS_FAULT:
            test_bus_fault_handler();
            break;
        case MCP_ERROR_TYPE_WATCHDOG:
            test_watchdog_handler();
            break;
        default:
            break;
        }
    }

    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);

    start_mcp_error_injection_listener((fpfw_icc_base_ctx_t*)1234);
}

TEST_FUNCTION(test_mcp_error_injection_handler_1, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, 0);
}

TEST_FUNCTION(test_mcp_error_injection_handler_2, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, MCP_ERROR_TYPE_TCM_UE);
}

TEST_FUNCTION(test_mcp_error_injection_handler_3, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, MCP_ERROR_TYPE_TCM_OVERFLOW);
}

TEST_FUNCTION(test_mcp_error_injection_handler_4, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, MCP_ERROR_TYPE_HARD_FAULT);
}

TEST_FUNCTION(test_mcp_error_injection_handler_5, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, MCP_ERROR_TYPE_BUS_FAULT);
}

TEST_FUNCTION(test_mcp_error_injection_handler_6, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, MCP_ERROR_TYPE_WATCHDOG);
}

TEST_FUNCTION(test_mcp_error_injection_handler_7, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, MCP_ERROR_TYPE_COUNT);
}

TEST_FUNCTION(test_mcp_error_injection_handler_8, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, MCP_ERROR_TYPE_TCM_CE);
}

TEST_FUNCTION(test_start_mcp_error_injection_listener, nullptr, nullptr)
{
    expect_function_call_any(__wrap_fpfw_icc_base_recv);
    will_return_always(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    start_mcp_error_injection_listener((fpfw_icc_base_ctx_t*)1234);
}

// Test function to check the behavior of the TCM ECC OF ISR
TEST_FUNCTION(test_tcm_ecc_of_isr, nullptr, nullptr)
{
    // Read the TCM error status register
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_TCM_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_OF_MASK);
    expect_function_call(__wrap_mmio_read32);

    // Read the TCM error address register
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_TCM_ERRADDRESS_REG);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Clear interrupt source
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_TCM_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);
    expect_value(__wrap_mmio_write32, addr, (uint32_t)MCP_TCM_ERRSTATUS_REG);
    expect_value(__wrap_mmio_write32, data, MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_OF_MASK);
    expect_function_call(__wrap_mmio_write32);
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_TCM_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);

    expect_function_call(__wrap_hm_submit_cper);
    expect_function_call(__wrap_crash_dump_bug_check);
    g_tcm_of_isr();
}

// Test function to check the behavior of the TCM ECC CE ISR
TEST_FUNCTION(test_tcm_ecc_ce_isr, nullptr, nullptr)
{
    // Read the TCM error status register
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_TCM_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_CE_MASK);
    expect_function_call(__wrap_mmio_read32);

    // Read the TCM error address register
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_TCM_ERRADDRESS_REG);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Clear interrupt source
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_TCM_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, 0);       // mcp_exp_csr_regs->rmss_ram1_scp_errstatus_reg
    expect_function_call(__wrap_mmio_read32); // mcp_exp_csr_regs->rmss_ram1_scp_errstatus_reg
    expect_value(__wrap_mmio_write32, addr, (uint32_t)MCP_TCM_ERRSTATUS_REG);
    expect_value(__wrap_mmio_write32, data, MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_CE_MASK);
    expect_function_call(__wrap_mmio_write32); // mcp_exp_csr_regs->rmss_ram1_scp_errstatus_reg
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_TCM_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);

    // only CPER submission expected for CE
    expect_function_call(__wrap_hm_submit_cper);
    g_tcm_ce_isr();
}

// Test function to check the behavior of the TCM ECC UE ISR
TEST_FUNCTION(test_tcm_ecc_ue_isr, nullptr, nullptr)
{
    // Read the TCM error status register
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_TCM_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_UE_MASK);
    expect_function_call(__wrap_mmio_read32);

    // Read the TCM error address register
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_TCM_ERRADDRESS_REG);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Clear interrupt source
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_TCM_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);
    expect_value(__wrap_mmio_write32, addr, (uint32_t)MCP_TCM_ERRSTATUS_REG);
    expect_value(__wrap_mmio_write32, data, MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_UE_MASK);
    expect_function_call(__wrap_mmio_write32);
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_TCM_ECCUE_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);

    expect_function_call(__wrap_hm_submit_cper);
    expect_function_call(__wrap_crash_dump_bug_check);
    g_tcm_ue_isr();
}

void test_hard_fault_handler()
{
    expect_any(NVIC_SetVector, isr);
    expect_function_call(NVIC_SetVector);
}

void test_bus_fault_handler()
{
    expect_value(__wrap_mmio_write32, addr, (uint32_t)MCP_TOP_UNUSED0_ADDRESS);
    expect_value(__wrap_mmio_write32, data, 0xBADC0FFE);
    expect_function_call(__wrap_mmio_write32);
}

void test_watchdog_handler()
{
    expect_function_call(__wrap_wdog_cmsdk_apb_disable);
    expect_value(__wrap_wdog_cmsdk_apb_init, reload_timeout, 1);
    expect_function_call(__wrap_wdog_cmsdk_apb_init);
}
