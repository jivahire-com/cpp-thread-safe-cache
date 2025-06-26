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
#include <atu_api.h>
#include <bug_check.h>
#include <cper.h>
#include <error_domain_i.h>
#include <health_monitor.h>
#include <idsw_kng.h>
#include <mscp_error_domain.h>
#include <mscp_exp_rmss_memory_map.h>
#include <mscp_ras_and_init_ctrl_registers_regs.h>
#include <nvic.h>
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <scp_top_regs.h>
#include <shared_sram_ecc_ras_registers_regs.h>

// clang-format off
#include <cmsis_m7.h>
#ifdef PLATFORM_CACHING_ENABLED
#include <cmsis_compiler.h>
#include <m-profile/armv7m_cachel1.h>
#endif
// clang-format on

/*-- Symbolic Constant Macros (defines) --*/
#define ARSM_RAM_DEFAULT_OFFSET (0x10)
#define RSM_RAM_DEFAULT_OFFSET  (0x08)

#define SCF_RAM_ADDRESS   (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_RAM_ADDRESS)
#define RMSS_RAM0_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS)
#define RMSS_RAM1_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS)
#define DTC_RAM_ADDRESS   (SCP_TOP_SCP_DATA_RAM_ADDRESS)

#define AP_RSM_ADDRESS_DIEn(n) (0x2F000000UL + 0x1000000000UL * (n))
#define AP_RSM_SIZE            (0x00010000UL)
#define ATU_MAPPING_RSM_RAM(die_id) \
    ATU_MAPPING((die_id == 0 ? AP_RSM_ADDRESS_DIEn(0) : AP_RSM_ADDRESS_DIEn(1)), 0, AP_RSM_SIZE, {ATU_BUS_ATTR_NS})

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

static void trigger_shared_sram_fault(bool arsm, int type, uint32_t target_addr, uint32_t err_mask)
{
    atu_map_entry_t entry;

    if (arsm)
    {
        BUG_ASSERT(type < SCP_ARSM_RAM_COUNT);
        get_arsm_ecc_atu_entry((scp_arsm_ram_type_t)type, &entry);
    }
    else
    {
        BUG_ASSERT(type < SCP_RSM_RAM_COUNT);
        get_rsm_ecc_atu_entry((scp_rsm_ram_type_t)type, &entry);
    }

    int status = atu_map(ATU_ID_MSCP, &entry);
    BUG_ASSERT(status == SILIBS_SUCCESS);

    if (err_mask & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK)
    {
        MMIO_UPDATE32(entry.mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_ADDRESS,
                      SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_CE_MASK,
                      SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_CE_MASK);
        inject_err_by_access(target_addr);
    }
    else if (err_mask & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK)
    {
        MMIO_UPDATE32(entry.mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_ADDRESS,
                      SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_UE_MASK,
                      SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_UE_MASK);
        inject_err_by_access(target_addr);
    }
    else if (err_mask & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK)
    {
        nvic_global_disable();
        MMIO_UPDATE32(entry.mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_ADDRESS,
                      SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_UE_MASK,
                      SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_UE_MASK);
        inject_err_by_access(target_addr);
        MMIO_UPDATE32(entry.mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_ADDRESS,
                      SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_UE_MASK,
                      SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRMISC1_INJECT_UE_MASK);
        inject_err_by_access(target_addr);
        nvic_global_enable();
    }

    status = atu_unmap(ATU_ID_MSCP, &entry);
    BUG_ASSERT(status == SILIBS_SUCCESS);
}

static void trigger_shared_sram_arsm_fault(scp_arsm_ram_type_t type, uint32_t target_addr, uint32_t err_mask)
{
    BUG_ASSERT(type < SCP_ARSM_RAM_COUNT);
    trigger_shared_sram_fault(true, (int)type, target_addr, err_mask);
}

static void trigger_shared_sram_rsm_fault(scp_rsm_ram_type_t type, uint32_t target_addr, uint32_t err_mask)
{
    BUG_ASSERT(type < SCP_RSM_RAM_COUNT);
    trigger_shared_sram_fault(false, (int)type, target_addr, err_mask);
}

static uint32_t map_rsm_address(atu_map_entry_t* atu_entry)
{
    BUG_ASSERT(atu_entry != NULL);
    KNG_DIE_ID die_id = idsw_get_die_id();

    *atu_entry = (atu_map_entry_t)ATU_MAPPING_RSM_RAM(die_id);

    int status = atu_map(ATU_ID_MSCP, atu_entry);
    BUG_ASSERT(status == SILIBS_SUCCESS);

    return atu_entry->mscp_start_address + RSM_RAM_DEFAULT_OFFSET;
}

