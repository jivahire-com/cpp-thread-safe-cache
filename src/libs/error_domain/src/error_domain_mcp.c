//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_domain_mcp.c
 * This file contains scp ras related functionality.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FpFwUtils.h>
#include <atu_lib.h>
#include <bug_check.h>
#include <cper.h>
#include <error_domain_i.h>
#include <fpfw_icc_base.h>
#include <health_monitor_icc.h>
#include <idsw_kng.h> // for KNG_DIE_ID, idsw_get_die_id
#include <interrupts.h>
#include <mcp_exp_csr_regs.h>
#include <mcp_exp_top_regs.h>
#include <mscp_error_domain.h>
#include <mscp_exp_rmss_memory_map.h>
#include <mscp_ras_and_init_ctrl_registers_regs.h>
#include <nvic.h>
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <mcp_top_regs.h>
#include <scp_top_regs.h>
#include <shared_sram_ecc_ras_registers_regs.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MCP_PROC_FRU "MCP_PROC"
#define MCP_PROC_ERROR_DOMAIN_FRU_GUID_DEF                 \
    {                                                      \
        0x339505B3, 0x649B, 0x4F5F,                        \
        {                                                  \
            0xC1, 0x97, 0x42, 0x51, 0xD9, 0x5D, 0x65, 0xDB \
        }                                                  \
    }

/*-------- Function Prototypes -----------*/

/*-------------- Typedefs ----------------*/
typedef struct
{
    uint32_t err_source_id;
    uint32_t err_source_irq;
    acpi_error_severity_t err_severity;
    KNG_STATUS err_code;
    bool bugcheck_required;
    uint32_t* err_status_addr;
    uint32_t* err_address_addr;
    uint32_t err_status_mask;
} mcp_ecc_isr_params_t;

/*-- Declarations (Statics and globals) --*/
static fpfw_icc_base_ctx_t* mhu_handle = NULL;
static vptr_mscp_ras_and_init_ctrl_registers_reg mcp_ras_and_init_ctrl_registers_reg =
    (vptr_mscp_ras_and_init_ctrl_registers_reg)(MCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS);
static vptr_systemcontrol_reg mcp_system_control_reg =
    (vptr_systemcontrol_reg)(MCP_TOP_CORTEX_M7_ADDRESS + CORTEXM7INTEGRATIONCS_MCP_SYSTEMCONTROL_ADDRESS);

/*-------------- Functions ---------------*/
static void register_mcp_error_domain_complete_cb(void* context, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(status);
}

uint32_t get_irq_num_for_mcp_ecc_isr(mscp_arsm_ram_type_t type)
{
    const uint32_t irq_nums[MSCP_ARSM_RAM_COUNT] = {
        HW_INT_MCP_S_ARSM_ECC_FHI_INT,  // MCP_RT_ARSM_RAM
        HW_INT_MCP_NS_ARSM_ECC_FHI_INT, // MCP_NS_ARSM_RAM
        HW_INT_MCP_RT_ARSM_ECC_FHI_INT, // MCP_RT_ARSM_RAM
        HW_INT_MCP_RL_ARSM_ECC_FHI_INT  // MCP_RL_ARSM_RAM
    };

    return irq_nums[type];
}

static void mcp_cache_ecc_isr(const mcp_ecc_isr_params_t* params)
{
    // Save stored error info from DEBR0 and DEBR1
    uint32_t err_bank_register0 = MMIO_READ32(params->err_status_addr);
    uint32_t err_bank_register1 = MMIO_READ32(params->err_address_addr);

    // Clear interrupt source
    nvic_irq_clear_pending(params->err_source_irq);

    // Submit CPER
    acpi_err_sec_firmware_t sec_fw_cper_section = {.severity = params->err_severity,
                                                   .record_id = params->err_source_id,
                                                   .param = {err_bank_register0, err_bank_register1, params->err_code, 0}};

    acpi_cper_section_t cper_section;
    cper_section.sec_fw = sec_fw_cper_section;

    hm_submit_cper(ACPI_ERROR_DOMAIN_MCP_PROC, params->err_severity, &cper_section, sizeof(cper_section));

    if (params->bugcheck_required)
    {
        BUG_CHECK(params->err_code, err_bank_register0, err_bank_register1);
    }
}

