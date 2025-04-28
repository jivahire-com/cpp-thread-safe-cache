//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_err_domain_scp_init.cpp
 * Tests the init of the scp error domain
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {

#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_init.h> // for fpfw_init_result_t, fpfw_init_component_t
#include <health_monitor.h>
#include <interrupts.h>
#include <mscp_error_domain.h>
#include <nvic.h> // for nvic_status_t
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <scp_top_regs.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/
#define SCP_CSR_ADDRESS               (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS)
#define SCP_CSR_SCFRAM_ERRCTRL_REG    (SCP_CSR_ADDRESS + SCP_EXP_CSR_SCFRAM_ERRCTRL_REG_ADDRESS)
#define SCP_CSR_SCFRAM_ERRSTATUS_REG  (SCP_CSR_ADDRESS + SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_ADDRESS)
#define SCP_CSR_SCFRAM_ERRADDRESS_REG (SCP_CSR_ADDRESS + SCP_EXP_CSR_SCFRAM_SCP_ERRADDR_REG_ADDRESS)
#define SCP_CSR_RAM0_ERRCTRL_REG      (SCP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM0_ERRCTRL_REG_ADDRESS)
#define SCP_CSR_RAM0_ERRSTATUS_REG    (SCP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM0_SCP_ERRSTATUS_REG_ADDRESS)
#define SCP_CSR_RAM0_ERRADDRESS_REG   (SCP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM0_SCP_ERRADDR_REG_ADDRESS)
#define SCP_CSR_RAM1_ERRCTRL_REG      (SCP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM1_ERRCTRL_REG_ADDRESS)
#define SCP_CSR_RAM1_ERRSTATUS_REG    (SCP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM1_SCP_ERRSTATUS_REG_ADDRESS)
#define SCP_CSR_RAM1_ERRADDRESS_REG   (SCP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM1_SCP_ERRADDR_REG_ADDRESS)

#define SCP_TOP_SCF_RAM_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_RAM_ADDRESS)
#define SCP_TOP_RAM0_ADDRESS    (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS)
#define SCP_TOP_RAM1_ADDRESS    (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
hm_error_injection_cb_t g_err_inject_cb = NULL;
isr_callback_fn_sans_params_t g_scfram_of_isr = NULL;
isr_callback_fn_sans_params_t g_scfram_ce_isr = NULL;
isr_callback_fn_sans_params_t g_ram0_of_isr = NULL;
isr_callback_fn_sans_params_t g_ram0_ce_isr = NULL;
isr_callback_fn_sans_params_t g_ram1_of_isr = NULL;
isr_callback_fn_sans_params_t g_ram1_ce_isr = NULL;

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_hm_register_error_domain(uint16_t error_domain_idx,
                                     const guid_t* error_domain_guid,
                                     const char* error_domain_name,
                                     hm_error_injection_cb_t err_inject_cb,
                                     void* err_inject_ctx)
{
    assert_true(error_domain_idx == ACPI_ERROR_DOMAIN_SCP_PROC);
    assert_true(err_inject_cb != NULL);
    FPFW_UNUSED(error_domain_guid);
    FPFW_UNUSED(error_domain_name);
    FPFW_UNUSED(err_inject_ctx);

    ras_einj_info_t einj_payload;
    err_inject_cb(&einj_payload, NULL);
    function_called();

    g_err_inject_cb = err_inject_cb;
}

void __wrap_hm_submit_cper(uint16_t error_domain_idx, acpi_error_severity_t err_severity, void* err_record_section, uint32_t err_record_section_size)
{
    assert_true(error_domain_idx == ACPI_ERROR_DOMAIN_SCP_PROC);
    assert_true(err_record_section != NULL);
    assert_true(err_record_section_size > 0);
    (void)err_severity;

    function_called();
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

nvic_status_t __wrap_nvic_irq_set_isr(uint32_t irq_num, isr_callback_fn_sans_params_t isr)
{
    assert_non_null(isr);

    switch (irq_num)
    {
    case HW_INT_SCP_SCFRAM_ECCOF_INT:
        g_scfram_of_isr = isr;
        break;
    case HW_INT_SCP_SCFRAM_ECCCE_INT:
        g_scfram_ce_isr = isr;
        break;
    case HW_INT_SCP_RAM0_ECCOF_INT:
        g_ram0_of_isr = isr;
        break;
    case HW_INT_SCP_RAM0_ECCCE_INT:
        g_ram0_ce_isr = isr;
        break;
    case HW_INT_SCP_RAM1_ECCOF_INT:
        g_ram1_of_isr = isr;
        break;
    case HW_INT_SCP_RAM1_ECCCE_INT:
        g_ram1_ce_isr = isr;
        break;
    default:
        assert_true(false);
        break;
    }

    function_called();
    return NVIC_STATUS_SUCCESS;
}

nvic_status_t __wrap_nvic_irq_enable(uint32_t irq_num)
{
    check_expected(irq_num);
    function_called();
    return NVIC_STATUS_SUCCESS;
}

void __wrap_nvic_global_enable(void)
{
    function_called();
}

void __wrap_nvic_global_disable(void)
{
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
}

//
// Tests
//

TEST_FUNCTION(test_register_scp_error_domain, nullptr, nullptr)
{
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_SCFRAM_ERRCTRL_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->scfram_errctrl_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->scfram_errctrl_reg
    expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_SCFRAM_ERRCTRL_REG);
    expect_value(__wrap_mmio_write32, data, SCP_EXP_CSR_SCFRAM_ERRCTRL_REG_ECC_EN_MASK);
    expect_function_call(__wrap_mmio_write32);

    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM0_ERRCTRL_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram0_errctrl_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram0_errctrl_reg
    expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_RAM0_ERRCTRL_REG);
    expect_value(__wrap_mmio_write32, data, SCP_EXP_CSR_RMSS_RAM0_ERRCTRL_REG_ECC_EN_MASK);
    expect_function_call(__wrap_mmio_write32);

    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM1_ERRCTRL_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram1_errctrl_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram1_errctrl_reg
    expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_RAM1_ERRCTRL_REG);
    expect_value(__wrap_mmio_write32, data, SCP_EXP_CSR_RMSS_RAM1_ERRCTRL_REG_ECC_EN_MASK);
    expect_function_call(__wrap_mmio_write32);

    expect_function_call(__wrap_hm_register_error_domain);

    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_SCP_SCFRAM_ECCOF_INT
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_SCP_SCFRAM_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_SCP_SCFRAM_ECCCE_INT
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_SCP_SCFRAM_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_SCP_RAM0_ECCOF_INT
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_SCP_RAM0_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_SCP_RAM0_ECCCE_INT
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_SCP_RAM0_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_SCP_RAM1_ECCOF_INT
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_SCP_RAM1_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_SCP_RAM1_ECCCE_INT
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_SCP_RAM1_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_enable);

    register_scp_error_domain();
}

