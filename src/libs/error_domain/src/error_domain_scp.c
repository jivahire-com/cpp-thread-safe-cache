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
#include <error_domain_i.h>
#include <health_monitor.h>
#include <idhw.h>
#include <idsw_kng.h> // for KNG_DIE_ID, idsw_get_die_id
#include <interrupts.h>
#include <mscp_error_domain.h>
#include <mscp_ras_and_init_ctrl_registers_regs.h>
#include <nvic.h>
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <scp_top_regs.h>
#include <shared_sram_ecc_ras_registers_regs.h>

/*-- Symbolic Constant Macros (defines) --*/
#define SCP_PROC_FRU "SCP_PROC"
#define SCP_PROC_ERROR_DOMAIN_FRU_GUID_DEF                 \
    {                                                      \
        0xedf48164, 0x1d29, 0x47dc,                        \
        {                                                  \
            0xa7, 0x25, 0x8d, 0x8b, 0x79, 0xbb, 0xfc, 0x78 \
        }                                                  \
    }

/*-------- Function Prototypes -----------*/

/*-------------- Typedefs ----------------*/
typedef struct
{
    guid_t err_source_id;
    uint32_t err_source_irq;
    acpi_error_severity_t err_severity;
    KNG_STATUS err_code;
    bool bugcheck_required;
    uint32_t* param1;
    uint32_t* param2;
    uint32_t err_status_mask;
} mscp_ecc_isr_params_t;

/*-- Declarations (Statics and globals) --*/
static guid_t SCP_ERROR_DOMAIN_GUID = SCP_PROC_ERROR_DOMAIN_FRU_GUID_DEF;
static vptr_scp_exp_csr_reg scp_exp_csr_regs =
    (vptr_scp_exp_csr_reg)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS);
static vptr_mscp_ras_and_init_ctrl_registers_reg scp_ras_and_init_ctrl_registers_reg =
    (vptr_mscp_ras_and_init_ctrl_registers_reg)(SCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS);
static vptr_systemcontrol_reg scp_system_control_reg =
    (vptr_systemcontrol_reg)(SCP_TOP_CORTEX_M7_ADDRESS + CORTEXM7INTEGRATIONCS_SCP_SYSTEMCONTROL_ADDRESS);

static const atu_map_entry_t s_hm_atu_entries[2][SCP_ARSM_RAM_COUNT] = {{ATU_MAPPING_SCP_S_ARSM_RAM_ECC(DIE_0),
                                                                         ATU_MAPPING_SCP_NS_ARSM_RAM_ECC(DIE_0),
                                                                         ATU_MAPPING_SCP_RT_ARSM_RAM_ECC(DIE_0),
                                                                         ATU_MAPPING_SCP_RL_ARSM_RAM_ECC(DIE_0)},
                                                                        {ATU_MAPPING_SCP_S_ARSM_RAM_ECC(DIE_1),
                                                                         ATU_MAPPING_SCP_NS_ARSM_RAM_ECC(DIE_1),
                                                                         ATU_MAPPING_SCP_RT_ARSM_RAM_ECC(DIE_1),
                                                                         ATU_MAPPING_SCP_RL_ARSM_RAM_ECC(DIE_1)}};

/*-------------- Functions ---------------*/
void get_shared_sram_ecc_atu_entry(scp_arsm_ram_type_t type, atu_map_entry_t* atu_entry)
{
    BUG_ASSERT(type < SCP_ARSM_RAM_COUNT);
    *atu_entry = s_hm_atu_entries[idsw_get_die_id()][type];
}

uint32_t get_irq_num_for_scp_ecc_isr(scp_arsm_ram_type_t type)
{
    const uint32_t irq_nums[SCP_ARSM_RAM_COUNT] = {
        HW_INT_SCP_S_ARSM_ECC_FHI_INT,  // SCP_S_ARSM_RAM
        HW_INT_SCP_NS_ARSM_ECC_FHI_INT, // SCP_NS_ARSM_RAM
        HW_INT_SCP_RT_ARSM_ECC_FHI_INT, // SCP_RT_ARSM_RAM
        HW_INT_SCP_RL_ARSM_ECC_FHI_INT  // SCP_RL_ARSM_RAM
    };

    return irq_nums[type];
}
static void cache_ecc_isr(const mscp_ecc_isr_params_t* params)
{
    // Save stored error info from DEBR0 and DEBR1
    uint32_t err_bank_register0 = MMIO_READ32(params->param1);
    uint32_t err_bank_register1 = MMIO_READ32(params->param2);

    // Clear interrupt source
    nvic_irq_clear_pending(params->err_source_irq);

    // Submit CPER
    acpi_err_sec_firmware_t sec_fw_cper_section = {.severity = params->err_severity,
                                                   .record_id = params->err_source_id,
                                                   .param = {err_bank_register0, err_bank_register1, params->err_code, 0}};

    acpi_cper_section_t cper_section;
    cper_section.sec_fw = sec_fw_cper_section;

    hm_submit_cper(ACPI_ERROR_DOMAIN_SCP_PROC, params->err_severity, &cper_section, sizeof(cper_section));

    if (params->bugcheck_required)
    {
        BUG_CHECK(params->err_code, err_bank_register0, err_bank_register1);
    }
}