static void unmap_rsm_address(atu_map_entry_t* atu_entry)
{
    int status = atu_unmap(ATU_ID_MSCP, atu_entry);
    BUG_ASSERT(status == SILIBS_SUCCESS);
}

acpi_einj_cmd_status_t mscp_error_injection_handler(ras_einj_info_t* einj_payload, void* ctx)
{
    FPFW_UNUSED(ctx);

    if (einj_payload->component_group != ACPI_ERROR_DOMAIN_SCP_PROC)
    {
        FPFW_DBGPRINT_ERROR("Invalid SCP error domain(%d)\n", einj_payload->component_group);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    // ARSM RAM error injection address
    uint32_t arsm_addr = ((idsw_get_die_id() == DIE_0) ? MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR
                                                       : MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR);
    arsm_addr = (einj_payload->param_type.address_64 == 0)
                    ? arsm_addr + ARSM_RAM_DEFAULT_OFFSET
                    : arsm_addr + (uint32_t)(uintptr_t)einj_payload->param_type.address_64;

    switch (einj_payload->param_type.error_type)
    {
    case SCP_ERROR_TYPE_SCF_RAM_CE:
        MMIO_UPDATE32(&scp_exp_csr_regs->scfram_errctrl_reg, SCP_EXP_CSR_SCFRAM_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        inject_err_by_access(SCF_RAM_ADDRESS);
        break;
    case SCP_ERROR_TYPE_SCF_RAM_UE:
        MMIO_UPDATE32(&scp_exp_csr_regs->scfram_errctrl_reg, SCP_EXP_CSR_SCFRAM_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_UE);
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
    case SCP_ERROR_TYPE_RMSS_RAM0_UE:
        MMIO_UPDATE32(&scp_exp_csr_regs->rmss_ram0_errctrl_reg, SCP_EXP_CSR_RMSS_RAM0_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_UE);
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
    case SCP_ERROR_TYPE_RMSS_RAM1_UE:
        MMIO_UPDATE32(&scp_exp_csr_regs->rmss_ram1_errctrl_reg, SCP_EXP_CSR_RMSS_RAM1_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_UE);
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
    case SCP_ERROR_TYPE_DATA_CACHE_CE:
        dcache_ce_isr();
        break;
    case SCP_ERROR_TYPE_DATA_CACHE_UE:
        dcache_ue_isr();
        break;
    case SCP_ERROR_TYPE_DATA_CACHE_TAG_CE:
        dcache_tag_ce_isr();
        break;
    case SCP_ERROR_TYPE_DATA_CACHE_TAG_UE:
        dcache_tag_ue_isr();
        break;
    case SCP_ERROR_TYPE_INSTRUCTION_CACHE_CE:
        icache_ce_isr();
        break;
    case SCP_ERROR_TYPE_INSTRUCTION_CACHE_UE:
        icache_ue_isr();
        break;
    case SCP_ERROR_TYPE_INSTRUCTION_CACHE_TAG_CE:
        icache_tag_ce_isr();
        break;
    case SCP_ERROR_TYPE_INSTRUCTION_CACHE_TAG_UE:
        icache_tag_ue_isr();
        break;
    case SCP_ERROR_TYPE_HARD_FAULT:
        trigger_hard_fault();
        break;
    case SCP_ERROR_TYPE_BUS_FAULT:
        trigger_bus_fault();
        break;
    case SCP_ERROR_TYPE_MMU_FAULT:
        trigger_mmu_fault();
        break;
    case SCP_ERROR_TYPE_USAGE_FAULT:
    case SCP_ERROR_TYPE_FW_FAULT:
        trigger_usage_fault();
        break;
    case SCP_ERROR_TYPE_WATCHDOG:
        trigger_mscp_watchdog_fault();
        break;
    case SCP_ERROR_TYPE_S_ARSM_CE:
        trigger_shared_sram_arsm_fault(SCP_S_ARSM_RAM, arsm_addr, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
        break;
    case SCP_ERROR_TYPE_S_ARSM_UE:
        FPFW_DBGPRINT_WARNING("SCP_ERROR_TYPE_S_ARSM_UE is not supported in this build.\n");
        // trigger_shared_sram_arsm_fault(SCP_S_ARSM_RAM,
        //                           arsm_addr,
        //                           SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
        break;
    case SCP_ERROR_TYPE_S_ARSM_OVERFLOW:
        FPFW_DBGPRINT_WARNING("SCP_ERROR_TYPE_S_ARSM_OVERFLOW is not supported in this build.\n");
        // trigger_shared_sram_arsm_fault(SCP_S_ARSM_RAM,
        //                           arsm_addr,
        //                           SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK);
        break;
    case SCP_ERROR_TYPE_NS_ARSM_CE:
        trigger_shared_sram_arsm_fault(SCP_NS_ARSM_RAM, arsm_addr, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
        break;
    case SCP_ERROR_TYPE_NS_ARSM_UE:
        FPFW_DBGPRINT_WARNING("SCP_ERROR_TYPE_NS_ARSM_UE is not supported in this build.\n");
        // trigger_shared_sram_arsm_fault(SCP_NS_ARSM_RAM,
        //                           arsm_addr,
        //                           SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
        break;
    case SCP_ERROR_TYPE_NS_ARSM_OVERFLOW:
        FPFW_DBGPRINT_WARNING("SCP_ERROR_TYPE_NS_ARSM_OVERFLOW is not supported in this build.\n");
        // trigger_shared_sram_arsm_fault(SCP_NS_ARSM_RAM,
        //                           arsm_addr,
        //                           SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK);
        break;
    case SCP_ERROR_TYPE_RT_ARSM_CE:
        trigger_shared_sram_arsm_fault(SCP_RT_ARSM_RAM, arsm_addr, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
        break;
    case SCP_ERROR_TYPE_RT_ARSM_UE:
        FPFW_DBGPRINT_WARNING("SCP_ERROR_TYPE_RT_ARSM_UE is not supported in this build.\n");
        // trigger_shared_sram_arsm_fault(SCP_RT_ARSM_RAM,
        //                           arsm_addr,
        //                           SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
        break;
    case SCP_ERROR_TYPE_RT_ARSM_OVERFLOW:
        FPFW_DBGPRINT_WARNING("SCP_ERROR_TYPE_RT_ARSM_OVERFLOW is not supported in this build.\n");
        // trigger_shared_sram_arsm_fault(SCP_RT_ARSM_RAM,
        //                           arsm_addr,
        //                           SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK);
        break;
    case SCP_ERROR_TYPE_RL_ARSM_CE:
        trigger_shared_sram_arsm_fault(SCP_RL_ARSM_RAM, arsm_addr, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
        break;
    case SCP_ERROR_TYPE_RL_ARSM_UE:
        FPFW_DBGPRINT_WARNING("SCP_ERROR_TYPE_RL_ARSM_UE is not supported in this build.\n");
        // trigger_shared_sram_arsm_fault(SCP_RL_ARSM_RAM,
        //                           arsm_addr,
        //                           SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
        break;
    case SCP_ERROR_TYPE_RL_ARSM_OVERFLOW:
        FPFW_DBGPRINT_WARNING("SCP_ERROR_TYPE_RL_ARSM_OVERFLOW is not supported in this build.\n");
        // trigger_shared_sram_arsm_fault(SCP_RL_ARSM_RAM,
        //                           arsm_addr,
        //                           SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK);
        break;
    case SCP_ERROR_TYPE_RSM_RAM_CE: {
        atu_map_entry_t rsm_atu_entry;
        uint32_t read_addr = map_rsm_address(&rsm_atu_entry);
        trigger_shared_sram_rsm_fault(SCP_S_RSM_RAM, read_addr, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
        unmap_rsm_address(&rsm_atu_entry);
        break;
    }
    case SCP_ERROR_TYPE_RSM_RAM_UE: {
        FPFW_DBGPRINT_WARNING("SCP_ERROR_TYPE_RSM_RAM_UE is not supported in this build.\n");
        // atu_map_entry_t rsm_atu_entry;
        // uint32_t read_addr = map_rsm_address(&rsm_atu_entry);
        // trigger_shared_sram_rsm_fault(SCP_S_RSM_RAM, read_addr,
        // SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK); unmap_rsm_address(&rsm_atu_entry);
        break;
    }

    default:
        FPFW_DBGPRINT_ERROR("Invalid/Unsupported SCP error type(%d)\n", einj_payload->param_type.error_type);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    // Placeholder for SCP error injection handling
    return ACPI_EINJ_SUCCESS;
}