static int test_setup(void** ctx)
{
    FPFW_UNUSED(ctx);

    if (g_err_inject_cb == NULL || g_scfram_of_isr == NULL || g_scfram_ce_isr == NULL ||
        g_ram0_of_isr == NULL || g_ram0_ce_isr == NULL || g_ram1_of_isr == NULL || g_ram1_ce_isr == NULL)
    {
        test_register_scp_error_domain(NULL);
    }

    return 0;
}

void test_scp_error_injection_handler(uint16_t component_group, uint16_t error_type)
{
    ras_einj_info_t einj_payload = {};
    acpi_einj_cmd_status_t expected_status = ACPI_EINJ_SUCCESS;

    if (component_group != ACPI_ERROR_DOMAIN_SCP_PROC)
    {
        expected_status = ACPI_EINJ_INVALID_ACCESS;
    }
    else
    {
        einj_payload.component_group = component_group;
        einj_payload.param_type.error_type = error_type;

        switch (error_type)
        {
        case SCP_ERROR_TYPE_SCF_RAM_CE:
            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_SCFRAM_ERRCTRL_REG);
            will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->scfram_errctrl_reg
            expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->scfram_errctrl_reg
            expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_SCFRAM_ERRCTRL_REG);
            expect_value(__wrap_mmio_write32, data, 0x20);
            expect_function_call(__wrap_mmio_write32);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TOP_SCF_RAM_ADDRESS);
            will_return(__wrap_mmio_read32, 0);       // SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_RAM_ADDRESS
            expect_function_call(__wrap_mmio_read32); // CP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_RAM_ADDRESS
            break;
        case SCP_ERROR_TYPE_SCF_RAM_OVERFLOW:
            expect_function_call(__wrap_nvic_global_disable);

            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_SCFRAM_ERRCTRL_REG);
            will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->scfram_errctrl_reg
            expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->scfram_errctrl_reg
            expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_SCFRAM_ERRCTRL_REG);
            expect_value(__wrap_mmio_write32, data, 0x20);
            expect_function_call(__wrap_mmio_write32);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TOP_SCF_RAM_ADDRESS);
            will_return(__wrap_mmio_read32, 0);       // SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_RAM_ADDRESS
            expect_function_call(__wrap_mmio_read32); // CP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_RAM_ADDRESS

            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_SCFRAM_ERRCTRL_REG);
            will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->scfram_errctrl_reg
            expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->scfram_errctrl_reg
            expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_SCFRAM_ERRCTRL_REG);
            expect_value(__wrap_mmio_write32, data, 0x20);
            expect_function_call(__wrap_mmio_write32);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TOP_SCF_RAM_ADDRESS);
            will_return(__wrap_mmio_read32, 0);       // SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_RAM_ADDRESS
            expect_function_call(__wrap_mmio_read32); // CP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_RAM_ADDRESS

            expect_function_call(__wrap_nvic_global_enable);
            break;
        case SCP_ERROR_TYPE_RMSS_RAM0_CE:
            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM0_ERRCTRL_REG);
            will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram0_errctrl_reg
            expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram0_errctrl_reg
            expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_RAM0_ERRCTRL_REG);
            expect_value(__wrap_mmio_write32, data, 0x20);
            expect_function_call(__wrap_mmio_write32);
            expect_any(SCB_InvalidateDCache_by_Addr, addr);
            expect_any(SCB_InvalidateDCache_by_Addr, dsize);
            expect_function_call(SCB_InvalidateDCache_by_Addr);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TOP_RAM0_ADDRESS);
            will_return(__wrap_mmio_read32, 0);       // SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS
            expect_function_call(__wrap_mmio_read32); // SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS
            break;
        case SCP_ERROR_TYPE_RMSS_RAM0_OVERFLOW:
            expect_function_call(__wrap_nvic_global_disable);

            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM0_ERRCTRL_REG);
            will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram0_errctrl_reg
            expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram0_errctrl_reg
            expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_RAM0_ERRCTRL_REG);
            expect_value(__wrap_mmio_write32, data, 0x20);
            expect_function_call(__wrap_mmio_write32);
            expect_any(SCB_InvalidateDCache_by_Addr, addr);
            expect_any(SCB_InvalidateDCache_by_Addr, dsize);
            expect_function_call(SCB_InvalidateDCache_by_Addr);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TOP_RAM0_ADDRESS);
            will_return(__wrap_mmio_read32, 0);       // SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS
            expect_function_call(__wrap_mmio_read32); // SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS

            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM0_ERRCTRL_REG);
            will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram0_errctrl_reg
            expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram0_errctrl_reg
            expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_RAM0_ERRCTRL_REG);
            expect_value(__wrap_mmio_write32, data, 0x20);
            expect_function_call(__wrap_mmio_write32);
            expect_any(SCB_InvalidateDCache_by_Addr, addr);
            expect_any(SCB_InvalidateDCache_by_Addr, dsize);
            expect_function_call(SCB_InvalidateDCache_by_Addr);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TOP_RAM0_ADDRESS);
            will_return(__wrap_mmio_read32, 0);       // SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS
            expect_function_call(__wrap_mmio_read32); // SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS

            expect_function_call(__wrap_nvic_global_enable);
            break;
        case SCP_ERROR_TYPE_RMSS_RAM1_CE:
            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM1_ERRCTRL_REG);
            will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram1_errctrl_reg
            expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram1_errctrl_reg
            expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_RAM1_ERRCTRL_REG);
            expect_value(__wrap_mmio_write32, data, 0x20);
            expect_function_call(__wrap_mmio_write32);
            expect_any(SCB_InvalidateDCache_by_Addr, addr);
            expect_any(SCB_InvalidateDCache_by_Addr, dsize);
            expect_function_call(SCB_InvalidateDCache_by_Addr);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TOP_RAM1_ADDRESS);
            will_return(__wrap_mmio_read32, 0);       // SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS
            expect_function_call(__wrap_mmio_read32); // SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS
            break;
        case SCP_ERROR_TYPE_RMSS_RAM1_OVERFLOW:
            expect_function_call(__wrap_nvic_global_disable);

            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM1_ERRCTRL_REG);
            will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram1_errctrl_reg
            expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram1_errctrl_reg
            expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_RAM1_ERRCTRL_REG);
            expect_value(__wrap_mmio_write32, data, 0x20);
            expect_function_call(__wrap_mmio_write32);
            expect_any(SCB_InvalidateDCache_by_Addr, addr);
            expect_any(SCB_InvalidateDCache_by_Addr, dsize);
            expect_function_call(SCB_InvalidateDCache_by_Addr);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TOP_RAM1_ADDRESS);
            will_return(__wrap_mmio_read32, 0);       // SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS
            expect_function_call(__wrap_mmio_read32); // SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS

            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM1_ERRCTRL_REG);
            will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram1_errctrl_reg
            expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram1_errctrl_reg
            expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_RAM1_ERRCTRL_REG);
            expect_value(__wrap_mmio_write32, data, 0x20);
            expect_function_call(__wrap_mmio_write32);
            expect_any(SCB_InvalidateDCache_by_Addr, addr);
            expect_any(SCB_InvalidateDCache_by_Addr, dsize);
            expect_function_call(SCB_InvalidateDCache_by_Addr);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TOP_RAM1_ADDRESS);
            will_return(__wrap_mmio_read32, 0);       // SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS
            expect_function_call(__wrap_mmio_read32); // SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS

            expect_function_call(__wrap_nvic_global_enable);
            break;
        default:
            expected_status = ACPI_EINJ_INVALID_ACCESS;
            break;
        }
    }

    // acpi_einj_cmd_status_t status = scp_error_injection_handler(&einj_payload, NULL);
    assert_non_null(g_err_inject_cb);
    acpi_einj_cmd_status_t status = g_err_inject_cb(&einj_payload, NULL);
    assert_int_equal(status, expected_status);
}

