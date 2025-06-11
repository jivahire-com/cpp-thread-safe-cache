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
#include <atu_api.h>
#include <atu_lib.h> // for atu_map_entry_t
#include <cortexm7integrationcs_scp_regs.h>
#include <fpfw_init.h> // for fpfw_init_result_t, fpfw_init_component_t
#include <health_monitor.h>
#include <idsw_kng.h>
#include <interrupts.h>
#include <kng_atu_mappings.h>
#include <mscp_error_domain.h>
#include <mscp_ras_and_init_ctrl_registers_regs.h>
#include <nvic.h> // for nvic_status_t
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <scp_top_regs.h>
#include <shared_sram_ecc_ras_registers_regs.h>
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
#define SCP_CSR_REMOTE_SCP_SCFRAM_ERRSTATUS_REG \
    (SCP_CSR_ADDRESS + SCP_EXP_CSR_SCFRAM_REM_SCP_ERRSTATUS_REG_ADDRESS)
#define SCP_CSR_REMOTE_SCP_RAM0_ERRSTATUS_REG \
    (SCP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM0_REM_SCP_ERRSTATUS_REG_ADDRESS)
#define SCP_CSR_REMOTE_SCP_RAM1_ERRSTATUS_REG \
    (SCP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM1_REM_SCP_ERRSTATUS_REG_ADDRESS)
#define SCP_TCM_ERRCTRL_REG \
    (SCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_ADDRESS)
#define SCP_TCM_ERRSTATUS_REG \
    (SCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_ADDRESS)
#define SCP_TCM_ERRADDRESS_REG \
    (SCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRADDR_ADDRESS)

#define SCP_TOP_SCF_RAM_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_RAM_ADDRESS)
#define SCP_TOP_RAM0_ADDRESS    (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS)
#define SCP_TOP_RAM1_ADDRESS    (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS)
#define DTC_RAM_ADDRESS         (SCP_TOP_SCP_DATA_RAM_ADDRESS)

#define SCP_CORTEX_M7_SystemControl_ADDRESS \
    (SCP_TOP_CORTEX_M7_ADDRESS + CORTEXM7INTEGRATIONCS_SCP_SYSTEMCONTROL_ADDRESS)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void test_dcache_ue_isr();
void test_dcache_ce_isr();
void test_dcache_tag_ue_isr();
void test_dcache_tag_ce_isr();
void test_icache_ue_isr();
void test_icache_ce_isr();
void test_icache_tag_ue_isr();
void test_icache_tag_ce_isr();
void test_hard_fault_handler();
void test_bus_fault_handler();
void test_watchdog_handler();
void test_trigger_shared_sram_fault(uint32_t err_mask);

/*-- Declarations (Statics and globals) --*/
hm_error_injection_cb_t g_err_inject_cb = NULL;
isr_callback_fn_sans_params_t g_scfram_of_isr = NULL;
isr_callback_fn_sans_params_t g_scfram_ce_isr = NULL;
isr_callback_fn_sans_params_t g_ram0_of_isr = NULL;
isr_callback_fn_sans_params_t g_ram0_ce_isr = NULL;
isr_callback_fn_sans_params_t g_ram1_of_isr = NULL;
isr_callback_fn_sans_params_t g_ram1_ce_isr = NULL;
isr_callback_fn_sans_params_t g_tcm_ce_isr = NULL;
isr_callback_fn_sans_params_t g_tcm_ue_isr = NULL;
isr_callback_fn_sans_params_t g_tcm_of_isr = NULL;
isr_callback_fn_sans_params_t g_dcache_ue_isr = NULL;
isr_callback_fn_sans_params_t g_dcache_ce_isr = NULL;
isr_callback_fn_sans_params_t g_dcache_tag_ue_isr = NULL;
isr_callback_fn_sans_params_t g_dcache_tag_ce_isr = NULL;
isr_callback_fn_sans_params_t g_icache_ue_isr = NULL;
isr_callback_fn_sans_params_t g_icache_ce_isr = NULL;
isr_callback_fn_sans_params_t g_icache_tag_ue_isr = NULL;
isr_callback_fn_sans_params_t g_icache_tag_ce_isr = NULL;
isr_callback_fn_sans_params_t g_rmss_remote_scp_scfram_bootram_isr = NULL;
isr_callback_fn_with_params_t g_s_arsm_ecc_isr = NULL;

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
    case HW_INT_SCP_TCM_ECCCE_INT:
        g_tcm_ce_isr = isr;
        break;
    case HW_INT_SCP_TCM_ECCUE_INT:
        g_tcm_ue_isr = isr;
        break;
    case HW_INT_SCP_TCM_ECCOF_INT:
        g_tcm_of_isr = isr;
        break;
    case HW_INT_DCDET_DATA_UE:
        g_dcache_ue_isr = isr;
        break;
    case HW_INT_DCDET_DATA_CE:
        g_dcache_ce_isr = isr;
        break;
    case HW_INT_DCDET_TAG_UE:
        g_dcache_tag_ue_isr = isr;
        break;
    case HW_INT_DCDET_TAG_CE:
        g_dcache_tag_ce_isr = isr;
        break;
    case HW_INT_ICDET_DATA_UE:
        g_icache_ue_isr = isr;
        break;
    case HW_INT_ICDET_DATA_CE:
        g_icache_ce_isr = isr;
        break;
    case HW_INT_ICDET_TAG_UE:
        g_icache_tag_ue_isr = isr;
        break;
    case HW_INT_ICDET_TAG_CE:
        g_icache_tag_ce_isr = isr;
        break;
    case HW_INT_REMOTE_SCP_RAM_CE_OF_INT:
        g_rmss_remote_scp_scfram_bootram_isr = isr;
        break;
    default:
        assert_true(false);
        break;
    }

    function_called();
    return NVIC_STATUS_SUCCESS;
}