static void ram_ecc_isr(const mscp_ecc_isr_params_t* params)
{
    uint32_t status = MMIO_READ32(params->param1);
    uint32_t address = MMIO_READ32(params->param2);

    // Clear interrupt source
    MMIO_SET_MASK32(params->param1, params->err_status_mask);
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
                                    .param1 = (uint32_t*)&scp_exp_csr_regs->scfram_scp_errstatus_reg,
                                    .param2 = (uint32_t*)&scp_exp_csr_regs->scfram_scp_erraddr_reg,
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
                                    .param1 = (uint32_t*)&scp_exp_csr_regs->scfram_scp_errstatus_reg,
                                    .param2 = (uint32_t*)&scp_exp_csr_regs->scfram_scp_erraddr_reg,
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
                                    .param1 = (uint32_t*)&scp_exp_csr_regs->rmss_ram0_scp_errstatus_reg,
                                    .param2 = (uint32_t*)&scp_exp_csr_regs->rmss_ram0_scp_erraddr_reg,
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
                                    .param1 = (uint32_t*)&scp_exp_csr_regs->rmss_ram0_scp_errstatus_reg,
                                    .param2 = (uint32_t*)&scp_exp_csr_regs->rmss_ram0_scp_erraddr_reg,
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
                                    .param1 = (uint32_t*)&scp_exp_csr_regs->rmss_ram1_scp_errstatus_reg,
                                    .param2 = (uint32_t*)&scp_exp_csr_regs->rmss_ram1_scp_erraddr_reg,
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
                                    .param1 = (uint32_t*)&scp_exp_csr_regs->rmss_ram1_scp_errstatus_reg,
                                    .param2 = (uint32_t*)&scp_exp_csr_regs->rmss_ram1_scp_erraddr_reg,
                                    .err_status_mask = SCP_EXP_CSR_RMSS_RAM1_SCP_ERRSTATUS_REG_OF_MASK};

    ram_ecc_isr(&params);
}

static void rmss_remote_scp_scfram_bootram_isr()
{
    // Once silib PR approved, this will be removed.
#ifndef SCP_REMOTE_SCP_RAM
    #define SCP_REMOTE_SCP_RAM                                 \
        {                                                      \
            0x633a8127, 0xd1b0, 0x4bfb,                        \
            {                                                  \
                0xba, 0x9b, 0xf9, 0xf8, 0x1e, 0xa3, 0xad, 0xb2 \
            }                                                  \
        }
#endif
    // Cross-die error monitoring once remote die scp ECC detected on SCF/BOOTRAM0/1
    uint32_t scf_status = MMIO_READ32((uint32_t*)&scp_exp_csr_regs->scfram_rem_scp_errstatus_reg);
    uint32_t ram0_status = MMIO_READ32((uint32_t*)&scp_exp_csr_regs->rmss_ram0_rem_scp_errstatus_reg);
    uint32_t ram1_status = MMIO_READ32((uint32_t*)&scp_exp_csr_regs->rmss_ram1_rem_scp_errstatus_reg);

    nvic_irq_clear_pending(HW_INT_REMOTE_SCP_RAM_CE_OF_INT);

    // Submit CPER
    acpi_err_sec_firmware_t sec_fw_cper_section = {.severity = ACPI_ERROR_SEVERITY_CORRECTED,
                                                   .record_id = (guid_t)SCP_REMOTE_SCP_RAM,
                                                   .param = {scf_status, ram0_status, ram1_status, KNG_HM_REMOTE_SCP_CE_OF}};

    acpi_cper_section_t cper_section;
    cper_section.sec_fw = sec_fw_cper_section;

    hm_submit_cper(ACPI_ERROR_DOMAIN_SCP_PROC, sec_fw_cper_section.severity, &cper_section, sizeof(cper_section));
}