static void mcp_ram_ecc_isr(const mcp_ecc_isr_params_t* params)
{
    uint32_t status = MMIO_READ32(params->err_status_addr);
    uint32_t address = MMIO_READ32(params->err_address_addr);

    // Clear interrupt source
    MMIO_SET_MASK32(params->err_status_addr, params->err_status_mask);
    nvic_irq_clear_pending(params->err_source_irq);

    if (status & params->err_status_mask)
    {
        // Submit CPER
        acpi_err_sec_firmware_t sec_fw_cper_section = {.severity = params->err_severity,
                                                       .record_id = params->err_source_id,
                                                       .param = {status, address, params->err_code, 0}};

        acpi_cper_section_t cper_section;
        cper_section.sec_fw = sec_fw_cper_section;

        hm_submit_cper(ACPI_ERROR_DOMAIN_MCP_PROC, params->err_severity, &cper_section, sizeof(cper_section));

        if (params->bugcheck_required)
        {
            BUG_CHECK(params->err_code, status, address);
        }
    }
}

static void tcm_overflow_isr()
{
    mcp_ecc_isr_params_t params = {.err_source_id = RECORD_ID_MCP_TCM,
                                   .err_source_irq = HW_INT_TCM_ECCOF_INT,
                                   .err_severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL,
                                   .err_code = KNG_HM_TCM_OF,
                                   .bugcheck_required = true,
                                   .err_status_addr = (uint32_t*)&mcp_ras_and_init_ctrl_registers_reg->tcmecc_errstatus,
                                   .err_address_addr = (uint32_t*)&mcp_ras_and_init_ctrl_registers_reg->tcmecc_erraddr,
                                   .err_status_mask = MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_OF_MASK};

    mcp_ram_ecc_isr(&params);
}

static void tcm_ce_isr()
{
    mcp_ecc_isr_params_t params = {.err_source_id = RECORD_ID_MCP_TCM,
                                   .err_source_irq = HW_INT_TCM_ECCCE_INT,
                                   .err_severity = ACPI_ERROR_SEVERITY_CORRECTED,
                                   .err_code = KNG_HM_TCM_CE,
                                   .bugcheck_required = false,
                                   .err_status_addr = (uint32_t*)&mcp_ras_and_init_ctrl_registers_reg->tcmecc_errstatus,
                                   .err_address_addr = (uint32_t*)&mcp_ras_and_init_ctrl_registers_reg->tcmecc_erraddr,
                                   .err_status_mask = MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_CE_MASK};

    mcp_ram_ecc_isr(&params);
}

static void tcm_ue_isr()
{
    mcp_ecc_isr_params_t params = {.err_source_id = RECORD_ID_MCP_TCM,
                                   .err_source_irq = HW_INT_TCM_ECCUE_INT,
                                   .err_severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL,
                                   .err_code = KNG_HM_TCM_UE,
                                   .bugcheck_required = true,
                                   .err_status_addr = (uint32_t*)&mcp_ras_and_init_ctrl_registers_reg->tcmecc_errstatus,
                                   .err_address_addr = (uint32_t*)&mcp_ras_and_init_ctrl_registers_reg->tcmecc_erraddr,
                                   .err_status_mask = MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_UE_MASK};

    mcp_ram_ecc_isr(&params);
}

void dcache_ce_isr()
{
    mcp_ecc_isr_params_t params = {.err_source_id = RECORD_ID_MCP_DCACHE,
                                   .err_source_irq = HW_INT_DCDET_DATA_CE,
                                   .err_severity = ACPI_ERROR_SEVERITY_CORRECTED,
                                   .err_code = KNG_HM_DCACHE_CE,
                                   .bugcheck_required = false,
                                   .err_status_addr = (uint32_t*)&mcp_system_control_reg->debr0h,
                                   .err_address_addr = (uint32_t*)&mcp_system_control_reg->debr1h};

    mcp_cache_ecc_isr(&params);
}

void dcache_ue_isr()
{
    mcp_ecc_isr_params_t params = {.err_source_id = RECORD_ID_MCP_DCACHE,
                                   .err_source_irq = HW_INT_DCDET_DATA_UE,
                                   .err_severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL,
                                   .err_code = KNG_HM_DCACHE_UE,
                                   .bugcheck_required = true,
                                   .err_status_addr = (uint32_t*)&mcp_system_control_reg->debr0h,
                                   .err_address_addr = (uint32_t*)&mcp_system_control_reg->debr1h};

    mcp_cache_ecc_isr(&params);
}

