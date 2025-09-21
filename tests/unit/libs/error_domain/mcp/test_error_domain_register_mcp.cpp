//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 @file test_err_domain_register_mcp_init.cpp
 * Tests the init of the mcp error domain
 */
/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {

#include <FpFwUtils.h> // for FPFW_UNUSED
#include <atu_api.h>
#include <fpfw_init.h> // for fpfw_init_result_t, fpfw_init_component_t
#include <health_monitor.h>
#include <health_monitor_icc.h>
#include <idsw_kng.h> // for KNG_DIE_ID, idsw_get_die_id
#include <interrupts.h>
#include <mscp_error_domain.h>
#include <nvic.h> // for nvic_status_t
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <mcp_top_regs.h>
#include <scp_top_regs.h>
#include <shared_sram_ecc_ras_registers_regs.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/
#define ARSM_RAM_DEFAULT_OFFSET (0x10)
#define RSM_RAM_DEFAULT_OFFSET  (0x08)
#define DTC_RAM_ADDRESS         (MCP_TOP_MCP_DATA_RAM_ADDRESS)
#define MCP_TCM_ERRCTRL_REG \
    (MCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_ADDRESS)
#define MCP_TCM_ERRCTRL_REG \
    (MCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_ADDRESS)
#define MCP_TCM_ERRSTATUS_REG \
    (MCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_ADDRESS)
#define MCP_TCM_ERRADDRESS_REG \
    (MCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRADDR_ADDRESS)

#define MCP_CORTEX_M7_SystemControl_ADDRESS \
    (MCP_TOP_CORTEX_M7_ADDRESS + CORTEXM7INTEGRATIONCS_MCP_SYSTEMCONTROL_ADDRESS)

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
void test_trigger_shared_sram_arsm_fault(uint32_t err_mask, uint32_t access_offset);
void test_trigger_shared_sram_rsm_fault(uint32_t err_mask, uint32_t access_offset);

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
isr_callback_fn_with_params_t g_s_arsm_ecc_isr = NULL;
isr_callback_fn_sans_params_t g_s_rsm_ecc_isr = NULL;
isr_callback_fn_sans_params_t g_dcache_ue_isr = NULL;
isr_callback_fn_sans_params_t g_dcache_ce_isr = NULL;
isr_callback_fn_sans_params_t g_dcache_tag_ue_isr = NULL;
isr_callback_fn_sans_params_t g_dcache_tag_ce_isr = NULL;
isr_callback_fn_sans_params_t g_icache_ue_isr = NULL;
isr_callback_fn_sans_params_t g_icache_ce_isr = NULL;
isr_callback_fn_sans_params_t g_icache_tag_ue_isr = NULL;
isr_callback_fn_sans_params_t g_icache_tag_ce_isr = NULL;

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
    case HW_INT_MCP_SCFRAM_ECCOF_INT:
        g_scfram_of_isr = isr;
        break;
    case HW_INT_MCP_SCFRAM_ECCCE_INT:
        g_scfram_ce_isr = isr;
        break;
    case HW_INT_MCP_RAM0_ECCOF_INT:
        g_ram0_of_isr = isr;
        break;
    case HW_INT_MCP_RAM0_ECCCE_INT:
        g_ram0_ce_isr = isr;
        break;
    case HW_INT_MCP_RAM1_ECCOF_INT:
        g_ram1_of_isr = isr;
        break;
    case HW_INT_MCP_RAM1_ECCCE_INT:
        g_ram1_ce_isr = isr;
        break;

    case HW_INT_TCM_ECCCE_INT:
        g_tcm_ce_isr = isr;
        break;
    case HW_INT_TCM_ECCUE_INT:
        g_tcm_ue_isr = isr;
        break;
    case HW_INT_TCM_ECCOF_INT:
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
    case MCP_ERROR_TYPE_BUS_FAULT:
        test_bus_fault_handler();
        break;
    case MCP_ERROR_TYPE_S_ARSM_CE:
    case MCP_ERROR_TYPE_NS_ARSM_CE:
    case MCP_ERROR_TYPE_RT_ARSM_CE:
    case MCP_ERROR_TYPE_RL_ARSM_CE:
        test_trigger_shared_sram_arsm_fault(SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK,
                                            MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_RAM_DEFAULT_OFFSET);
        break;

    case MCP_ERROR_TYPE_S_ARSM_UE:
    case MCP_ERROR_TYPE_NS_ARSM_UE:
    case MCP_ERROR_TYPE_RL_ARSM_UE:
        test_trigger_shared_sram_arsm_fault(SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK,
                                            MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_RAM_DEFAULT_OFFSET);
        break;

    case MCP_ERROR_TYPE_S_ARSM_OVERFLOW:
    case MCP_ERROR_TYPE_NS_ARSM_OVERFLOW:
    case MCP_ERROR_TYPE_RT_ARSM_OVERFLOW:
    case MCP_ERROR_TYPE_RL_ARSM_OVERFLOW:
        test_trigger_shared_sram_arsm_fault(SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK,
                                            MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_RAM_DEFAULT_OFFSET);
        break;
    case MCP_ERROR_TYPE_RSM_RAM_CE:
        test_trigger_shared_sram_rsm_fault(SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK,
                                           MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_RAM_DEFAULT_OFFSET);
        break;

    case MCP_ERROR_TYPE_RSM_RAM_UE:
        test_trigger_shared_sram_rsm_fault(SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK,
                                           MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_RAM_DEFAULT_OFFSET);
        break;

    case HW_INT_MCP_RSM_RAM_FHI_INT:
        g_s_rsm_ecc_isr = isr;
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
    case HW_INT_MCP_S_ARSM_ECC_FHI_INT:
    case HW_INT_MCP_NS_ARSM_ECC_FHI_INT:
    case HW_INT_MCP_RT_ARSM_ECC_FHI_INT:
    case HW_INT_MCP_RL_ARSM_ECC_FHI_INT:
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

KNG_PLAT_ID __wrap_idsw_get_platform_sdv()
{
    return mock_type(KNG_PLAT_ID);
}

bool __wrap_is_cached_space(uint32_t addr)
{
    FPFW_UNUSED(addr);
    // This function is used to determine if the address is in cached space.
    return mock_type(bool);
}
}