static void tcm_overflow_isr()
{
    mscp_ecc_isr_params_t params = {.err_source_id = (guid_t)SCP_TCM,
                                    .err_source_irq = HW_INT_SCP_TCM_ECCOF_INT,
                                    .err_severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL,
                                    .err_code = KNG_HM_TCM_OF,
                                    .bugcheck_required = true,
                                    .param1 = (uint32_t*)&scp_ras_and_init_ctrl_registers_reg->tcmecc_errstatus,
                                    .param2 = (uint32_t*)&scp_ras_and_init_ctrl_registers_reg->tcmecc_erraddr,
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
                                    .param1 = (uint32_t*)&scp_ras_and_init_ctrl_registers_reg->tcmecc_errstatus,
                                    .param2 = (uint32_t*)&scp_ras_and_init_ctrl_registers_reg->tcmecc_erraddr,
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
                                    .param1 = (uint32_t*)&scp_ras_and_init_ctrl_registers_reg->tcmecc_errstatus,
                                    .param2 = (uint32_t*)&scp_ras_and_init_ctrl_registers_reg->tcmecc_erraddr,
                                    .err_status_mask = MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_UE_MASK};

    ram_ecc_isr(&params);
}

void dcache_ue_isr()
{
    mscp_ecc_isr_params_t params = {.err_source_id = (guid_t)SCP_DCACHE,
                                    .err_source_irq = HW_INT_DCDET_DATA_UE,
                                    .err_severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL,
                                    .err_code = KNG_HM_DCACHE_UE,
                                    .bugcheck_required = true,
                                    .param1 = (uint32_t*)&scp_system_control_reg->debr0h,
                                    .param2 = (uint32_t*)&scp_system_control_reg->debr1h};

    cache_ecc_isr(&params);
}

void dcache_ce_isr()
{
    mscp_ecc_isr_params_t params = {.err_source_id = (guid_t)SCP_DCACHE,
                                    .err_source_irq = HW_INT_DCDET_DATA_CE,
                                    .err_severity = ACPI_ERROR_SEVERITY_CORRECTED,
                                    .err_code = KNG_HM_DCACHE_CE,
                                    .bugcheck_required = false,
                                    .param1 = (uint32_t*)&scp_system_control_reg->debr0h,
                                    .param2 = (uint32_t*)&scp_system_control_reg->debr1h};

    cache_ecc_isr(&params);
}

void dcache_tag_ue_isr()
{
    mscp_ecc_isr_params_t params = {.err_source_id = (guid_t)SCP_DCACHE,
                                    .err_source_irq = HW_INT_DCDET_TAG_UE,
                                    .err_severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL,
                                    .err_code = KNG_HM_DCACHE_TAG_UE,
                                    .bugcheck_required = true,
                                    .param1 = (uint32_t*)&scp_system_control_reg->debr0h,
                                    .param2 = (uint32_t*)&scp_system_control_reg->debr1h};

    cache_ecc_isr(&params);
}

void dcache_tag_ce_isr()
{
    mscp_ecc_isr_params_t params = {.err_source_id = (guid_t)SCP_DCACHE,
                                    .err_source_irq = HW_INT_DCDET_TAG_CE,
                                    .err_severity = ACPI_ERROR_SEVERITY_CORRECTED,
                                    .err_code = KNG_HM_DCACHE_TAG_CE,
                                    .bugcheck_required = false,
                                    .param1 = (uint32_t*)&scp_system_control_reg->debr0h,
                                    .param2 = (uint32_t*)&scp_system_control_reg->debr1h};

    cache_ecc_isr(&params);
}

void icache_ue_isr()
{
    mscp_ecc_isr_params_t params = {.err_source_id = (guid_t)SCP_ICACHE,
                                    .err_source_irq = HW_INT_ICDET_DATA_UE,
                                    .err_severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL,
                                    .err_code = KNG_HM_ICACHE_UE,
                                    .bugcheck_required = true,
                                    .param1 = (uint32_t*)&scp_system_control_reg->iebr0,
                                    .param2 = (uint32_t*)&scp_system_control_reg->iebr1h};

    cache_ecc_isr(&params);
}

void icache_ce_isr()
{
    mscp_ecc_isr_params_t params = {.err_source_id = (guid_t)SCP_ICACHE,
                                    .err_source_irq = HW_INT_ICDET_DATA_CE,
                                    .err_severity = ACPI_ERROR_SEVERITY_CORRECTED,
                                    .err_code = KNG_HM_ICACHE_CE,
                                    .bugcheck_required = false,
                                    .param1 = (uint32_t*)&scp_system_control_reg->iebr0,
                                    .param2 = (uint32_t*)&scp_system_control_reg->iebr1h};

    cache_ecc_isr(&params);
}