void dcache_tag_ue_isr()
{
    mcp_ecc_isr_params_t params = {.err_source_id = RECORD_ID_MCP_DCACHE,
                                   .err_source_irq = HW_INT_DCDET_TAG_UE,
                                   .err_severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL,
                                   .err_code = KNG_HM_DCACHE_TAG_UE,
                                   .bugcheck_required = true,
                                   .err_status_addr = (uint32_t*)&mcp_system_control_reg->debr0h,
                                   .err_address_addr = (uint32_t*)&mcp_system_control_reg->debr1h};

    mcp_cache_ecc_isr(&params);
}

void dcache_tag_ce_isr()
{
    mcp_ecc_isr_params_t params = {.err_source_id = RECORD_ID_MCP_DCACHE,
                                   .err_source_irq = HW_INT_DCDET_TAG_CE,
                                   .err_severity = ACPI_ERROR_SEVERITY_CORRECTED,
                                   .err_code = KNG_HM_DCACHE_TAG_CE,
                                   .bugcheck_required = false,
                                   .err_status_addr = (uint32_t*)&mcp_system_control_reg->debr0h,
                                   .err_address_addr = (uint32_t*)&mcp_system_control_reg->debr1h};

    mcp_cache_ecc_isr(&params);
}

void icache_ue_isr()
{
    mcp_ecc_isr_params_t params = {.err_source_id = RECORD_ID_MCP_ICACHE,
                                   .err_source_irq = HW_INT_ICDET_DATA_UE,
                                   .err_severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL,
                                   .err_code = KNG_HM_ICACHE_UE,
                                   .bugcheck_required = true,
                                   .err_status_addr = (uint32_t*)&mcp_system_control_reg->iebr0,
                                   .err_address_addr = (uint32_t*)&mcp_system_control_reg->iebr1h};

    mcp_cache_ecc_isr(&params);
}

void icache_ce_isr()
{
    mcp_ecc_isr_params_t params = {.err_source_id = RECORD_ID_MCP_ICACHE,
                                   .err_source_irq = HW_INT_ICDET_DATA_CE,
                                   .err_severity = ACPI_ERROR_SEVERITY_CORRECTED,
                                   .err_code = KNG_HM_ICACHE_CE,
                                   .bugcheck_required = false,
                                   .err_status_addr = (uint32_t*)&mcp_system_control_reg->iebr0,
                                   .err_address_addr = (uint32_t*)&mcp_system_control_reg->iebr1h};

    mcp_cache_ecc_isr(&params);
}

void icache_tag_ue_isr()
{
    mcp_ecc_isr_params_t params = {.err_source_id = RECORD_ID_MCP_ICACHE,
                                   .err_source_irq = HW_INT_ICDET_TAG_UE,
                                   .err_severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL,
                                   .err_code = KNG_HM_ICACHE_TAG_UE,
                                   .bugcheck_required = true,
                                   .err_status_addr = (uint32_t*)&mcp_system_control_reg->iebr0,
                                   .err_address_addr = (uint32_t*)&mcp_system_control_reg->iebr1h};

    mcp_cache_ecc_isr(&params);
}

void icache_tag_ce_isr()
{
    mcp_ecc_isr_params_t params = {.err_source_id = RECORD_ID_MCP_ICACHE,
                                   .err_source_irq = HW_INT_ICDET_TAG_CE,
                                   .err_severity = ACPI_ERROR_SEVERITY_CORRECTED,
                                   .err_code = KNG_HM_ICACHE_TAG_CE,
                                   .bugcheck_required = false,
                                   .err_status_addr = (uint32_t*)&mcp_system_control_reg->iebr0,
                                   .err_address_addr = (uint32_t*)&mcp_system_control_reg->iebr1h};

    mcp_cache_ecc_isr(&params);
}