nvic_status_t __wrap_nvic_irq_set_isr_with_param(uint32_t irq_num, isr_callback_fn_with_params_t isr, void* parameter)
{
    FPFW_UNUSED(parameter);
    switch (irq_num)
    {
    case HW_INT_SCP_S_ARSM_ECC_FHI_INT:
    case HW_INT_SCP_NS_ARSM_ECC_FHI_INT:
    case HW_INT_SCP_RT_ARSM_ECC_FHI_INT:
    case HW_INT_SCP_RL_ARSM_ECC_FHI_INT:
        if (g_s_arsm_ecc_isr == NULL)
        {
            g_s_arsm_ecc_isr = isr;
        }
        break;
    default:
        break;
    }

    function_called();
    return NVIC_STATUS_SUCCESS;
}

nvic_status_t __wrap_nvic_get_current_irq(uint32_t* irq_num)
{
    assert_non_null(irq_num);
    *irq_num = mock_type(uint32_t);
    function_called();

    return NVIC_STATUS_SUCCESS;
}

nvic_status_t __wrap_nvic_irq_enable(uint32_t irq_num)
{
    check_expected(irq_num);
    function_called();
    return NVIC_STATUS_SUCCESS;
}

nvic_status_t __wrap_nvic_irq_clear_pending(uint32_t irq_num)
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

static uint8_t mapped_region[0x20000] = {0};

int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    assert_true(atu_id == ATU_ID_MSCP);
    assert_non_null(atu_map_entry);

    atu_map_entry->mscp_start_address = (uint32_t)mapped_region;

    function_called();

    return 0;
}

int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    assert_true(atu_id == ATU_ID_MSCP);
    assert_non_null(atu_map_entry);

    function_called();

    return 0;
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return 0; // DIE0 for testing purposes
}

bool __wrap_idhw_is_single_die_boot_en(void)
{
    return mock_type(bool);
}
}

//
// Tests
//

