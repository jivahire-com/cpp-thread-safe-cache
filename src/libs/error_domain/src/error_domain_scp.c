//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_domain_scp.c
 * This file contains scp ras related functionality.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <cper.h>
#include <health_monitor.h>
#include <interrupts.h>
#include <mscp_error_domain.h>
#include <mscp_ras_and_init_ctrl_registers_regs.h>
#include <nvic.h>
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <scp_top_regs.h>

/*-- Symbolic Constant Macros (defines) --*/
#define SCP_PROC_FRU "SCP_PROC"
#define SCP_PROC_ERROR_DOMAIN_FRU_GUID_DEF                 \
    {                                                      \
        0xedf48164, 0x1d29, 0x47dc,                        \
        {                                                  \
            0xa7, 0x25, 0x8d, 0x8b, 0x79, 0xbb, 0xfc, 0x78 \
        }                                                  \
    }

#define MMIO_SET_MASK32(addr, mask)   MMIO_WRITE32(addr, (MMIO_READ32(addr) | (mask)))
#define MMIO_CLEAR_MASK32(addr, mask) MMIO_WRITE32(addr, (MMIO_READ32(addr) & ~(mask)))
/*-------- Function Prototypes -----------*/

/*-------------- Typedefs ----------------*/
typedef struct
{
    guid_t err_source_id;
    uint32_t err_source_irq;
    acpi_error_severity_t err_severity;
    KNG_STATUS err_code;
    bool bugcheck_required;
    uint32_t* err_status_addr;
    uint32_t* err_address_addr;
    uint32_t err_status_mask;
} mscp_ecc_isr_params_t;

/*-- Declarations (Statics and globals) --*/
static guid_t SCP_ERROR_DOMAIN_GUID = SCP_PROC_ERROR_DOMAIN_FRU_GUID_DEF;
static vptr_scp_exp_csr_reg scp_exp_csr_regs =
    (vptr_scp_exp_csr_reg)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS);
static vptr_mscp_ras_and_init_ctrl_registers_reg scp_ras_and_init_ctrl_registers_reg =
    (vptr_mscp_ras_and_init_ctrl_registers_reg)(SCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS);

/*-------------- Functions ---------------*/
static void ram_ecc_isr(const mscp_ecc_isr_params_t* params)
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

        hm_submit_cper(ACPI_ERROR_DOMAIN_SCP_PROC, params->err_severity, &cper_section, sizeof(cper_section));

        if (params->bugcheck_required)
        {
            BUG_CHECK(params->err_code, status, address);
        }
    }
}

static void rmss_scfram_ecc_ce_isr()
{
    mscp_ecc_isr_params_t params = {.err_source_id = (guid_t)SCP_SCF_RAM,
                                    .err_source_irq = HW_INT_SCP_SCFRAM_ECCCE_INT,
                                    .err_severity = ACPI_ERROR_SEVERITY_CORRECTED,
                                    .err_code = KNG_HM_SCF_CE,
                                    .bugcheck_required = false,
                                    .err_status_addr = (uint32_t*)&scp_exp_csr_regs->scfram_scp_errstatus_reg,
                                    .err_address_addr = (uint32_t*)&scp_exp_csr_regs->scfram_scp_erraddr_reg,
                                    .err_status_mask = SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_CE_MASK};

    ram_ecc_isr(&params);
}

static void rmss_scfram_ecc_of_isr()
{
    mscp_ecc_isr_params_t params = {.err_source_id = (guid_t)SCP_SCF_RAM,
                                    .err_source_irq = HW_INT_SCP_SCFRAM_ECCOF_INT,
                                    .err_severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL,
                                    .err_code = KNG_HM_SCF_OF,
                                    .bugcheck_required = true,
                                    .err_status_addr = (uint32_t*)&scp_exp_csr_regs->scfram_scp_errstatus_reg,
                                    .err_address_addr = (uint32_t*)&scp_exp_csr_regs->scfram_scp_erraddr_reg,
                                    .err_status_mask = SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_OF_MASK};

    ram_ecc_isr(&params);
}

static void rmss_ram0_ecc_ce_isr()
{
    mscp_ecc_isr_params_t params = {.err_source_id = (guid_t)SCP_RMSS_RAM0,
                                    .err_source_irq = HW_INT_SCP_RAM0_ECCCE_INT,
                                    .err_severity = ACPI_ERROR_SEVERITY_CORRECTED,
                                    .err_code = KNG_HM_RMSS_RAM0_CE,
                                    .bugcheck_required = false,
                                    .err_status_addr = (uint32_t*)&scp_exp_csr_regs->rmss_ram0_scp_errstatus_reg,
                                    .err_address_addr = (uint32_t*)&scp_exp_csr_regs->rmss_ram0_scp_erraddr_reg,
                                    .err_status_mask = SCP_EXP_CSR_RMSS_RAM0_SCP_ERRSTATUS_REG_CE_MASK};

    ram_ecc_isr(&params);
}