static void enable_mcp_ecc_error()
{
    // // Enable TCM ECC errors
    MMIO_SET_MASK32(&mcp_ras_and_init_ctrl_registers_reg->tcmecc_errctrl.as_uint32,
                    MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_DTCMRAM_ECC_EN_MASK);
    MMIO_SET_MASK32(&mcp_ras_and_init_ctrl_registers_reg->tcmecc_errctrl.as_uint32,
                    MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_ITCMRAM_ECC_EN_MASK);

    // Enable ARSM ECC errors
    for (mscp_arsm_ram_type_t i = MSCP_S_ARSM_RAM; i < MSCP_ARSM_RAM_COUNT; i++)
    {
        atu_map_entry_t atu_entry;
        get_shared_sram_ecc_atu_entry(i, &atu_entry);
        int status = atu_map(ATU_ID_MSCP, &atu_entry);
        BUG_ASSERT(status == SILIBS_SUCCESS);

        uint32_t feature = MMIO_READ32(atu_entry.mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRFR_ADDRESS);
        uint32_t ctrl_mask = 0;
        if (feature & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRFR_ED_MASK) // Error reporting and logging
        {
            ctrl_mask |= SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRCTRL_ED_MASK; // Enable ECC error detection
        }

        if (feature & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRFR_FI_MASK) // Fault handling interrupt
        {
            ctrl_mask |= SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRCTRL_FI_MASK; // Fault handling interrupt is generated for all detected errors
        }

        if (feature & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRFR_UE_MASK) // In-band error response
        {
            ctrl_mask |= SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRCTRL_UE_MASK; // External abort response for uncorrected errors enabled
        }

        if (feature & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRFR_CFI_MASK) // Fault handling interrupt for corrected errors
        {
            ctrl_mask |= SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRCTRL_CFI_MASK; // Fault handling interrupt generated for corrected errors
        }

        // Enable ECC errors for the shared SRAM ECC RAS registers
        MMIO_SET_MASK32(atu_entry.mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRCTRL_ADDRESS, ctrl_mask);

        status = atu_unmap(ATU_ID_MSCP, &atu_entry);
        BUG_ASSERT(status == SILIBS_SUCCESS);
    }

    // Enable RSM ECC errors
    KNG_PLAT_ID plat = idsw_get_platform_sdv();

    if (plat != PLATFORM_SVP_SIM)
    {
        // ADO - Current SVP doesn't support RSM ECC
        enable_shared_sram_errors(get_rsm_ecc_atu_entry_wrapper, MSCP_RSM_RAM_COUNT);
    }
}

static void register_mcp_ecc_isr(uint32_t irq_num, isr_callback_fn_sans_params_t isr)
{
    nvic_status_t nvic_status = NVIC_STATUS_SUCCESS;

    nvic_status = nvic_irq_set_isr(irq_num, isr);
    BUG_ASSERT_PARAM(nvic_status == NVIC_STATUS_SUCCESS, nvic_status, irq_num);
    nvic_status = nvic_irq_clear_pending(irq_num);
    BUG_ASSERT_PARAM(nvic_status == NVIC_STATUS_SUCCESS, nvic_status, irq_num);
    nvic_status = nvic_irq_enable(irq_num);
    BUG_ASSERT_PARAM(nvic_status == NVIC_STATUS_SUCCESS, nvic_status, irq_num);
}

void register_mcp_ecc_isr_with_param(uint32_t irq_num, isr_callback_fn_with_params_t isr, void* isr_param)
{
    nvic_status_t nvic_status = NVIC_STATUS_SUCCESS;

    nvic_status = nvic_irq_set_isr_with_param(irq_num, isr, isr_param);
    BUG_ASSERT_PARAM(nvic_status == NVIC_STATUS_SUCCESS, nvic_status, irq_num);
    nvic_status = nvic_irq_clear_pending(irq_num);
    BUG_ASSERT_PARAM(nvic_status == NVIC_STATUS_SUCCESS, nvic_status, irq_num);
    nvic_status = nvic_irq_enable(irq_num);
    BUG_ASSERT_PARAM(nvic_status == NVIC_STATUS_SUCCESS, nvic_status, irq_num);
}

