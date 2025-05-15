//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_injection_scp.c
 * This file contains scp ras injection related functionality.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FpFwUtils.h>
#include <arm_intrinsic.h> // for __DSB on Windows builds (empty define)
#include <cper.h>
#include <health_monitor.h>
#include <mscp_exp_rmss_memory_map.h>
#include <mscp_ras_and_init_ctrl_registers_regs.h>
#include <nvic.h>
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <scp_top_regs.h>

// clang-format off
#include <cmsis_m7.h>
#ifdef PLATFORM_CACHING_ENABLED
#include <cmsis_compiler.h>
#include <m-profile/armv7m_cachel1.h>
#endif
// clang-format on

/*-- Symbolic Constant Macros (defines) --*/
#define TCM_TGT_RAM_DTCM0RAM (0x80)
#define MASK_CE              0x20
#define MASK_UE              0x40

#define SCF_RAM_ADDRESS   (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_RAM_ADDRESS)
#define RMSS_RAM0_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS)
#define RMSS_RAM1_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS)
#define DTC_RAM_ADDRESS   (SCP_TOP_SCP_DATA_RAM_ADDRESS)

/*-------- Function Prototypes -----------*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
static vptr_scp_exp_csr_reg scp_exp_csr_regs =
    (vptr_scp_exp_csr_reg)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS);
static vptr_mscp_ras_and_init_ctrl_registers_reg scp_ras_and_init_ctrl_registers_reg =
    (vptr_mscp_ras_and_init_ctrl_registers_reg)(SCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS);
/*-------------- Functions ---------------*/

static void inject_err_by_access(uint32_t addr)
{
    __DSB();

    if (SCP_EXP_CACHEABLE_SECTION_BASE <= addr && addr <= SCP_EXP_CACHEABLE_SECTION_END)
    {
        SCB_InvalidateDCache_by_Addr((uint32_t*)addr, sizeof(uint32_t));
    }

    MMIO_READ32(addr);
}

acpi_einj_cmd_status_t scp_error_injection_handler(ras_einj_info_t* einj_payload, void* ctx)
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
        inject_err_by_access(SCF_RAM_ADDRESS);
        break;
    case SCP_ERROR_TYPE_SCF_RAM_OVERFLOW:
        nvic_global_disable();
        MMIO_UPDATE32(&scp_exp_csr_regs->scfram_errctrl_reg, SCP_EXP_CSR_SCFRAM_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        inject_err_by_access(SCF_RAM_ADDRESS);
        MMIO_UPDATE32(&scp_exp_csr_regs->scfram_errctrl_reg, SCP_EXP_CSR_SCFRAM_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        inject_err_by_access(SCF_RAM_ADDRESS);
        nvic_global_enable();
        break;
    case SCP_ERROR_TYPE_RMSS_RAM0_CE:
        MMIO_UPDATE32(&scp_exp_csr_regs->rmss_ram0_errctrl_reg, SCP_EXP_CSR_RMSS_RAM0_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        inject_err_by_access(RMSS_RAM0_ADDRESS);
        break;
    case SCP_ERROR_TYPE_RMSS_RAM0_OVERFLOW:
        nvic_global_disable();
        MMIO_UPDATE32(&scp_exp_csr_regs->rmss_ram0_errctrl_reg, SCP_EXP_CSR_RMSS_RAM0_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        inject_err_by_access(RMSS_RAM0_ADDRESS);
        MMIO_UPDATE32(&scp_exp_csr_regs->rmss_ram0_errctrl_reg, SCP_EXP_CSR_RMSS_RAM0_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        inject_err_by_access(RMSS_RAM0_ADDRESS);
        nvic_global_enable();
        break;
    case SCP_ERROR_TYPE_RMSS_RAM1_CE:
        MMIO_UPDATE32(&scp_exp_csr_regs->rmss_ram1_errctrl_reg, SCP_EXP_CSR_RMSS_RAM1_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        inject_err_by_access(RMSS_RAM1_ADDRESS);
        break;
    case SCP_ERROR_TYPE_RMSS_RAM1_OVERFLOW:
        nvic_global_disable();
        MMIO_UPDATE32(&scp_exp_csr_regs->rmss_ram1_errctrl_reg, SCP_EXP_CSR_RMSS_RAM1_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        inject_err_by_access(RMSS_RAM1_ADDRESS);
        MMIO_UPDATE32(&scp_exp_csr_regs->rmss_ram1_errctrl_reg, SCP_EXP_CSR_RMSS_RAM1_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        inject_err_by_access(RMSS_RAM1_ADDRESS);
        nvic_global_enable();
        break;
    case SCP_ERROR_TYPE_TCM_CE:
        MMIO_UPDATE32(&scp_ras_and_init_ctrl_registers_reg->tcmecc_errctrl.as_uint32,
                      MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_TGT_RAM_MASK |
                          MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_INJECT_ERROR_MASK,
                      TCM_TGT_RAM_DTCM0RAM | MASK_CE);
        inject_err_by_access(DTC_RAM_ADDRESS);
        break;
    case SCP_ERROR_TYPE_TCM_UE:
        MMIO_UPDATE32(&scp_ras_and_init_ctrl_registers_reg->tcmecc_errctrl.as_uint32,
                      MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_TGT_RAM_MASK |
                          MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_INJECT_ERROR_MASK,
                      TCM_TGT_RAM_DTCM0RAM | MASK_UE);
        inject_err_by_access(DTC_RAM_ADDRESS);
        break;
    case SCP_ERROR_TYPE_TCM_OVERFLOW:
        nvic_global_disable();
        MMIO_UPDATE32(&scp_ras_and_init_ctrl_registers_reg->tcmecc_errctrl.as_uint32,
                      MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_TGT_RAM_MASK |
                          MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_INJECT_ERROR_MASK,
                      TCM_TGT_RAM_DTCM0RAM | MASK_CE);
        inject_err_by_access(DTC_RAM_ADDRESS);
        MMIO_UPDATE32(&scp_ras_and_init_ctrl_registers_reg->tcmecc_errctrl.as_uint32,
                      MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_TGT_RAM_MASK |
                          MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_INJECT_ERROR_MASK,
                      TCM_TGT_RAM_DTCM0RAM | MASK_CE);
        inject_err_by_access(DTC_RAM_ADDRESS);
        nvic_global_enable();
        break;
    default:
        FPFW_DBGPRINT_ERROR("Invalid/Unsupported SCP error type(%d)\n", einj_payload->component_type);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    // Placeholder for SCP error injection handling
    return ACPI_EINJ_SUCCESS;
}