TEST_FUNCTION(test_scp_error_injection_handler, test_setup, nullptr)
{
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, 0);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_SCF_RAM_CE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_SCF_RAM_OVERFLOW);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_RMSS_RAM0_CE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_RMSS_RAM0_OVERFLOW);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_RMSS_RAM1_CE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_RMSS_RAM1_OVERFLOW);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_USAGE_FAULT);
}

TEST_FUNCTION(test_rmss_scfram_ecc_ce_isr, test_setup, nullptr)
{
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_SCFRAM_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_CE_MASK);
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->scfram_scp_errstatus_reg
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_SCFRAM_ERRADDRESS_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->scfram_scp_erraddr_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->scfram_scp_erraddr_reg

    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_SCFRAM_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->scfram_scp_errstatus_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->scfram_scp_errstatus_reg
    expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_SCFRAM_ERRSTATUS_REG);
    expect_value(__wrap_mmio_write32, data, SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_CE_MASK);
    expect_function_call(__wrap_mmio_write32); // scp_exp_csr_regs->scfram_scp_errstatus_reg

    expect_function_call(__wrap_hm_submit_cper);
    g_scfram_ce_isr();
}

TEST_FUNCTION(test_rmss_scfram_ecc_of_isr, test_setup, nullptr)
{
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_SCFRAM_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_OF_MASK);
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->scfram_scp_errstatus_reg
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_SCFRAM_ERRADDRESS_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->scfram_scp_erraddr_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->scfram_scp_erraddr_reg

    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_SCFRAM_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->scfram_scp_errstatus_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->scfram_scp_errstatus_reg
    expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_SCFRAM_ERRSTATUS_REG);
    expect_value(__wrap_mmio_write32, data, SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_OF_MASK);
    expect_function_call(__wrap_mmio_write32); // scp_exp_csr_regs->scfram_scp_erraddr_reg

    expect_function_call(__wrap_hm_submit_cper);
    expect_function_call(__wrap_crash_dump_bug_check);
    g_scfram_of_isr();
}

