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
#include <fuses_csr_regs.h>
#include <health_monitor.h>
#include <idsw_kng.h>
#include <mscp_error_domain.h>
#include <mscp_exp_rmss_memory_map.h>
#include <mscp_ras_and_init_ctrl_registers_regs.h>
#include <nvic.h>
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <fuses_top_regs.h>
#include <scp_top_regs.h>
#include <shared_sram_ecc_ras_registers_regs.h>
#include <utils.h>

// clang-format off
#include <cmsis_m7.h>
#ifdef PLATFORM_CACHING_ENABLED
#include <cmsis_compiler.h>
#include <m-profile/armv7m_cachel1.h>
#endif
// clang-format on

/*-- Symbolic Constant Macros (defines) --*/
#define SCF_RAM_ADDRESS   (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_RAM_ADDRESS)
#define RMSS_RAM0_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS)
#define RMSS_RAM1_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS)
#define DTC_RAM_ADDRESS   (SCP_TOP_SCP_DATA_RAM_ADDRESS)
#define FUSES_RAM_ADDRESS \
    (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_FUSE_ADDRESS + FUSES_TOP_FUSE_RAM_SPACE_ADDRESS)

/*-------- Function Prototypes -----------*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
static vptr_scp_exp_csr_reg scp_exp_csr_regs =
    (vptr_scp_exp_csr_reg)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS);
static vptr_mscp_ras_and_init_ctrl_registers_reg scp_ras_and_init_ctrl_registers_reg =
    (vptr_mscp_ras_and_init_ctrl_registers_reg)(SCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS);
static vptr_fuses_csr_reg scp_exp_fuses_regs =
    (vptr_fuses_csr_reg)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_FUSE_ADDRESS + FUSES_TOP_FUSES_CSR_ADDRESS);

/*-------------- Functions ---------------*/
// static PLACED_CODE uint32_t get_error_injection_address(ras_einj_info_t* einj_payload,
//                                                         atu_map_entry_t* atu_entry,
//                                                         atu_map_entry_t* atu_recover_entry)
static uint32_t get_error_injection_address(ras_einj_info_t* einj_payload, atu_map_entry_t* atu_entry, atu_map_entry_t* atu_recover_entry)
{
    uint32_t err_addr = 0;

    switch (einj_payload->param_type.error_type)
    {
    case SCP_ERROR_TYPE_S_ARSM_CE:
    case SCP_ERROR_TYPE_S_ARSM_UE:
    case SCP_ERROR_TYPE_S_ARSM_OVERFLOW: {
        atu_entry_attr_t attributes = {ATU_BUS_ATTR_S};
        err_addr = get_arsm_attr_address(attributes.as_uint32, atu_entry, atu_recover_entry);
    }
    break;
    case SCP_ERROR_TYPE_NS_ARSM_CE:
    case SCP_ERROR_TYPE_NS_ARSM_UE:
    case SCP_ERROR_TYPE_NS_ARSM_OVERFLOW:
        if (idsw_get_die_id() == DIE_0)
        {
            atu_entry_attr_t attributes = {ATU_BUS_ATTR_NS};
            err_addr = get_arsm_attr_address(attributes.as_uint32, atu_entry, atu_recover_entry);
        }
        else
        {
            err_addr = MSCP_ATU_AP_WINDOW_ARSM_DIE_1_NS_BASE_ADDR;
        }
        break;
    case SCP_ERROR_TYPE_RT_ARSM_CE:
    case SCP_ERROR_TYPE_RT_ARSM_UE:
    case SCP_ERROR_TYPE_RT_ARSM_OVERFLOW:
        err_addr = ((idsw_get_die_id() == DIE_0) ? MSCP_ATU_AP_WINDOW_ARSM_DIE_0_ROOT_BASE_ADDR
                                                 : MSCP_ATU_AP_WINDOW_ARSM_DIE_1_ROOT_BASE_ADDR);
        break;
    case SCP_ERROR_TYPE_RL_ARSM_CE:
    case SCP_ERROR_TYPE_RL_ARSM_UE:
    case SCP_ERROR_TYPE_RL_ARSM_OVERFLOW: {
        atu_entry_attr_t attributes = {ATU_BUS_ATTR_REALM};
        err_addr = get_arsm_attr_address(attributes.as_uint32, atu_entry, atu_recover_entry);
    }
    break;
    default:
        FPFW_DBGPRINT_ERROR("Invalid SCP error type(%d)\n", einj_payload->param_type.error_type);
        break;
    }

    return err_addr;
}