void icache_tag_ue_isr()
{
    mscp_ecc_isr_params_t params = {.err_source_id = (guid_t)SCP_ICACHE,
                                    .err_source_irq = HW_INT_ICDET_TAG_UE,
                                    .err_severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL,
                                    .err_code = KNG_HM_ICACHE_TAG_UE,
                                    .bugcheck_required = true,
                                    .param1 = (uint32_t*)&scp_system_control_reg->iebr0,
                                    .param2 = (uint32_t*)&scp_system_control_reg->iebr1h};

    cache_ecc_isr(&params);
}

void icache_tag_ce_isr()
{
    mscp_ecc_isr_params_t params = {.err_source_id = (guid_t)SCP_ICACHE,
                                    .err_source_irq = HW_INT_ICDET_TAG_CE,
                                    .err_severity = ACPI_ERROR_SEVERITY_CORRECTED,
                                    .err_code = KNG_HM_ICACHE_TAG_CE,
                                    .bugcheck_required = false,
                                    .param1 = (uint32_t*)&scp_system_control_reg->iebr0,
                                    .param2 = (uint32_t*)&scp_system_control_reg->iebr1h};

    cache_ecc_isr(&params);
}

void shared_sram_ecc_isr(void* ctx)
{
    uint32_t err_status = 0;
    uint32_t err_addr = 0;
    KNG_STATUS err_code = KNG_SUCCESS;
    acpi_error_severity_t severity = ACPI_ERROR_SEVERITY_INFORMATIONAL;
    atu_map_entry_t atu_entry = *(atu_map_entry_t*)ctx;

    // Map the shared SRAM ECC registers
    int status = atu_map(ATU_ID_MSCP, &atu_entry);
    BUG_ASSERT(status == SILIBS_SUCCESS);

    // Read error status
    err_status = MMIO_READ32(atu_entry.mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ADDRESS);

    if ((err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK) && // SRAMECC_ERRSTATUS is valid
        (err_status & (SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK |
                       SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_PN_MASK |
                       SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_DE_MASK)))
    {
        uint32_t err_clr_mask = err_status & ~SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_SERR_MASK;

        // Read error address if applicable
        err_addr = (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_MASK)
                       ? MMIO_READ32(atu_entry.mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRADDR_ADDRESS)
                       : 0;

        // Determine error severity and code based on the error status
        if (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK)
        {
            severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL;
            err_code = KNG_HM_ARSM_UE;
        }
        else if (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_OF_MASK)
        {
            severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL;
            err_code = KNG_HM_ARSM_OF;
        }
        else if (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_PN_MASK)
        {
            severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL;
            err_code = KNG_HM_ARSM_UE;

            // Write to the poison address register
            MMIO_WRITE32(err_addr, 0x00000000);
        }
        else if (err_status & SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK)
        {
            severity = ACPI_ERROR_SEVERITY_CORRECTED;
            err_code = KNG_HM_ARSM_CE;
            err_clr_mask |= SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_CE_MASK; // 0x11 is used to clear CE bit
        }

        // Clear error status
        MMIO_WRITE32(atu_entry.mscp_start_address + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ADDRESS, err_clr_mask);

        // Submit CPER
        acpi_err_sec_firmware_t sec_fw_cper_section = {.severity = severity,
                                                       .record_id = SCP_ARSM_RAM,
                                                       .param = {err_status, err_addr, err_code, 0}};

        acpi_cper_section_t cper_section;
        cper_section.sec_fw = sec_fw_cper_section;

        hm_submit_cper(ACPI_ERROR_DOMAIN_SCP_PROC, severity, &cper_section, sizeof(cper_section));
    }

    // Unmap the shared SRAM ECC registers
    status = atu_unmap(ATU_ID_MSCP, &atu_entry);
    BUG_ASSERT(status == SILIBS_SUCCESS);

    // Bug check if the error is uncorrectable fatal
    if (severity == ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL)
    {
        BUG_CHECK(err_code, err_status, err_addr);
    }
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

    // Enable ARSM ECC errors
    for (scp_arsm_ram_type_t i = SCP_S_ARSM_RAM; i < SCP_ARSM_RAM_COUNT; i++)
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
}

static void register_scp_ecc_isr(uint32_t irq_num, isr_callback_fn_sans_params_t isr)
{
    nvic_status_t nvic_status = NVIC_STATUS_SUCCESS;

    nvic_status = nvic_irq_set_isr(irq_num, isr);
    BUG_ASSERT_PARAM(nvic_status == NVIC_STATUS_SUCCESS, nvic_status, irq_num);
    nvic_status = nvic_irq_clear_pending(irq_num);
    BUG_ASSERT_PARAM(nvic_status == NVIC_STATUS_SUCCESS, nvic_status, irq_num);
    nvic_status = nvic_irq_enable(irq_num);
    BUG_ASSERT_PARAM(nvic_status == NVIC_STATUS_SUCCESS, nvic_status, irq_num);
}