//
// Tests
//

TEST_FUNCTION(test_register_mcp_error_domain, nullptr, nullptr)
{
    expect_function_call(__wrap_fpfw_icc_base_send);
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);

    // SCF RAM ECC
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_MCP_SCFRAM_ECCOF_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_MCP_SCFRAM_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_MCP_SCFRAM_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_MCP_SCFRAM_ECCCE_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_MCP_SCFRAM_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_MCP_SCFRAM_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_enable);

    // Boot RAM ECC
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_MCP_RAM0_ECCOF_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_MCP_RAM0_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_MCP_RAM0_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_MCP_RAM0_ECCCE_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_MCP_RAM0_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_MCP_RAM0_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_MCP_RAM1_ECCOF_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_MCP_RAM1_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_MCP_RAM1_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_MCP_RAM1_ECCCE_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_MCP_RAM1_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_MCP_RAM1_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_enable);

    // TCM ECC
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_TCM_ECCCE_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_TCM_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_TCM_ECCCE_INT);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_TCM_ECCUE_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_TCM_ECCUE_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_TCM_ECCUE_INT);
    expect_function_call(__wrap_nvic_irq_enable);
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_TCM_ECCOF_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_TCM_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_TCM_ECCOF_INT);
    expect_function_call(__wrap_nvic_irq_enable);

    expect_function_call(__wrap_nvic_irq_set_isr_with_param); // HW_INT_MCP_S_ARSM_ECC_FHI_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_MCP_S_ARSM_ECC_FHI_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_MCP_S_ARSM_ECC_FHI_INT);
    expect_function_call(__wrap_nvic_irq_enable);

    expect_function_call(__wrap_nvic_irq_set_isr_with_param); // HW_INT_MCP_NS_ARSM_ECC_FHI_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_MCP_NS_ARSM_ECC_FHI_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_MCP_NS_ARSM_ECC_FHI_INT);
    expect_function_call(__wrap_nvic_irq_enable);

    expect_function_call(__wrap_nvic_irq_set_isr_with_param); // HW_INT_MCP_RT_ARSM_ECC_FHI_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_MCP_RT_ARSM_ECC_FHI_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_MCP_RT_ARSM_ECC_FHI_INT);
    expect_function_call(__wrap_nvic_irq_enable);

    expect_function_call(__wrap_nvic_irq_set_isr_with_param); // HW_INT_MCP_RL_ARSM_ECC_FHI_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_MCP_RL_ARSM_ECC_FHI_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_MCP_RL_ARSM_ECC_FHI_INT);
    expect_function_call(__wrap_nvic_irq_enable);

    // RSM ECC Interrupt expectations
    expect_function_call(__wrap_nvic_irq_set_isr); // HW_INT_MCP_RSM_RAM_FHI_INT
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, HW_INT_MCP_RSM_RAM_FHI_INT);
    expect_function_call(__wrap_nvic_irq_clear_pending);
    expect_value(__wrap_nvic_irq_enable, irq_num, HW_INT_MCP_RSM_RAM_FHI_INT);
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

    // Shared SRAM ECC
    for (int i = MSCP_S_ARSM_RAM; i < MSCP_ARSM_RAM_COUNT; i++)
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

    // RSM ECC
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    for (int i = MSCP_S_RSM_RAM; i < MSCP_RSM_RAM_COUNT; i++)
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

    register_mcp_error_domain((fpfw_icc_base_ctx_t*)1234);
}