static void rmss_ram0_ecc_of_isr()
{
    mscp_ecc_isr_params_t params = {.err_source_id = (guid_t)SCP_RMSS_RAM0,
                                    .err_source_irq = HW_INT_SCP_RAM0_ECCOF_INT,
                                    .err_severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL,
                                    .err_code = KNG_HM_RMSS_RAM0_OF,
                                    .bugcheck_required = true,
                                    .err_status_addr = (uint32_t*)&scp_exp_csr_regs->rmss_ram0_scp_errstatus_reg,
                                    .err_address_addr = (uint32_t*)&scp_exp_csr_regs->rmss_ram0_scp_erraddr_reg,
                                    .err_status_mask = SCP_EXP_CSR_RMSS_RAM0_SCP_ERRSTATUS_REG_OF_MASK};

    ram_ecc_isr(&params);
}

static void rmss_ram1_ecc_ce_isr()
{
    mscp_ecc_isr_params_t params = {.err_source_id = (guid_t)SCP_RMSS_RAM1,
                                    .err_source_irq = HW_INT_SCP_RAM1_ECCCE_INT,
                                    .err_severity = ACPI_ERROR_SEVERITY_CORRECTED,
                                    .err_code = KNG_HM_RMSS_RAM1_CE,
                                    .bugcheck_required = false,
                                    .err_status_addr = (uint32_t*)&scp_exp_csr_regs->rmss_ram1_scp_errstatus_reg,
                                    .err_address_addr = (uint32_t*)&scp_exp_csr_regs->rmss_ram1_scp_erraddr_reg,
                                    .err_status_mask = SCP_EXP_CSR_RMSS_RAM1_SCP_ERRSTATUS_REG_CE_MASK};

    ram_ecc_isr(&params);
}

static void rmss_ram1_ecc_of_isr()
{
    mscp_ecc_isr_params_t params = {.err_source_id = (guid_t)SCP_RMSS_RAM1,
                                    .err_source_irq = HW_INT_SCP_RAM1_ECCOF_INT,
                                    .err_severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL,
                                    .err_code = KNG_HM_RMSS_RAM1_OF,
                                    .bugcheck_required = true,
                                    .err_status_addr = (uint32_t*)&scp_exp_csr_regs->rmss_ram1_scp_errstatus_reg,
                                    .err_address_addr = (uint32_t*)&scp_exp_csr_regs->rmss_ram1_scp_erraddr_reg,
                                    .err_status_mask = SCP_EXP_CSR_RMSS_RAM1_SCP_ERRSTATUS_REG_OF_MASK};

    ram_ecc_isr(&params);
}

static void tcm_overflow_isr()
{
    mscp_ecc_isr_params_t params = {.err_source_id = (guid_t)SCP_TCM,
                                    .err_source_irq = HW_INT_SCP_TCM_ECCOF_INT,
                                    .err_severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL,
                                    .err_code = KNG_HM_TCM_OF,
                                    .bugcheck_required = true,
                                    .err_status_addr = (uint32_t*)&scp_ras_and_init_ctrl_registers_reg->tcmecc_errstatus,
                                    .err_address_addr = (uint32_t*)&scp_ras_and_init_ctrl_registers_reg->tcmecc_erraddr,
                                    .err_status_mask = MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_OF_MASK};

    ram_ecc_isr(&params);
}

static void tcm_ce_isr()
{
    mscp_ecc_isr_params_t params = {.err_source_id = (guid_t)SCP_TCM,
                                    .err_source_irq = HW_INT_SCP_TCM_ECCCE_INT,
                                    .err_severity = ACPI_ERROR_SEVERITY_CORRECTED,
                                    .err_code = KNG_HM_TCM_CE,
                                    .bugcheck_required = false,
                                    .err_status_addr = (uint32_t*)&scp_ras_and_init_ctrl_registers_reg->tcmecc_errstatus,
                                    .err_address_addr = (uint32_t*)&scp_ras_and_init_ctrl_registers_reg->tcmecc_erraddr,
                                    .err_status_mask = MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_CE_MASK};

    ram_ecc_isr(&params);
}