static void enable_mcp_ecc_interrupts()
{
    // // TCM ECC
    register_mcp_ecc_isr(HW_INT_TCM_ECCCE_INT, tcm_ce_isr);       // TCM CE
    register_mcp_ecc_isr(HW_INT_TCM_ECCUE_INT, tcm_ue_isr);       // TCM UE
    register_mcp_ecc_isr(HW_INT_TCM_ECCOF_INT, tcm_overflow_isr); // TCM overflow

    // ARSM ECC FHI
    KNG_DIE_ID die_id = idsw_get_die_id();
    register_mcp_ecc_isr_with_param(HW_INT_MCP_S_ARSM_ECC_FHI_INT,
                                    shared_sram_ecc_isr,
                                    (void*)&s_hm_arsm_atu_entries[die_id][MSCP_S_ARSM_RAM]); // Secure onchip shared ARSM RAM ECC FHI Interrupt for MCP accesses
    register_mcp_ecc_isr_with_param(HW_INT_MCP_NS_ARSM_ECC_FHI_INT,
                                    shared_sram_ecc_isr,
                                    (void*)&s_hm_arsm_atu_entries[die_id][MSCP_NS_ARSM_RAM]); // Non-secure onchip shared ARSM RAM ECC FHI Interrupt for MCP accesses
    register_mcp_ecc_isr_with_param(HW_INT_MCP_RT_ARSM_ECC_FHI_INT,
                                    shared_sram_ecc_isr,
                                    (void*)&s_hm_arsm_atu_entries[die_id][MSCP_RT_ARSM_RAM]); // Root onchip shared ARSM RAM ECC FHI Interrupt for MCP accesses
    register_mcp_ecc_isr_with_param(
        HW_INT_MCP_RL_ARSM_ECC_FHI_INT,
        shared_sram_ecc_isr,
        (void*)&s_hm_arsm_atu_entries[die_id][MSCP_RL_ARSM_RAM]); // Realm onchip shared ARSM RAM ECC FHI
                                                                  // Interrupt for MCP accesses RSM ECC FHI
    register_mcp_ecc_isr(HW_INT_MCP_RSM_RAM_FHI_INT, shared_sram_ecc_isr_ext); // MCP Secure&Non-Secure RSM RAM ECC

    // Data Cache ECC
    register_mcp_ecc_isr(HW_INT_DCDET_DATA_UE, dcache_ue_isr);    // D-cache Data RAM UE
    register_mcp_ecc_isr(HW_INT_DCDET_DATA_CE, dcache_ce_isr);    // D-cache Data RAM CE
    register_mcp_ecc_isr(HW_INT_DCDET_TAG_UE, dcache_tag_ue_isr); // D-cache Tag RAM UE
    register_mcp_ecc_isr(HW_INT_DCDET_TAG_CE, dcache_tag_ce_isr); // D-cache Tag RAM CE

    // Instruction Cache ECC
    register_mcp_ecc_isr(HW_INT_ICDET_DATA_UE, icache_ue_isr);    // I-cache Data RAM UE
    register_mcp_ecc_isr(HW_INT_ICDET_DATA_CE, icache_ce_isr);    // I-cache Data RAM CE
    register_mcp_ecc_isr(HW_INT_ICDET_TAG_UE, icache_tag_ue_isr); // I-cache Tag RAM UE
    register_mcp_ecc_isr(HW_INT_ICDET_TAG_CE, icache_tag_ce_isr); // I-cache Tag RAM CE
}

void register_mcp_error_domain(fpfw_icc_base_ctx_t* icc_ctx)
{
    BUG_ASSERT_PARAM(icc_ctx != NULL, icc_ctx, 0);
    mhu_handle = icc_ctx;

    static hm_mhu_error_domain_register_payload_t mcp_err_domain_register_payload = {0};
    mcp_err_domain_register_payload.header.msg_header.command = ICC_HM_ERROR_DOMAIN_REGISTER_MCP;
    mcp_err_domain_register_payload.header.msg_header.payload_size =
        sizeof(hm_mhu_error_domain_register_payload_t) - sizeof(icc_mhu_header_t);
    mcp_err_domain_register_payload.error_domain_idx = ACPI_ERROR_DOMAIN_MCP_PROC;
    mcp_err_domain_register_payload.valid_fru_id = 1;
    mcp_err_domain_register_payload.valid_fru_text = 1;
    mcp_err_domain_register_payload.fru_id = (guid_t)MCP_PROC_ERROR_DOMAIN_FRU_GUID_DEF;
    strncpy(mcp_err_domain_register_payload.fru_text, MCP_PROC_FRU, sizeof(mcp_err_domain_register_payload.fru_text) - 1);
    mcp_err_domain_register_payload.fru_text[sizeof(mcp_err_domain_register_payload.fru_text) - 1] = '\0';

    static fpfw_icc_base_send_req_t mcp_err_domain_register_req = {0};
    mcp_err_domain_register_req.payload_buffer = &mcp_err_domain_register_payload;
    mcp_err_domain_register_req.buffer_size = sizeof(mcp_err_domain_register_payload);
    mcp_err_domain_register_req.cb = register_mcp_error_domain_complete_cb;
    mcp_err_domain_register_req.cb_ctx = &mcp_err_domain_register_payload;

    fpfw_status_t status = fpfw_icc_base_send(icc_ctx, &mcp_err_domain_register_req);
    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, icc_ctx);
    enable_mcp_ecc_interrupts();
    enable_mcp_ecc_error();
}

fpfw_icc_base_ctx_t* get_mhu_handle(void)
{
    return (mhu_handle);
}