static int test_setup(void** ctx)
{
    FPFW_UNUSED(ctx);

    if (g_err_inject_cb == NULL || g_scfram_of_isr == NULL || g_scfram_ce_isr == NULL ||
        g_ram0_of_isr == NULL || g_ram0_ce_isr == NULL || g_ram1_of_isr == NULL || g_ram1_ce_isr == NULL ||
        g_tcm_ce_isr == NULL || g_tcm_ue_isr == NULL || g_tcm_of_isr == NULL || g_s_arsm_ecc_isr == NULL ||
        g_s_rsm_ecc_isr == NULL || g_dcache_ue_isr == NULL || g_dcache_ce_isr == NULL ||
        g_dcache_tag_ue_isr == NULL || g_dcache_tag_ce_isr == NULL || g_icache_ue_isr == NULL ||
        g_icache_ce_isr == NULL || g_icache_tag_ue_isr == NULL || g_icache_tag_ce_isr == NULL)
    {
        test_register_mcp_error_domain(NULL);
    }

    return 0;
}

void test_trigger_shared_sram_arsm_fault(uint32_t err_mask, uint32_t access_offset)
{
    FPFW_UNUSED(access_offset);
    expect_function_call(__wrap_atu_map);

    if (err_mask & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK)
    {
        expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_ADDRESS);
        will_return(__wrap_mmio_read32, 0);
        expect_function_call(__wrap_mmio_read32);
        expect_value(__wrap_mmio_write32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_ADDRESS);
        expect_value(__wrap_mmio_write32, data, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_CE_MASK);
        expect_function_call(__wrap_mmio_write32);

        expect_any(__wrap_mmio_read32, addr);
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
        expect_any(__wrap_mmio_read32, addr);

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

        expect_any(__wrap_mmio_read32, addr);
        will_return(__wrap_mmio_read32, 0);
        expect_function_call(__wrap_mmio_read32);

        expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_ADDRESS);
        will_return(__wrap_mmio_read32, 0);
        expect_function_call(__wrap_mmio_read32);
        expect_any(__wrap_mmio_write32, addr);
        expect_value(__wrap_mmio_write32, data, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_UE_MASK);
        expect_function_call(__wrap_mmio_write32);

        expect_any(__wrap_mmio_read32, addr);
        will_return(__wrap_mmio_read32, 0);
        expect_function_call(__wrap_mmio_read32);

        expect_function_call(__wrap_nvic_global_enable);
    }

    expect_function_call(__wrap_atu_unmap);
}