static PLACED_CODE void trigger_arsm_fault(ras_einj_info_t* einj_payload, mscp_arsm_ram_type_t type, uint32_t err_mask)
{
    atu_map_entry_t arsm_atu_entry = {0};
    atu_map_entry_t arsm_atu_recover_entry = {0};
    trigger_shared_sram_arsm_fault(type, get_error_injection_address(einj_payload, &arsm_atu_entry, &arsm_atu_recover_entry), err_mask);
    BUG_ASSERT(atu_unmap(ATU_ID_MSCP, &arsm_atu_entry) == SILIBS_SUCCESS);
    if (arsm_atu_recover_entry.mscp_start_address != 0)
    {
        BUG_ASSERT(atu_map(ATU_ID_MSCP, &arsm_atu_recover_entry) == SILIBS_SUCCESS);
    }
}

PLACED_CODE acpi_einj_cmd_status_t mscp_error_injection_handler(ras_einj_info_t* einj_payload, void* ctx)
{
    FPFW_UNUSED(ctx);

    if (einj_payload->component_group != ACPI_ERROR_DOMAIN_SCP_PROC)
    {
        FPFW_DBGPRINT_ERROR("Invalid SCP error domain(%d)\n", einj_payload->component_group);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    // ARSM RAM error injection address
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
        trigger_arsm_fault(einj_payload, MSCP_S_ARSM_RAM, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
        break;
    case SCP_ERROR_TYPE_S_ARSM_UE:
        if (!IS_PLATFORM_FPGA())
        {
            trigger_arsm_fault(einj_payload, MSCP_S_ARSM_RAM, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
        }
        else
        {
            FPFW_DBGPRINT_WARNING("SCP_ERROR_TYPE_S_ARSM_UE is not supported on FPGA.\n");
        }
        break;
    case SCP_ERROR_TYPE_S_ARSM_OVERFLOW:
        if (!IS_PLATFORM_FPGA())
        {
            trigger_arsm_fault(einj_payload, MSCP_S_ARSM_RAM, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK);
        }
        else
        {
            FPFW_DBGPRINT_WARNING("SCP_ERROR_TYPE_S_ARSM_OVERFLOW is not supported on FPGA.\n");
        }
        break;
    case SCP_ERROR_TYPE_NS_ARSM_CE:
        trigger_arsm_fault(einj_payload, MSCP_NS_ARSM_RAM, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
        break;
    case SCP_ERROR_TYPE_NS_ARSM_UE:
        if (!IS_PLATFORM_FPGA())
        {
            trigger_arsm_fault(einj_payload, MSCP_NS_ARSM_RAM, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
        }
        else
        {
            FPFW_DBGPRINT_WARNING("SCP_ERROR_TYPE_NS_ARSM_UE is not supported on FPGA.\n");
        }
        break;
    case SCP_ERROR_TYPE_NS_ARSM_OVERFLOW:
        if (!IS_PLATFORM_FPGA())
        {
            trigger_arsm_fault(einj_payload, MSCP_NS_ARSM_RAM, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK);
        }
        else
        {
            FPFW_DBGPRINT_WARNING("SCP_ERROR_TYPE_NS_ARSM_OVERFLOW is not supported on FPGA.\n");
        }
        break;
    case SCP_ERROR_TYPE_RT_ARSM_CE:
        trigger_shared_sram_arsm_fault(MSCP_RT_ARSM_RAM,
                                       get_error_injection_address(einj_payload, NULL, NULL),
                                       SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
        break;
    case SCP_ERROR_TYPE_RT_ARSM_UE:
        if (!IS_PLATFORM_FPGA())
        {
            trigger_shared_sram_arsm_fault(MSCP_RT_ARSM_RAM,
                                           get_error_injection_address(einj_payload, NULL, NULL),
                                           SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
        }
        else
        {
            FPFW_DBGPRINT_WARNING("SCP_ERROR_TYPE_RT_ARSM_UE is not supported on FPGA.\n");
        }
        break;
    case SCP_ERROR_TYPE_RT_ARSM_OVERFLOW:
        if (!IS_PLATFORM_FPGA())
        {
            trigger_shared_sram_arsm_fault(MSCP_RT_ARSM_RAM,
                                           get_error_injection_address(einj_payload, NULL, NULL),
                                           SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK);
        }
        else
        {
            FPFW_DBGPRINT_WARNING("SCP_ERROR_TYPE_RT_ARSM_OVERFLOW is not supported on FPGA.\n");
        }
        break;
    case SCP_ERROR_TYPE_RL_ARSM_CE:
        trigger_arsm_fault(einj_payload, MSCP_RL_ARSM_RAM, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
        break;
    case SCP_ERROR_TYPE_RL_ARSM_UE:
        if (!IS_PLATFORM_FPGA())
        {
            trigger_arsm_fault(einj_payload, MSCP_RL_ARSM_RAM, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
        }
        else
        {
            FPFW_DBGPRINT_WARNING("SCP_ERROR_TYPE_RL_ARSM_UE is not supported on FPGA.\n");
        }
        break;
    case SCP_ERROR_TYPE_RL_ARSM_OVERFLOW:
        if (!IS_PLATFORM_FPGA())
        {
            trigger_arsm_fault(einj_payload, MSCP_RL_ARSM_RAM, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK);
        }
        else
        {
            FPFW_DBGPRINT_WARNING("SCP_ERROR_TYPE_RL_ARSM_OVERFLOW is not supported on FPGA.\n");
        }
        break;
    case SCP_ERROR_TYPE_S_RSM_CE: {
        atu_map_entry_t rsm_atu_entry;
        uint32_t read_addr = map_rsm_address(MSCP_S_RSM_RAM, &rsm_atu_entry);
        trigger_shared_sram_rsm_fault(MSCP_S_RSM_RAM, read_addr, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
        unmap_rsm_address(&rsm_atu_entry);
        break;
    }
    case SCP_ERROR_TYPE_S_RSM_UE:
        if (!IS_PLATFORM_FPGA())
        {
            atu_map_entry_t rsm_atu_entry;
            uint32_t read_addr = map_rsm_address(MSCP_S_RSM_RAM, &rsm_atu_entry);
            trigger_shared_sram_rsm_fault(MSCP_S_RSM_RAM, read_addr, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
            unmap_rsm_address(&rsm_atu_entry);
        }
        else
        {
            FPFW_DBGPRINT_WARNING("SCP_ERROR_TYPE_RSM_RAM_UE is not supported on FPGA.\n");
        }
        break;
    case SCP_ERROR_TYPE_NS_RSM_CE: {
        atu_map_entry_t rsm_atu_entry;
        uint32_t read_addr = map_rsm_address(MSCP_NS_RSM_RAM, &rsm_atu_entry);
        trigger_shared_sram_rsm_fault(MSCP_NS_RSM_RAM, read_addr, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK);
        unmap_rsm_address(&rsm_atu_entry);

        break;
    }
    case SCP_ERROR_TYPE_NS_RSM_UE:
        if (!IS_PLATFORM_FPGA())
        {
            atu_map_entry_t rsm_atu_entry;
            uint32_t read_addr = map_rsm_address(MSCP_NS_RSM_RAM, &rsm_atu_entry);
            trigger_shared_sram_rsm_fault(MSCP_NS_RSM_RAM, read_addr, SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK);
            unmap_rsm_address(&rsm_atu_entry);
        }
        else
        {
            FPFW_DBGPRINT_WARNING("SCP_ERROR_TYPE_NS_RSM_UE is not supported on FPGA.\n");
        }
        break;
    case SCP_ERROR_TYPE_FUSE_CE: {
        MMIO_UPDATE32(&scp_exp_fuses_regs->sfcram_errctrl, FUSES_CSR_SFCRAM_ERRCTRL_INJECT_ERROR_MASK, MASK_CE);
        inject_err_by_access(FUSES_RAM_ADDRESS);
        break;
    }
    case SCP_ERROR_TYPE_FUSE_UE: {
        // index_a : Bit index of the error when injecting a CE error. Valid values are 0-31.
        const uint32_t index_a = 1;
        static_assert((index_a <= 31), "Invalid range on Index A");

        // index_b : Bit index of second error when injecting a UE error. Valid values are 0-31
        // Must be different than Index A. if same UE will downgrade to CE.
        const uint32_t index_b = (index_a << 1);
        static_assert((index_b <= 31 && index_b != index_a), "Invalid range on Index B");

        /* Update Index A */
        MMIO_UPDATE32(&scp_exp_fuses_regs->sfcram_errctrl,
                      FUSES_CSR_SFCRAM_ERRCTRL_INDEX_A_MASK,
                      (index_a << FUSES_CSR_SFCRAM_ERRCTRL_INDEX_A_LSB));

        /* Update Index B */
        MMIO_UPDATE32(&scp_exp_fuses_regs->sfcram_errctrl,
                      FUSES_CSR_SFCRAM_ERRCTRL_INDEX_B_MASK,
                      (index_b << FUSES_CSR_SFCRAM_ERRCTRL_INDEX_B_LSB));

        MMIO_UPDATE32(&scp_exp_fuses_regs->sfcram_errctrl, FUSES_CSR_SFCRAM_ERRCTRL_INJECT_ERROR_MASK, MASK_UE);
        inject_err_by_access(FUSES_RAM_ADDRESS);
        break;
    }
    case SCP_ERROR_TYPE_FUSE_OVERFLOW: {
        nvic_global_disable();
        MMIO_UPDATE32(&scp_exp_fuses_regs->sfcram_errctrl, FUSES_CSR_SFCRAM_ERRCTRL_INJECT_ERROR_MASK, MASK_CE);
        inject_err_by_access(FUSES_RAM_ADDRESS);
        MMIO_UPDATE32(&scp_exp_fuses_regs->sfcram_errctrl, FUSES_CSR_SFCRAM_ERRCTRL_INJECT_ERROR_MASK, MASK_CE);
        inject_err_by_access(FUSES_RAM_ADDRESS);
        nvic_global_enable();
        break;
    }
    case SCP_ERROR_TYPE_M7_LOCKUP:
        trigger_lockup();
        break;

    case SCP_ERROR_TYPE_ATU_ERR:
        trigger_atu_error();
        break;

    default:
        FPFW_DBGPRINT_ERROR("Invalid/Unsupported SCP error type(%d)\n", einj_payload->param_type.error_type);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    // Placeholder for SCP error injection handling
    return ACPI_EINJ_SUCCESS;
}

PLACED_CODE static void mcp_error_injection_setup_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);
    FPFW_UNUSED(status);

    icc_mhu_mscp_err_injection_setup_packet_t* req_params = (icc_mhu_mscp_err_injection_setup_packet_t*)context;

    switch (req_params->mcp_error_type)
    {
    case MCP_ERROR_TYPE_SCF_RAM_CE:
        MMIO_UPDATE32(&scp_exp_csr_regs->scfram_errctrl_reg, SCP_EXP_CSR_SCFRAM_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        break;

    case MCP_ERROR_TYPE_SCF_RAM_UE:
        MMIO_UPDATE32(&scp_exp_csr_regs->scfram_errctrl_reg, SCP_EXP_CSR_SCFRAM_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_UE);
        break;

    case MCP_ERROR_TYPE_SCF_RAM_OVERFLOW:
        MMIO_UPDATE32(&scp_exp_csr_regs->scfram_errctrl_reg, SCP_EXP_CSR_SCFRAM_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        break;

    case MCP_ERROR_TYPE_RMSS_RAM0_CE:
        MMIO_UPDATE32(&scp_exp_csr_regs->rmss_ram0_errctrl_reg, SCP_EXP_CSR_RMSS_RAM0_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        break;

    case MCP_ERROR_TYPE_RMSS_RAM0_UE:
        MMIO_UPDATE32(&scp_exp_csr_regs->rmss_ram0_errctrl_reg, SCP_EXP_CSR_RMSS_RAM0_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_UE);
        break;

    case MCP_ERROR_TYPE_RMSS_RAM0_OVERFLOW:
        MMIO_UPDATE32(&scp_exp_csr_regs->rmss_ram0_errctrl_reg, SCP_EXP_CSR_RMSS_RAM0_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        break;

    case MCP_ERROR_TYPE_RMSS_RAM1_CE:
        MMIO_UPDATE32(&scp_exp_csr_regs->rmss_ram1_errctrl_reg, SCP_EXP_CSR_RMSS_RAM1_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        break;

    case MCP_ERROR_TYPE_RMSS_RAM1_UE:
        MMIO_UPDATE32(&scp_exp_csr_regs->rmss_ram1_errctrl_reg, SCP_EXP_CSR_RMSS_RAM1_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_UE);
        break;

    case SCP_ERROR_TYPE_RMSS_RAM1_OVERFLOW:
        MMIO_UPDATE32(&scp_exp_csr_regs->rmss_ram1_errctrl_reg, SCP_EXP_CSR_RMSS_RAM1_ERRCTRL_REG_INJECT_ERROR_MASK, MASK_CE);
        break;
    }
    mcp_error_injection_setup_listener(get_mhu_handle());
}

PLACED_CODE void mcp_error_injection_setup_listener(fpfw_icc_base_ctx_t* icc_ctx)
{
    // Static buffer to hold the received message
    static uint8_t mcp_einj_recv_payload[32] = {0};

    static fpfw_icc_base_recv_req_t mcp_einj_recv_params;
    mcp_einj_recv_params.recv_cmd_code = ICC_HM_ERROR_INJECTION_SETUP_REQ; // Command code to listen for
    mcp_einj_recv_params.payload_buffer = mcp_einj_recv_payload;           // Buffer to receive the message
    mcp_einj_recv_params.buffer_size = sizeof(mcp_einj_recv_payload);      // Size of the buffer
    mcp_einj_recv_params.cb = mcp_error_injection_setup_cb; // Callback function for handling the received message
    mcp_einj_recv_params.cb_ctx = mcp_einj_recv_payload; // Context for the callback

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &mcp_einj_recv_params);
    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, icc_ctx);
}