TEST_FUNCTION(test_register_scp_error_domain, nullptr, nullptr)
{
    expect_function_call(__wrap_hm_register_error_domain);

    // SCF RAM ECC
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_SCP_SCFRAM_ECCOF_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_SCFRAM_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_SCP_SCFRAM_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_SCP_SCFRAM_ECCCE_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_SCFRAM_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_SCP_SCFRAM_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_enable);

    // Boot RAM ECC
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_SCP_RAM0_ECCOF_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_RAM0_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_SCP_RAM0_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_SCP_RAM0_ECCCE_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_RAM0_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_SCP_RAM0_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_SCP_RAM1_ECCOF_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_RAM1_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_SCP_RAM1_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_SCP_RAM1_ECCCE_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_RAM1_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_SCP_RAM1_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_enable);

    // Remote SCP ECC on SCF/RAM0/RAM1
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_REMOTE_SCP_RAM_CE_OF_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_REMOTE_SCP_RAM_CE_OF_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_REMOTE_SCP_RAM_CE_OF_INT);
    expect_function_call(__wrap_nvic_irq_enable);

    // TCM ECC
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_SCP_TCM_ECCCE_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_TCM_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_SCP_TCM_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_SCP_TCM_ECCUE_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_TCM_ECCUE_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_SCP_TCM_ECCUE_INT);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_SCP_TCM_ECCOF_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_TCM_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_SCP_TCM_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_enable);

    // Data Cache ECC
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_DCDET_DATA_UE
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_DCDET_DATA_UE);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_DCDET_DATA_UE);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_DCDET_DATA_CE
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_DCDET_DATA_CE);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_DCDET_DATA_CE);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_DCDET_TAG_UE
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_DCDET_TAG_UE);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_DCDET_TAG_UE);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_DCDET_TAG_CE
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_DCDET_TAG_CE);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_DCDET_TAG_CE);
    expect_function_call(__wrap_nvic_irq_enable);

    // Instruction Cache ECC
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_ICDET_DATA_UE
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_ICDET_DATA_UE);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_ICDET_DATA_UE);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_ICDET_DATA_CE
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_ICDET_DATA_CE);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_ICDET_DATA_CE);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_ICDET_TAG_UE
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_ICDET_TAG_UE);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_ICDET_TAG_UE);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_ICDET_TAG_CE
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_ICDET_TAG_CE);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_ICDET_TAG_CE);
    expect_function_call(__wrap_nvic_irq_enable);

    expect_function_call(__wrap_nvic_irq_set_isr_with_param); // HW_INT_SCP_S_ARSM_ECC_FHI_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_S_ARSM_ECC_FHI_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_SCP_S_ARSM_ECC_FHI_INT);
    expect_function_call(__wrap_nvic_irq_enable);

    expect_function_call(__wrap_nvic_irq_set_isr_with_param); // HW_INT_SCP_NS_ARSM_ECC_FHI_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_NS_ARSM_ECC_FHI_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_SCP_NS_ARSM_ECC_FHI_INT);
    expect_function_call(__wrap_nvic_irq_enable);

    expect_function_call(__wrap_nvic_irq_set_isr_with_param); // HW_INT_SCP_RT_ARSM_ECC_FHI_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_RT_ARSM_ECC_FHI_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_SCP_RT_ARSM_ECC_FHI_INT);
    expect_function_call(__wrap_nvic_irq_enable);

    expect_function_call(__wrap_nvic_irq_set_isr_with_param); // HW_INT_SCP_RL_ARSM_ECC_FHI_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_RL_ARSM_ECC_FHI_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_SCP_RL_ARSM_ECC_FHI_INT);
    expect_function_call(__wrap_nvic_irq_enable);

    // SCF RAM ECC
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_SCFRAM_ERRCTRL_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->scfram_errctrl_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->scfram_errctrl_reg
    expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_SCFRAM_ERRCTRL_REG);
    expect_value(__wrap_mmio_write32, data, SCP_EXP_CSR_SCFRAM_ERRCTRL_REG_ECC_EN_MASK);
    expect_function_call(__wrap_mmio_write32);

    // Boot RAM ECC
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

    // TCM ECC
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TCM_ERRCTRL_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram1_errctrl_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram1_errctrl_reg
    expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_TCM_ERRCTRL_REG);
    expect_value(__wrap_mmio_write32, data, MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_DTCMRAM_ECC_EN_MASK);
    expect_function_call(__wrap_mmio_write32);

    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TCM_ERRCTRL_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram1_errctrl_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram1_errctrl_reg
    expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_TCM_ERRCTRL_REG);
    expect_value(__wrap_mmio_write32, data, MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_ITCMRAM_ECC_EN_MASK);
    expect_function_call(__wrap_mmio_write32);

    // Shared SRAM ECC
    for (int i = SCP_S_ARSM_RAM; i < SCP_ARSM_RAM_COUNT; i++)
    {
        expect_function_call(__wrap_atu_map);
        expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRFR_ADDRESS);
        will_return(__wrap_mmio_read32,
                    SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRFR_ED_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRFR_FI_MASK |
                        SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRFR_UE_MASK |
                        SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRFR_CFI_MASK);
        expect_function_call(__wrap_mmio_read32);
        expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRCTRL_ADDRESS);
        will_return(__wrap_mmio_read32, 0);
        expect_function_call(__wrap_mmio_read32);
        expect_value(__wrap_mmio_write32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRCTRL_ADDRESS);
        expect_value(__wrap_mmio_write32,
                     data,
                     SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRCTRL_ED_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRCTRL_FI_MASK |
                         SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRCTRL_UE_MASK |
                         SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRCTRL_CFI_MASK);
        expect_function_call(__wrap_mmio_write32);
        expect_function_call(__wrap_atu_unmap);
    }

    register_scp_error_domain();
}

static int test_setup(void** ctx)
{
    FPFW_UNUSED(ctx);

    if (g_err_inject_cb == NULL || g_scfram_of_isr == NULL || g_scfram_ce_isr == NULL ||
        g_ram0_of_isr == NULL || g_ram0_ce_isr == NULL || g_ram1_of_isr == NULL || g_ram1_ce_isr == NULL ||
        g_tcm_ce_isr == NULL || g_tcm_ue_isr == NULL || g_tcm_of_isr == NULL || g_dcache_ue_isr == NULL ||
        g_dcache_ce_isr == NULL || g_dcache_tag_ue_isr == NULL || g_dcache_tag_ce_isr == NULL ||
        g_icache_ue_isr == NULL || g_icache_ce_isr == NULL || g_icache_tag_ue_isr == NULL ||
        g_icache_tag_ce_isr == NULL || g_rmss_remote_scp_scfram_bootram_isr == NULL || g_s_arsm_ecc_isr == NULL)
    {
        test_register_scp_error_domain(NULL);
    }

    return 0;
}

