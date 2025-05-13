//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_domain_register_scp.c
 * This file contains scp ras related functionality.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FpFwUtils.h>
#include <arm_intrinsic.h> // for __DSB on Windows builds (empty define)
#include <bug_check.h>
#include <cper.h>
#include <health_monitor.h>
#include <interrupts.h>
#include <nvic.h>
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <scp_top_regs.h>
#include <stdio.h>

// clang-format off
#include <cmsis_m7.h>
#ifdef PLATFORM_CACHING_ENABLED
#include <cmsis_compiler.h>
#include <m-profile/armv7m_cachel1.h>
#endif
// clang-format on

/*-- Symbolic Constant Macros (defines) --*/
#define MASK_CE 0x20
#define MASK_UE 0x40

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

#define SCF_RAM_ADDRESS   (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_RAM_ADDRESS)
#define RMSS_RAM0_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS)
#define RMSS_RAM1_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS)

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static guid_t SCP_ERROR_DOMAIN_GUID = SCP_PROC_ERROR_DOMAIN_FRU_GUID_DEF;
static vptr_scp_exp_csr_reg scp_exp_csr_regs =
    (vptr_scp_exp_csr_reg)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS);

/*-------------- Functions ---------------*/
static void ram_ecc_isr(uint32_t* status_addr, uint32_t* address_addr, uint32_t status_mask, KNG_STATUS err, bool bugcheck)
{
    uint32_t status = MMIO_READ32(status_addr);
    uint32_t address = MMIO_READ32(address_addr);

    // Clear interrupt source
    MMIO_SET_MASK32(status_addr, status_mask);

    if (status & status_mask)
    {
        // Submit CPER
        acpi_err_sec_firmware_t sec_fw_cper_section = {.severity = ACPI_ERROR_SEVERITY_CORRECTED,
                                                       .record_id = SCP_SCF_RAM,
                                                       .param = {status, address, err, 0}};

        acpi_cper_section_t cper_section;
        cper_section.sec_fw = sec_fw_cper_section;

        hm_submit_cper(ACPI_ERROR_DOMAIN_SCP_PROC, ACPI_ERROR_SEVERITY_CORRECTED, &cper_section, sizeof(cper_section));

        if (bugcheck)
        {
            BUG_CHECK(err, status, address);
        }
    }
}

static void rmss_scfram_ecc_ce_isr()
{
    ram_ecc_isr((void*)&scp_exp_csr_regs->scfram_scp_errstatus_reg,
                (void*)&scp_exp_csr_regs->scfram_scp_erraddr_reg,
                SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_CE_MASK,
                KNG_HM_SCF_CE,
                false);
}

static void rmss_scfram_ecc_of_isr()
{
    ram_ecc_isr((void*)&scp_exp_csr_regs->scfram_scp_errstatus_reg,
                (void*)&scp_exp_csr_regs->scfram_scp_erraddr_reg,
                SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_OF_MASK,
                KNG_HM_SCF_OF,
                true);
}

static void rmss_ram0_ecc_ce_isr()
{
    ram_ecc_isr((void*)&scp_exp_csr_regs->rmss_ram0_scp_errstatus_reg,
                (void*)&scp_exp_csr_regs->rmss_ram0_scp_erraddr_reg,
                SCP_EXP_CSR_RMSS_RAM0_SCP_ERRSTATUS_REG_CE_MASK,
                KNG_HM_RMSS_RAM0_CE,
                false);
}

static void rmss_ram0_ecc_of_isr()
{
    ram_ecc_isr((void*)&scp_exp_csr_regs->rmss_ram0_scp_errstatus_reg,
                (void*)&scp_exp_csr_regs->rmss_ram0_scp_erraddr_reg,
                SCP_EXP_CSR_RMSS_RAM0_SCP_ERRSTATUS_REG_OF_MASK,
                KNG_HM_RMSS_RAM0_OF,
                true);
}

static void rmss_ram1_ecc_ce_isr()
{
    ram_ecc_isr((void*)&scp_exp_csr_regs->rmss_ram1_scp_errstatus_reg,
                (void*)&scp_exp_csr_regs->rmss_ram1_scp_erraddr_reg,
                SCP_EXP_CSR_RMSS_RAM1_SCP_ERRSTATUS_REG_CE_MASK,
                KNG_HM_RMSS_RAM1_CE,
                false);
}

static void rmss_ram1_ecc_of_isr()
{
    ram_ecc_isr((void*)&scp_exp_csr_regs->rmss_ram1_scp_errstatus_reg,
                (void*)&scp_exp_csr_regs->rmss_ram1_scp_erraddr_reg,
                SCP_EXP_CSR_RMSS_RAM1_SCP_ERRSTATUS_REG_OF_MASK,
                KNG_HM_RMSS_RAM1_OF,
                true);
}

static void inject_err_by_access(uint32_t addr, bool cacheable)
{
    __DSB();
    if (cacheable)
    {
        SCB_InvalidateDCache_by_Addr((uint32_t*)addr, sizeof(uint32_t));
    }
    MMIO_READ32(addr);
}