void test_trigger_shared_sram_rsm_fault(uint32_t err_mask, uint32_t access_offset)
{
    FPFW_UNUSED(access_offset);
    expect_function_call(__wrap_atu_map);

    if (err_mask & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK)
    {
        expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_ADDRESS);
        will_return(__wrap_mmio_read32, 0);
        expect_function_call(__wrap_mmio_read32);
        expect_value(__wrap_mmio_write32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_ADDRESS);
        expect_value(__wrap_mmio_write32, data, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_CE_MASK);
        expect_function_call(__wrap_mmio_write32);

        expect_any(__wrap_mmio_read32, addr);
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
        expect_any(__wrap_mmio_read32, addr);

        will_return(__wrap_mmio_read32, 0);
        expect_function_call(__wrap_mmio_read32);
    }
    expect_function_call(__wrap_atu_unmap);
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
            will_return(__wrap_is_cached_space, false);
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
            will_return(__wrap_is_cached_space, false);
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
            will_return(__wrap_is_cached_space, false);
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

            will_return(__wrap_is_cached_space, false);
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

        case MCP_ERROR_TYPE_DATA_CACHE_CE:
            test_dcache_ce_isr();
            break;
        case MCP_ERROR_TYPE_DATA_CACHE_UE:
            test_dcache_ue_isr();
            break;
        case MCP_ERROR_TYPE_DATA_CACHE_TAG_CE:
            test_dcache_tag_ce_isr();
            break;
        case MCP_ERROR_TYPE_DATA_CACHE_TAG_UE:
            test_dcache_tag_ue_isr();
            break;
        case MCP_ERROR_TYPE_INSTRUCTION_CACHE_CE:
            test_icache_ce_isr();
            break;
        case MCP_ERROR_TYPE_INSTRUCTION_CACHE_UE:
            test_icache_ue_isr();
            break;
        case MCP_ERROR_TYPE_INSTRUCTION_CACHE_TAG_CE:
            test_icache_tag_ce_isr();
            break;
        case MCP_ERROR_TYPE_INSTRUCTION_CACHE_TAG_UE:
            test_icache_tag_ue_isr();
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
        case MCP_ERROR_TYPE_S_ARSM_CE:
        case MCP_ERROR_TYPE_NS_ARSM_CE:
        case MCP_ERROR_TYPE_RT_ARSM_CE:
        case MCP_ERROR_TYPE_RL_ARSM_CE:
            will_return(__wrap_is_cached_space, false);
            test_trigger_shared_sram_arsm_fault(SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK,
                                                MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_RAM_DEFAULT_OFFSET);
            break;
        case MCP_ERROR_TYPE_S_ARSM_UE:
        case MCP_ERROR_TYPE_NS_ARSM_UE:
        case MCP_ERROR_TYPE_RT_ARSM_UE:
        case MCP_ERROR_TYPE_RL_ARSM_UE:
            will_return_count(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON, 3);
            will_return(__wrap_is_cached_space, false);
            test_trigger_shared_sram_arsm_fault(SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK,
                                                MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_RAM_DEFAULT_OFFSET);
            break;
        case MCP_ERROR_TYPE_S_ARSM_OVERFLOW:
        case MCP_ERROR_TYPE_NS_ARSM_OVERFLOW:
        case MCP_ERROR_TYPE_RT_ARSM_OVERFLOW:
        case MCP_ERROR_TYPE_RL_ARSM_OVERFLOW:
            will_return(__wrap_is_cached_space, false);
            will_return_count(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON, 3);
            will_return(__wrap_is_cached_space, false);
            test_trigger_shared_sram_arsm_fault(SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK,
                                                MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_RAM_DEFAULT_OFFSET);
            break;
        case MCP_ERROR_TYPE_RSM_RAM_CE: {
            uint32_t mapped_rsm_addr = (uint32_t)(uintptr_t)mapped_region;
            will_return(__wrap_is_cached_space, false);
            expect_function_call(__wrap_atu_map);
            test_trigger_shared_sram_rsm_fault(SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK,
                                               mapped_rsm_addr + RSM_RAM_DEFAULT_OFFSET);
            expect_function_call(__wrap_atu_unmap);
            break;
        }
        case MCP_ERROR_TYPE_RSM_RAM_UE: {
            uint32_t mapped_rsm_addr2 = (uint32_t)(uintptr_t)mapped_region;
            will_return(__wrap_is_cached_space, false);
            will_return_count(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON, 3);
            expect_function_call(__wrap_atu_map);
            test_trigger_shared_sram_rsm_fault(SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK,
                                               mapped_rsm_addr2 + RSM_RAM_DEFAULT_OFFSET);
            expect_function_call(__wrap_atu_unmap);
            break;
        }

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
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, 0);
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

TEST_FUNCTION(test_mcp_error_injection_handler_9, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, MCP_ERROR_TYPE_S_ARSM_CE);
}

TEST_FUNCTION(test_mcp_error_injection_handler_10, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, MCP_ERROR_TYPE_NS_ARSM_CE);
}