void test_trigger_shared_sram_fault(uint32_t err_mask)
{
    expect_function_call(__wrap_atu_map);

    if (err_mask & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK)
    {
        expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_ADDRESS);
        will_return(__wrap_mmio_read32, 0);
        expect_function_call(__wrap_mmio_read32);
        expect_value(__wrap_mmio_write32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_ADDRESS);
        expect_value(__wrap_mmio_write32, data, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_CE_MASK);
        expect_function_call(__wrap_mmio_write32);

        expect_value(__wrap_mmio_read32, addr, MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + 0x10);
        will_return(__wrap_mmio_read32, 0);
        expect_function_call(__wrap_mmio_read32);
    }
    else if (err_mask & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK)
    {
        expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_ADDRESS);
        will_return(__wrap_mmio_read32, 0);
        expect_function_call(__wrap_mmio_read32);
        expect_value(__wrap_mmio_write32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_ADDRESS);
        expect_value(__wrap_mmio_write32, data, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_UE_MASK);
        expect_function_call(__wrap_mmio_write32);

        expect_value(__wrap_mmio_read32, addr, MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + 0x10);
        will_return(__wrap_mmio_read32, 0);
        expect_function_call(__wrap_mmio_read32);
    }
    else if (err_mask & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK)
    {
        expect_function_call(__wrap_nvic_global_disable);

        expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_ADDRESS);
        will_return(__wrap_mmio_read32, 0);
        expect_function_call(__wrap_mmio_read32);
        expect_value(__wrap_mmio_write32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_ADDRESS);
        expect_value(__wrap_mmio_write32, data, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_UE_MASK);
        expect_function_call(__wrap_mmio_write32);

        expect_value(__wrap_mmio_read32, addr, MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + 0x10);
        will_return(__wrap_mmio_read32, 0);
        expect_function_call(__wrap_mmio_read32);

        expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_ADDRESS);
        will_return(__wrap_mmio_read32, 0);
        expect_function_call(__wrap_mmio_read32);
        expect_value(__wrap_mmio_write32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_ADDRESS);
        expect_value(__wrap_mmio_write32, data, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_UE_MASK);
        expect_function_call(__wrap_mmio_write32);

        expect_value(__wrap_mmio_read32, addr, MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + 0x10);
        will_return(__wrap_mmio_read32, 0);
        expect_function_call(__wrap_mmio_read32);

        expect_function_call(__wrap_nvic_global_enable);
    }

    expect_function_call(__wrap_atu_unmap);
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
        case SCP_ERROR_TYPE_SCF_RAM_UE:
            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_SCFRAM_ERRCTRL_REG);
            will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->scfram_errctrl_reg
            expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->scfram_errctrl_reg
            expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_SCFRAM_ERRCTRL_REG);
            expect_value(__wrap_mmio_write32, data, 0x40);
            expect_function_call(__wrap_mmio_write32);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TOP_SCF_RAM_ADDRESS);
            will_return(__wrap_mmio_read32, 0);       // SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_RAM_ADDRESS
            expect_function_call(__wrap_mmio_read32); // CP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_RAM_ADDRESS
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
        case SCP_ERROR_TYPE_RMSS_RAM0_UE:
            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM0_ERRCTRL_REG);
            will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram0_errctrl_reg
            expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram0_errctrl_reg
            expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_RAM0_ERRCTRL_REG);
            expect_value(__wrap_mmio_write32, data, 0x40);
            expect_function_call(__wrap_mmio_write32);
            expect_any(SCB_InvalidateDCache_by_Addr, addr);
            expect_any(SCB_InvalidateDCache_by_Addr, dsize);
            expect_function_call(SCB_InvalidateDCache_by_Addr);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TOP_RAM0_ADDRESS);
            will_return(__wrap_mmio_read32, 0);       // SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS
            expect_function_call(__wrap_mmio_read32); // SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS
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
        case SCP_ERROR_TYPE_RMSS_RAM1_UE:
            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_RAM1_ERRCTRL_REG);
            will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram1_errctrl_reg
            expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram1_errctrl_reg
            expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_CSR_RAM1_ERRCTRL_REG);
            expect_value(__wrap_mmio_write32, data, 0x40);
            expect_function_call(__wrap_mmio_write32);
            expect_any(SCB_InvalidateDCache_by_Addr, addr);
            expect_any(SCB_InvalidateDCache_by_Addr, dsize);
            expect_function_call(SCB_InvalidateDCache_by_Addr);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TOP_RAM1_ADDRESS);
            will_return(__wrap_mmio_read32, 0);       // SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS
            expect_function_call(__wrap_mmio_read32); // SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS
            break;
        case SCP_ERROR_TYPE_TCM_CE:
            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TCM_ERRCTRL_REG);
            will_return(__wrap_mmio_read32, 0);
            expect_function_call(__wrap_mmio_read32);
            expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_TCM_ERRCTRL_REG);
            expect_value(__wrap_mmio_write32, data, 0x80 | 0x20);
            expect_function_call(__wrap_mmio_write32);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)DTC_RAM_ADDRESS);
            will_return(__wrap_mmio_read32, 0);
            expect_function_call(__wrap_mmio_read32);
            break;
        case SCP_ERROR_TYPE_TCM_UE:
            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TCM_ERRCTRL_REG);
            will_return(__wrap_mmio_read32, 0);
            expect_function_call(__wrap_mmio_read32);
            expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_TCM_ERRCTRL_REG);
            expect_value(__wrap_mmio_write32, data, 0x80 | 0x40);
            expect_function_call(__wrap_mmio_write32);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)DTC_RAM_ADDRESS);
            will_return(__wrap_mmio_read32, 0);
            expect_function_call(__wrap_mmio_read32);
            break;
        case SCP_ERROR_TYPE_TCM_OVERFLOW:
            expect_function_call(__wrap_nvic_global_disable);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TCM_ERRCTRL_REG);
            will_return(__wrap_mmio_read32, 0);
            expect_function_call(__wrap_mmio_read32);
            expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_TCM_ERRCTRL_REG);
            expect_value(__wrap_mmio_write32, data, 0x80 | 0x20);
            expect_function_call(__wrap_mmio_write32);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)DTC_RAM_ADDRESS);
            will_return(__wrap_mmio_read32, 0);
            expect_function_call(__wrap_mmio_read32);

            expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TCM_ERRCTRL_REG);
            will_return(__wrap_mmio_read32, 0);
            expect_function_call(__wrap_mmio_read32);
            expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_TCM_ERRCTRL_REG);
            expect_value(__wrap_mmio_write32, data, 0x80 | 0x20);
            expect_function_call(__wrap_mmio_write32);
            expect_value(__wrap_mmio_read32, addr, (uint32_t)DTC_RAM_ADDRESS);
            will_return(__wrap_mmio_read32, 0);
            expect_function_call(__wrap_mmio_read32);
            expect_function_call(__wrap_nvic_global_enable);
            break;
        case SCP_ERROR_TYPE_DATA_CACHE_CE:
            test_dcache_ce_isr();
            break;
        case SCP_ERROR_TYPE_DATA_CACHE_UE:
            test_dcache_ue_isr();
            break;
        case SCP_ERROR_TYPE_DATA_CACHE_TAG_CE:
            test_dcache_tag_ce_isr();
            break;
        case SCP_ERROR_TYPE_DATA_CACHE_TAG_UE:
            test_dcache_tag_ue_isr();
            break;
        case SCP_ERROR_TYPE_INSTRUCTION_CACHE_CE:
            test_icache_ce_isr();
            break;
        case SCP_ERROR_TYPE_INSTRUCTION_CACHE_UE:
            test_icache_ue_isr();
            break;
        case SCP_ERROR_TYPE_INSTRUCTION_CACHE_TAG_CE:
            test_icache_tag_ce_isr();
            break;
        case SCP_ERROR_TYPE_INSTRUCTION_CACHE_TAG_UE:
            test_icache_tag_ue_isr();
            break;
        case SCP_ERROR_TYPE_HARD_FAULT:
            test_hard_fault_handler();
            break;
        case SCP_ERROR_TYPE_BUS_FAULT:
            test_bus_fault_handler();
            break;
        case SCP_ERROR_TYPE_WATCHDOG:
            test_watchdog_handler();
            break;
        case SCP_ERROR_TYPE_S_ARSM_CE:
        case SCP_ERROR_TYPE_NS_ARSM_CE:
        case SCP_ERROR_TYPE_RT_ARSM_CE:
        case SCP_ERROR_TYPE_RL_ARSM_CE:
            test_trigger_shared_sram_fault(SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
            break;
        case SCP_ERROR_TYPE_S_ARSM_UE:
        case SCP_ERROR_TYPE_NS_ARSM_UE:
        case SCP_ERROR_TYPE_RT_ARSM_UE:
        case SCP_ERROR_TYPE_RL_ARSM_UE:
            // test_trigger_shared_sram_fault(SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
            break;
        case SCP_ERROR_TYPE_S_ARSM_OVERFLOW:
        case SCP_ERROR_TYPE_NS_ARSM_OVERFLOW:
        case SCP_ERROR_TYPE_RT_ARSM_OVERFLOW:
        case SCP_ERROR_TYPE_RL_ARSM_OVERFLOW:
            // test_trigger_shared_sram_fault(SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK);
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
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_SCF_RAM_UE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_RMSS_RAM0_CE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_RMSS_RAM0_OVERFLOW);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_RMSS_RAM0_UE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_RMSS_RAM1_CE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_RMSS_RAM1_OVERFLOW);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_RMSS_RAM1_UE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_TCM_CE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_TCM_UE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_TCM_OVERFLOW);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_DATA_CACHE_CE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_DATA_CACHE_UE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_DATA_CACHE_TAG_CE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_DATA_CACHE_TAG_UE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_INSTRUCTION_CACHE_CE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_INSTRUCTION_CACHE_UE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_INSTRUCTION_CACHE_TAG_CE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_HARD_FAULT);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_BUS_FAULT);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_WATCHDOG);

    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_S_ARSM_CE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_S_ARSM_UE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_S_ARSM_OVERFLOW);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_NS_ARSM_CE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_NS_ARSM_UE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_NS_ARSM_OVERFLOW);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_RT_ARSM_CE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_RT_ARSM_UE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_RT_ARSM_OVERFLOW);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_RL_ARSM_CE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_RL_ARSM_UE);
    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_RL_ARSM_OVERFLOW);

    test_scp_error_injection_handler(ACPI_ERROR_DOMAIN_SCP_PROC, SCP_ERROR_TYPE_COUNT);
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
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_SCFRAM_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);

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
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_SCFRAM_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);

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
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_RAM0_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);

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
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_RAM0_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);

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
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_RAM1_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);

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
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_RAM1_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);

    expect_function_call(__wrap_hm_submit_cper);
    expect_function_call(__wrap_crash_dump_bug_check);
    g_ram1_of_isr();
}