static void tcm_ue_isr()
{
    mscp_ecc_isr_params_t params = {.err_source_id = (guid_t)SCP_TCM,
                                    .err_source_irq = HW_INT_SCP_TCM_ECCUE_INT,
                                    .err_severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL,
                                    .err_code = KNG_HM_TCM_UE,
                                    .bugcheck_required = true,
                                    .err_status_addr = (uint32_t*)&scp_ras_and_init_ctrl_registers_reg->tcmecc_errstatus,
                                    .err_address_addr = (uint32_t*)&scp_ras_and_init_ctrl_registers_reg->tcmecc_erraddr,
                                    .err_status_mask = MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_UE_MASK};

    ram_ecc_isr(&params);
}

static void enable_scp_ecc_error()
{
    // Enable SCF ECC errors
    MMIO_SET_MASK32(&scp_exp_csr_regs->scfram_errctrl_reg, SCP_EXP_CSR_SCFRAM_ERRCTRL_REG_ECC_EN_MASK);

    // Enable MSCP EXP RAM0 ECC errors
    MMIO_SET_MASK32(&scp_exp_csr_regs->rmss_ram0_errctrl_reg, SCP_EXP_CSR_RMSS_RAM0_ERRCTRL_REG_ECC_EN_MASK);

    // Enable MSCP EXP RAM1 ECC errors
    MMIO_SET_MASK32(&scp_exp_csr_regs->rmss_ram1_errctrl_reg, SCP_EXP_CSR_RMSS_RAM1_ERRCTRL_REG_ECC_EN_MASK);

    // Enable TCM ECC errors
    MMIO_SET_MASK32(&scp_ras_and_init_ctrl_registers_reg->tcmecc_errctrl.as_uint32,
                    MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_DTCMRAM_ECC_EN_MASK);
    MMIO_SET_MASK32(&scp_ras_and_init_ctrl_registers_reg->tcmecc_errctrl.as_uint32,
                    MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_ITCMRAM_ECC_EN_MASK);
}

static void register_scp_ecc_isr(uint32_t irq_num, isr_callback_fn_sans_params_t isr)
{
    nvic_status_t nvic_status = NVIC_STATUS_SUCCESS;

    nvic_status = nvic_irq_set_isr(irq_num, isr);
    BUG_ASSERT_PARAM(nvic_status == NVIC_STATUS_SUCCESS, nvic_status, irq_num);
    nvic_status = nvic_irq_enable(irq_num);
    BUG_ASSERT_PARAM(nvic_status == NVIC_STATUS_SUCCESS, nvic_status, irq_num);
}

static void enable_scp_ecc_interrupts()
{
    // SCF RAM ECC
    register_scp_ecc_isr(HW_INT_SCP_SCFRAM_ECCOF_INT, rmss_scfram_ecc_of_isr); // SCF RAM overflow
    register_scp_ecc_isr(HW_INT_SCP_SCFRAM_ECCCE_INT, rmss_scfram_ecc_ce_isr); // SCF RAM CE

    // Boot RAM ECC
    register_scp_ecc_isr(HW_INT_SCP_RAM0_ECCOF_INT, rmss_ram0_ecc_of_isr); // RMSS RAM0 overflow
    register_scp_ecc_isr(HW_INT_SCP_RAM0_ECCCE_INT, rmss_ram0_ecc_ce_isr); // RMSS RAM0 CE

    register_scp_ecc_isr(HW_INT_SCP_RAM1_ECCOF_INT, rmss_ram1_ecc_of_isr); // RMSS RAM1 overflow
    register_scp_ecc_isr(HW_INT_SCP_RAM1_ECCCE_INT, rmss_ram1_ecc_ce_isr); // RMSS RAM1 CE

    // TCM ECC
    register_scp_ecc_isr(HW_INT_SCP_TCM_ECCCE_INT, tcm_ce_isr);       // TCM CE
    register_scp_ecc_isr(HW_INT_SCP_TCM_ECCUE_INT, tcm_ue_isr);       // TCM UE
    register_scp_ecc_isr(HW_INT_SCP_TCM_ECCOF_INT, tcm_overflow_isr); // TCM overflow
}

void register_scp_error_domain()
{
    // Enable ECC error injection
    enable_scp_ecc_error();

    // Register the error domain
    hm_register_error_domain(ACPI_ERROR_DOMAIN_SCP_PROC, &SCP_ERROR_DOMAIN_GUID, SCP_PROC_FRU, scp_error_injection_handler, NULL);

    // Register the error interrupt handlers
    enable_scp_ecc_interrupts();
}