TEST_FUNCTION(test_mcp_error_injection_handler_11, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, MCP_ERROR_TYPE_RT_ARSM_CE);
}

TEST_FUNCTION(test_mcp_error_injection_handler_12, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, MCP_ERROR_TYPE_RL_ARSM_CE);
}

TEST_FUNCTION(test_mcp_error_injection_handler_13, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, MCP_ERROR_TYPE_S_ARSM_UE);
}

TEST_FUNCTION(test_mcp_error_injection_handler_14, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, MCP_ERROR_TYPE_NS_ARSM_UE);
}

TEST_FUNCTION(test_mcp_error_injection_handler_15, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, MCP_ERROR_TYPE_RT_ARSM_UE);
}

TEST_FUNCTION(test_mcp_error_injection_handler_16, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, MCP_ERROR_TYPE_RL_ARSM_UE);
}

TEST_FUNCTION(test_mcp_error_injection_handler_17, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, MCP_ERROR_TYPE_S_ARSM_OVERFLOW);
}

TEST_FUNCTION(test_mcp_error_injection_handler_18, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, MCP_ERROR_TYPE_NS_ARSM_OVERFLOW);
}

TEST_FUNCTION(test_mcp_error_injection_handler_19, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, MCP_ERROR_TYPE_RT_ARSM_OVERFLOW);
}

TEST_FUNCTION(test_mcp_error_injection_handler_20, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, MCP_ERROR_TYPE_RL_ARSM_OVERFLOW);
}

TEST_FUNCTION(test_mcp_error_injection_handler_21, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, MCP_ERROR_TYPE_RSM_RAM_CE);
}

TEST_FUNCTION(test_mcp_error_injection_handler_22, test_setup, nullptr)
{
    test_mcp_error_injection_handler(ACPI_ERROR_DOMAIN_MCP_PROC, MCP_ERROR_TYPE_RSM_RAM_UE);
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

// Test function to check the behavior of the Data Cache ECC UE ISR
void test_dcache_ue_isr()
{
    // Read the DEBR0
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_DEBR0H_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Read the DEBR1
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_DEBR1H_ADDRESS);
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
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_DEBR0H_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Read the DEBR1
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_DEBR1H_ADDRESS);
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
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_DEBR0H_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Read the DEBR1
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_DEBR1H_ADDRESS);
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
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_DEBR0H_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Read the DEBR1
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_DEBR1H_ADDRESS);
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
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_IEBR0_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Read the DEBR1
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_IEBR1H_ADDRESS);
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
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_IEBR0_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Read the DEBR1
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_IEBR1H_ADDRESS);
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
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_IEBR0_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Read the DEBR1
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_IEBR1H_ADDRESS);
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
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_IEBR0_ADDRESS);
    will_return(__wrap_mmio_read32, 0);
    expect_function_call(__wrap_mmio_read32);

    // Read the DEBR1
    expect_value(__wrap_mmio_read32, addr, (uint32_t)MCP_CORTEX_M7_SystemControl_ADDRESS + SYSTEMCONTROL_IEBR1H_ADDRESS);
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

TEST_FUNCTION(test_rsm_ecc_isr_ce, test_setup, nullptr)
{
    for (int i = MSCP_S_RSM_RAM; i < MSCP_RSM_RAM_COUNT; i++)
    {
        // Map the shared SRAM ECC registers
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
                     SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK |
                         SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_MASK |
                         SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
        expect_function_call(__wrap_mmio_write32);

        expect_function_call(__wrap_hm_submit_cper);

        // Unmap the shared SRAM ECC registers
        expect_function_call(__wrap_atu_unmap);
    }

    g_s_rsm_ecc_isr();
}

TEST_FUNCTION(test_rsm_ecc_isr_ue, test_setup, nullptr)
{
    for (int i = MSCP_S_RSM_RAM; i < MSCP_RSM_RAM_COUNT; i++)
    {
        // Map the shared SRAM ECC registers
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
                     SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK |
                         SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_MASK |
                         SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
        expect_function_call(__wrap_mmio_write32);

        expect_function_call(__wrap_hm_submit_cper);

        // Unmap the shared SRAM ECC registers
        expect_function_call(__wrap_atu_unmap);

        expect_function_call(__wrap_crash_dump_bug_check);
    }

    g_s_rsm_ecc_isr();
}