TEST_FUNCTION(test_rmss_remote_scp_scfram_bootram_isr, test_setup, nullptr)
{
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_REMOTE_SCP_SCFRAM_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->scfram_rem_scp_errstatus_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->scfram_rem_scp_errstatus_reg

    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_REMOTE_SCP_RAM0_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram0_rem_scp_errstatus_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram0_rem_scp_errstatus_reg

    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CSR_REMOTE_SCP_RAM1_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram1_rem_scp_errstatus_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram1_rem_scp_errstatus_reg

    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_REMOTE_SCP_RAM_CE_OF_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);

    expect_function_call(__wrap_hm_submit_cper);
    g_rmss_remote_scp_scfram_bootram_isr();
}

// Test function to check the behavior of the TCM ECC OF ISR
TEST_FUNCTION(test_tcm_ecc_of_isr, test_setup, nullptr)
{
    // Read the TCM error status register
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TCM_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_OF_MASK);
    expect_function_call(__wrap_mmio_read32);

    // Read the TCM error address register
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TCM_ERRADDRESS_REG);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Clear interrupt source
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TCM_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);
    expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_TCM_ERRSTATUS_REG);
    expect_value(__wrap_mmio_write32, data, MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_OF_MASK);
    expect_function_call(__wrap_mmio_write32);
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_TCM_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);

    expect_function_call(__wrap_hm_submit_cper);
    expect_function_call(__wrap_crash_dump_bug_check);
    g_tcm_of_isr();
}