TEST_FUNCTION(test_rmss_ram0_ecc_ce_isr, test_setup, nullptr)
{
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM0_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, SCP_EXP_CSR_RMSS_RAM0_SCP_ERRSTATUS_REG_CE_MASK);
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram0_scp_errstatus_reg
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM0_ERRADDRESS_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram0_scp_erraddr_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram0_scp_erraddr_reg

    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM0_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram0_scp_errstatus_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram0_scp_errstatus_reg
    expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_RAM0_ERRSTATUS_REG);
    expect_value(__wrap_mmio_write32, data, SCP_EXP_CSR_RMSS_RAM0_SCP_ERRSTATUS_REG_CE_MASK);
    expect_function_call(__wrap_mmio_write32); // scp_exp_csr_regs->rmss_ram0_scp_errstatus_reg

    expect_function_call(__wrap_hm_submit_cper);
    g_ram0_ce_isr();
}

TEST_FUNCTION(test_rmss_ram0_ecc_of_isr, test_setup, nullptr)
{
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM0_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, SCP_EXP_CSR_RMSS_RAM0_SCP_ERRSTATUS_REG_OF_MASK);
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram0_scp_errstatus_reg
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM0_ERRADDRESS_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram0_scp_erraddr_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram0_scp_erraddr_reg

    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM0_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram0_scp_errstatus_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram0_scp_errstatus_reg
    expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_RAM0_ERRSTATUS_REG);
    expect_value(__wrap_mmio_write32, data, SCP_EXP_CSR_RMSS_RAM0_SCP_ERRSTATUS_REG_OF_MASK);
    expect_function_call(__wrap_mmio_write32); // scp_exp_csr_regs->rmss_ram0_scp_errstatus_reg

    expect_function_call(__wrap_hm_submit_cper);
    expect_function_call(__wrap_crash_dump_bug_check);
    g_ram0_of_isr();
}