void register_scp_ecc_isr_with_param(uint32_t irq_num, isr_callback_fn_with_params_t isr, void* isr_param)
{
    nvic_status_t nvic_status = NVIC_STATUS_SUCCESS;

    nvic_status = nvic_irq_set_isr_with_param(irq_num, isr, isr_param);
    BUG_ASSERT_PARAM(nvic_status == NVIC_STATUS_SUCCESS, nvic_status, irq_num);
    nvic_status = nvic_irq_clear_pending(irq_num);
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

    if (!idhw_is_single_die_boot_en())
    {
        // Remote SCP ECC on SCF/RAM0/RAM1
        register_scp_ecc_isr(HW_INT_REMOTE_SCP_RAM_CE_OF_INT, rmss_remote_scp_scfram_bootram_isr);
    }

    // TCM ECC
    register_scp_ecc_isr(HW_INT_SCP_TCM_ECCCE_INT, tcm_ce_isr);       // TCM CE
    register_scp_ecc_isr(HW_INT_SCP_TCM_ECCUE_INT, tcm_ue_isr);       // TCM UE
    register_scp_ecc_isr(HW_INT_SCP_TCM_ECCOF_INT, tcm_overflow_isr); // TCM overflow

    // Data Cache ECC
    register_scp_ecc_isr(HW_INT_DCDET_DATA_UE, dcache_ue_isr);    // D-cache Data RAM UE
    register_scp_ecc_isr(HW_INT_DCDET_DATA_CE, dcache_ce_isr);    // D-cache Data RAM CE
    register_scp_ecc_isr(HW_INT_DCDET_TAG_UE, dcache_tag_ue_isr); // D-cache Tag RAM UE
    register_scp_ecc_isr(HW_INT_DCDET_TAG_CE, dcache_tag_ce_isr); // D-cache Tag RAM CE

    // Instruction Cache ECC
    register_scp_ecc_isr(HW_INT_ICDET_DATA_UE, icache_ue_isr);    // I-cache Data RAM UE
    register_scp_ecc_isr(HW_INT_ICDET_DATA_CE, icache_ce_isr);    // I-cache Data RAM CE
    register_scp_ecc_isr(HW_INT_ICDET_TAG_UE, icache_tag_ue_isr); // I-cache Tag RAM UE
    register_scp_ecc_isr(HW_INT_ICDET_TAG_CE, icache_tag_ce_isr); // I-cache Tag RAM CE

    // ARSM ECC FHI
    KNG_DIE_ID die_id = idsw_get_die_id();
    register_scp_ecc_isr_with_param(HW_INT_SCP_S_ARSM_ECC_FHI_INT,
                                    shared_sram_ecc_isr,
                                    (void*)&s_hm_atu_entries[die_id][SCP_S_ARSM_RAM]); // Secure onchip shared ARSM RAM ECC FHI Interrupt for SCP accesses
    register_scp_ecc_isr_with_param(HW_INT_SCP_NS_ARSM_ECC_FHI_INT,
                                    shared_sram_ecc_isr,
                                    (void*)&s_hm_atu_entries[die_id][SCP_NS_ARSM_RAM]); // Non-secure onchip shared ARSM RAM ECC FHI Interrupt for SCP accesses
    register_scp_ecc_isr_with_param(HW_INT_SCP_RT_ARSM_ECC_FHI_INT,
                                    shared_sram_ecc_isr,
                                    (void*)&s_hm_atu_entries[die_id][SCP_RT_ARSM_RAM]); // Root onchip shared ARSM RAM ECC FHI Interrupt for SCP accesses
    register_scp_ecc_isr_with_param(HW_INT_SCP_RL_ARSM_ECC_FHI_INT,
                                    shared_sram_ecc_isr,
                                    (void*)&s_hm_atu_entries[die_id][SCP_RL_ARSM_RAM]); // Realm onchip shared ARSM RAM ECC FHI Interrupt for SCP accesses
}

void register_scp_error_domain()
{
    // Register the error domain
    hm_register_error_domain(ACPI_ERROR_DOMAIN_SCP_PROC, &SCP_ERROR_DOMAIN_GUID, SCP_PROC_FRU, mscp_error_injection_handler, NULL);

    // Register the error interrupt handlers
    enable_scp_ecc_interrupts();

    // Enable ECC error injection
    enable_scp_ecc_error();
}