// Test function to check the behavior of the TCM ECC CE ISR
TEST_FUNCTION(test_tcm_ecc_ce_isr, test_setup, nullptr)
{
    // Read the TCM error status register
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TCM_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_CE_MASK);
    expect_function_call(__wrap_mmio_read32);

    // Read the TCM error address register
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TCM_ERRADDRESS_REG);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Clear interrupt source
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TCM_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, 0);       // scp_exp_csr_regs->rmss_ram1_scp_errstatus_reg
    expect_function_call(__wrap_mmio_read32); // scp_exp_csr_regs->rmss_ram1_scp_errstatus_reg
    expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_TCM_ERRSTATUS_REG);
    expect_value(__wrap_mmio_write32, data, MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_CE_MASK);
    expect_function_call(__wrap_mmio_write32); // scp_exp_csr_regs->rmss_ram1_scp_errstatus_reg
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_TCM_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);

    // only CPER submission expected for CE
    expect_function_call(__wrap_hm_submit_cper);
    g_tcm_ce_isr();
}

// Test function to check the behavior of the TCM ECC UE ISR
TEST_FUNCTION(test_tcm_ecc_ue_isr, test_setup, nullptr)
{
    // Read the TCM error status register
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TCM_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_UE_MASK);
    expect_function_call(__wrap_mmio_read32);

    // Read the TCM error address register
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TCM_ERRADDRESS_REG);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Clear interrupt source
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_TCM_ERRSTATUS_REG);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);
    expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_TCM_ERRSTATUS_REG);
    expect_value(__wrap_mmio_write32, data, MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_UE_MASK);
    expect_function_call(__wrap_mmio_write32);
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_SCP_TCM_ECCUE_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);

    expect_function_call(__wrap_hm_submit_cper);
    expect_function_call(__wrap_crash_dump_bug_check);
    g_tcm_ue_isr();
}