TEST_FUNCTION(test_rmss_ram1_ecc_ce_isr, test_setup, nullptr)
{
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM1_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, SCP_EXP_CSR_RMSS_RAM1_SCP_ERRSTATUS_REG_CE_MASK);
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram1_scp_errstatus_reg
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM1_ERRADDRESS_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram1_scp_erraddr_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram1_scp_erraddr_reg

    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM1_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram1_scp_errstatus_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram1_scp_errstatus_reg
    expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_RAM1_ERRSTATUS_REG);
    expect_value(__wrap_mmio_write32, data, SCP_EXP_CSR_RMSS_RAM1_SCP_ERRSTATUS_REG_CE_MASK);
    expect_function_call(__wrap_mmio_write32); // scp_exp_csr_regs->rmss_ram1_scp_errstatus_reg

    expect_function_call(__wrap_hm_submit_cper);
    g_ram1_ce_isr();
}

TEST_FUNCTION(test_rmss_ram1_ecc_of_isr, test_setup, nullptr)
{
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM1_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, SCP_EXP_CSR_RMSS_RAM1_SCP_ERRSTATUS_REG_OF_MASK);
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram1_scp_errstatus_reg
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM1_ERRADDRESS_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram1_scp_erraddr_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram1_scp_erraddr_reg

    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM1_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram1_scp_errstatus_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram1_scp_errstatus_reg
    expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_RAM1_ERRSTATUS_REG);
    expect_value(__wrap_mmio_write32, data, SCP_EXP_CSR_RMSS_RAM1_SCP_ERRSTATUS_REG_OF_MASK);
    expect_function_call(__wrap_mmio_write32); // scp_exp_csr_regs->rmss_ram1_scp_errstatus_reg

    expect_function_call(__wrap_hm_submit_cper);
    expect_function_call(__wrap_crash_dump_bug_check);
    g_ram1_of_isr();
}