static acpi_einj_cmd_status_t scp_error_injection_handler(ras_einj_info_t* einj_payload, void* ctx)
{
    FPFW_UNUSED(ctx);

    if (einj_payload->component_group != ACPI_ERROR_DOMAIN_SCP_PROC)
    {
        FPFW_DBGPRINT_ERROR("Invalid SCP error domain(%d)\n", einj_payload->component_group);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    switch (einj_payload->param_type.error_type)
    {
    case SCP_ERROR_TYPE_SCF_RAM_CE:
        MMIO_UPDATE32(&scp_exp_csr_regs->scfram_errctrl_reg, SCP_EXP_CSR_SCFRAM_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        inject_err_by_access(SCF_RAM_ADDRESS, false);
        break;
    case SCP_ERROR_TYPE_SCF_RAM_OVERFLOW:
        nvic_global_disable();
        MMIO_UPDATE32(&scp_exp_csr_regs->scfram_errctrl_reg, SCP_EXP_CSR_SCFRAM_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        inject_err_by_access(SCF_RAM_ADDRESS, false);
        MMIO_UPDATE32(&scp_exp_csr_regs->scfram_errctrl_reg, SCP_EXP_CSR_SCFRAM_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        inject_err_by_access(SCF_RAM_ADDRESS, false);
        nvic_global_enable();
        break;
    case SCP_ERROR_TYPE_RMSS_RAM0_CE:
        MMIO_UPDATE32(&scp_exp_csr_regs->rmss_ram0_errctrl_reg, SCP_EXP_CSR_RMSS_RAM0_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        inject_err_by_access(RMSS_RAM0_ADDRESS, true);
        break;
    case SCP_ERROR_TYPE_RMSS_RAM0_OVERFLOW:
        nvic_global_disable();
        MMIO_UPDATE32(&scp_exp_csr_regs->rmss_ram0_errctrl_reg, SCP_EXP_CSR_RMSS_RAM0_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        inject_err_by_access(RMSS_RAM0_ADDRESS, true);
        MMIO_UPDATE32(&scp_exp_csr_regs->rmss_ram0_errctrl_reg, SCP_EXP_CSR_RMSS_RAM0_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        inject_err_by_access(RMSS_RAM0_ADDRESS, true);
        nvic_global_enable();
        break;
    case SCP_ERROR_TYPE_RMSS_RAM1_CE:
        MMIO_UPDATE32(&scp_exp_csr_regs->rmss_ram1_errctrl_reg, SCP_EXP_CSR_RMSS_RAM1_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        inject_err_by_access(RMSS_RAM1_ADDRESS, true);
        break;
    case SCP_ERROR_TYPE_RMSS_RAM1_OVERFLOW:
        nvic_global_disable();
        MMIO_UPDATE32(&scp_exp_csr_regs->rmss_ram1_errctrl_reg, SCP_EXP_CSR_RMSS_RAM1_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        inject_err_by_access(RMSS_RAM1_ADDRESS, true);
        MMIO_UPDATE32(&scp_exp_csr_regs->rmss_ram1_errctrl_reg, SCP_EXP_CSR_RMSS_RAM1_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        inject_err_by_access(RMSS_RAM1_ADDRESS, true);
        nvic_global_enable();
        break;
    default:
        FPFW_DBGPRINT_ERROR("Invalid/Unsupported SCP error type(%d)\n", einj_payload->component_type);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    // Placeholder for SCP error injection handling
    return ACPI_EINJ_SUCCESS;
}

static void enable_scp_ecc_error()
{
    // Enable SCF ECC errors
    MMIO_SET_MASK32(&scp_exp_csr_regs->scfram_errctrl_reg, SCP_EXP_CSR_SCFRAM_ERRCTRL_REG_ECC_EN_MASK);

    // Enable MSCP EXP RAM0 ECC errors
    MMIO_SET_MASK32(&scp_exp_csr_regs->rmss_ram0_errctrl_reg, SCP_EXP_CSR_RMSS_RAM0_ERRCTRL_REG_ECC_EN_MASK);

    // Enable MSCP EXP RAM1 ECC errors
    MMIO_SET_MASK32(&scp_exp_csr_regs->rmss_ram1_errctrl_reg, SCP_EXP_CSR_RMSS_RAM1_ERRCTRL_REG_ECC_EN_MASK);
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
    register_scp_ecc_isr(HW_INT_SCP_SCFRAM_ECCOF_INT, rmss_scfram_ecc_of_isr); // SCF RAM overflow
    register_scp_ecc_isr(HW_INT_SCP_SCFRAM_ECCCE_INT, rmss_scfram_ecc_ce_isr); // SCF RAM CE

    register_scp_ecc_isr(HW_INT_SCP_RAM0_ECCOF_INT, rmss_ram0_ecc_of_isr); // RMSS RAM0 overflow
    register_scp_ecc_isr(HW_INT_SCP_RAM0_ECCCE_INT, rmss_ram0_ecc_ce_isr); // RMSS RAM0 CE

    register_scp_ecc_isr(HW_INT_SCP_RAM1_ECCOF_INT, rmss_ram1_ecc_of_isr); // RMSS RAM1 overflow
    register_scp_ecc_isr(HW_INT_SCP_RAM1_ECCCE_INT, rmss_ram1_ecc_ce_isr); // RMSS RAM1 CE
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