// Test function to check the behavior of the Data Cache ECC UE ISR
void test_dcache_ue_isr()
{
    // Read the DEBR0
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_DEBR0H_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Read the DEBR1
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_DEBR1H_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Clear interrupt source
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_DCDET_DATA_UE);
    expect_function_call(__wrap_nvic_irq_clear_pending);

    // Submit CPER and Bugcheck as UE
    expect_function_call(__wrap_hm_submit_cper);
    expect_function_call(__wrap_crash_dump_bug_check);
}

TEST_FUNCTION(test_dcache_ue_isr, test_setup, nullptr)
{
    test_dcache_ue_isr();
    g_dcache_ue_isr();
}

void test_dcache_ce_isr()
{
    // Read the DEBR0
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_DEBR0H_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Read the DEBR1
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_DEBR1H_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Clear interrupt source
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_DCDET_DATA_CE);
    expect_function_call(__wrap_nvic_irq_clear_pending);

    // Submit CPER
    expect_function_call(__wrap_hm_submit_cper);
}

// Test function to check the behavior of the Data Cache ECC CE ISR
TEST_FUNCTION(test_dcache_ce_isr, test_setup, nullptr)
{
    test_dcache_ce_isr();
    g_dcache_ce_isr();
}

void test_dcache_tag_ue_isr()
{
    // Read the DEBR0
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_DEBR0H_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Read the DEBR1
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_DEBR1H_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Clear interrupt source
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_DCDET_TAG_UE);
    expect_function_call(__wrap_nvic_irq_clear_pending);

    // Submit CPER and Bugcheck as UE
    expect_function_call(__wrap_hm_submit_cper);
    expect_function_call(__wrap_crash_dump_bug_check);
}

// Test function to check the behavior of the Data Cache Tag ECC UE ISR
TEST_FUNCTION(dcache_tag_ue_isr, test_setup, nullptr)
{
    test_dcache_tag_ue_isr();
    g_dcache_tag_ue_isr();
}

// Test function to check the behavior of the Data Cache Tag ECC CE ISR
void test_dcache_tag_ce_isr()
{
    // Read the DEBR0
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_DEBR0H_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Read the DEBR1
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_DEBR1H_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Clear interrupt source
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_DCDET_TAG_CE);
    expect_function_call(__wrap_nvic_irq_clear_pending);

    // Submit CPER
    expect_function_call(__wrap_hm_submit_cper);
}

TEST_FUNCTION(dcache_tag_ce_isr, test_setup, nullptr)
{
    test_dcache_tag_ce_isr();
    g_dcache_tag_ce_isr();
}

// Test function to check the behavior of the Instruction Cache ECC UE ISR
void test_icache_ue_isr()
{
    // Read the DEBR0
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_IEBR0_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Read the DEBR1
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_IEBR1H_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Clear interrupt source
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_ICDET_DATA_UE);
    expect_function_call(__wrap_nvic_irq_clear_pending);

    // Submit CPER and Bugcheck as UE
    expect_function_call(__wrap_hm_submit_cper);
    expect_function_call(__wrap_crash_dump_bug_check);
}

TEST_FUNCTION(test_icache_ue_isr, test_setup, nullptr)
{
    test_icache_ue_isr();
    g_icache_ue_isr();
}

// Test function to check the behavior of the Instruction Cache ECC CE ISR
void test_icache_ce_isr()
{
    // Read the DEBR0
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_IEBR0_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Read the DEBR1
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_IEBR1H_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Clear interrupt source
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_ICDET_DATA_CE);
    expect_function_call(__wrap_nvic_irq_clear_pending);

    // Submit CPER
    expect_function_call(__wrap_hm_submit_cper);
}

TEST_FUNCTION(test_icache_ce_isr, test_setup, nullptr)
{
    test_icache_ce_isr();
    g_icache_ce_isr();
}

// Test function to check the behavior of the Instruction Cache Tag ECC UE ISR
void test_icache_tag_ue_isr()
{
    // Read the DEBR0
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_IEBR0_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Read the DEBR1
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_IEBR1H_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Clear interrupt source
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_ICDET_TAG_UE);
    expect_function_call(__wrap_nvic_irq_clear_pending);

    // Submit CPER and Bugcheck as UE
    expect_function_call(__wrap_hm_submit_cper);
    expect_function_call(__wrap_crash_dump_bug_check);
}

TEST_FUNCTION(test_icache_tag_ue_isr, test_setup, nullptr)
{
    test_icache_tag_ue_isr();
    g_icache_tag_ue_isr();
}

// Test function to check the behavior of the Instruction Cache Tag ECC CE ISR
void test_icache_tag_ce_isr()
{
    // Read the DEBR0
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_IEBR0_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Read the DEBR1
    expect_value(__wrap_mmio_read32, addr, (uint32_t)SCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_IEBR1H_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Clear interrupt source
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_ICDET_TAG_CE);
    expect_function_call(__wrap_nvic_irq_clear_pending);

    // Submit CPER
    expect_function_call(__wrap_hm_submit_cper);
}

TEST_FUNCTION(test_icache_tag_ce_isr, test_setup, nullptr)
{
    test_icache_tag_ce_isr();
    g_icache_tag_ce_isr();
}

void test_hard_fault_handler()
{
    expect_any(NVIC_SetVector, isr);
    expect_function_call(NVIC_SetVector);
}

void test_bus_fault_handler()
{
    expect_value(__wrap_mmio_write32, addr, (uint32_t)SCP_TOP_UNUSED10_ADDRESS);
    expect_value(__wrap_mmio_write32, data, 0xBADC0FFE);
    expect_function_call(__wrap_mmio_write32);
}

void test_watchdog_handler()
{
    expect_function_call(__wrap_wdog_cmsdk_apb_disable);
    expect_value(__wrap_wdog_cmsdk_apb_init, reload_timeout, 1);
    expect_function_call(__wrap_wdog_cmsdk_apb_init);
}

TEST_FUNCTION(test_shared_sram_ecc_isr_ue, test_setup, nullptr)
{
    atu_map_entry_t atu_entry = ATU_MAPPING_SCP_S_ARSM_RAM_ECC(0);

    expect_function_call(__wrap_atu_map);

    // Read error status
    expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ADDRESS);
    will_return(__wrap_mmio_read32,
                SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_MASK |
                    SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK | 0x02);
    expect_function_call(__wrap_mmio_read32);

    // Read error address if applicable
    expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRADDR_ADDRESS);
    will_return(__wrap_mmio_read32, 0x12345678);
    expect_function_call(__wrap_mmio_read32);

    // Clear the error status
    expect_value(__wrap_mmio_write32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ADDRESS);
    expect_value(__wrap_mmio_write32,
                 data,
                 SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_MASK |
                     SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
    expect_function_call(__wrap_mmio_write32);

    expect_function_call(__wrap_hm_submit_cper);

    // Unmap the shared SRAM ECC registers
    expect_function_call(__wrap_atu_unmap);

    expect_function_call(__wrap_crash_dump_bug_check);

    g_s_arsm_ecc_isr((void*)&atu_entry);
}

TEST_FUNCTION(test_shared_sram_ecc_isr_ce, test_setup, nullptr)
{
    atu_map_entry_t atu_entry = ATU_MAPPING_SCP_S_ARSM_RAM_ECC(0);

    expect_function_call(__wrap_atu_map);

    // Read error status
    expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ADDRESS);
    will_return(__wrap_mmio_read32,
                SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_MASK |
                    SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK | 0x02);
    expect_function_call(__wrap_mmio_read32);

    // Read error address if applicable
    expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRADDR_ADDRESS);
    will_return(__wrap_mmio_read32, 0x12345678);
    expect_function_call(__wrap_mmio_read32);

    // Clear the error status
    expect_value(__wrap_mmio_write32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ADDRESS);
    expect_value(__wrap_mmio_write32,
                 data,
                 SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_MASK |
                     SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
    expect_function_call(__wrap_mmio_write32);

    expect_function_call(__wrap_hm_submit_cper);

    // Unmap the shared SRAM ECC registers
    expect_function_call(__wrap_atu_unmap);

    g_s_arsm_ecc_isr((void*)&atu_entry);
}

TEST_FUNCTION(test_shared_sram_ecc_isr_of, test_setup, nullptr)
{
    atu_map_entry_t atu_entry = ATU_MAPPING_SCP_S_ARSM_RAM_ECC(0);

    expect_function_call(__wrap_atu_map);

    // Read error status
    expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ADDRESS);
    will_return(__wrap_mmio_read32,
                SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_MASK |
                    SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK | 0x02);
    expect_function_call(__wrap_mmio_read32);

    // Read error address if applicable
    expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRADDR_ADDRESS);
    will_return(__wrap_mmio_read32, 0x12345678);
    expect_function_call(__wrap_mmio_read32);

    // Clear the error status
    expect_value(__wrap_mmio_write32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ADDRESS);
    expect_value(__wrap_mmio_write32,
                 data,
                 SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_MASK |
                     SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK);
    expect_function_call(__wrap_mmio_write32);

    expect_function_call(__wrap_hm_submit_cper);

    // Unmap the shared SRAM ECC registers
    expect_function_call(__wrap_atu_unmap);

    expect_function_call(__wrap_crash_dump_bug_check);

    g_s_arsm_ecc_isr((void*)&atu_entry);
}