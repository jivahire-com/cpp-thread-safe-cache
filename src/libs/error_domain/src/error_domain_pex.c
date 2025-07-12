//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_domain_pex.c
 * This file contains pex ras related functionality.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <cper.h>
#include <dvfs_regs.h> // for (anonymous union)::(anonymous), dvfs_...
#include <error_domain_i.h>
#include <error_domain_pex.h>
#include <fpfw_init.h>
#include <health_monitor.h>
#include <idhw.h>
#include <idsw_kng.h> // for KNG_DIE_ID, idsw_get_die_id
#include <interrupts.h>
#include <limits.h> // for CHAR_BIT
#include <mscp_error_domain.h>
#include <nvic.h>
#include <pex_regs.h>
#include <pex_rng.h>
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <scp_top_regs.h>

/*-- Symbolic Constant Macros (defines) --*/
#define PEX_FRU "PEX"
#define ACPI_ERROR_TYPE_PEX                                \
    {                                                      \
        0x3f9d6b20, 0x482e, 0x4c3f,                        \
        {                                                  \
            0xa1, 0x72, 0x8b, 0x5c, 0xf4, 0x92, 0x6e, 0xd8 \
        }                                                  \
    }

/*-------- Function Prototypes -----------*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
static guid_t PEX_GUID = ACPI_ERROR_TYPE_PEX;
static vptr_scp_pwr_ctrl_reg scp_pwr_ctrl_regs = (vptr_scp_pwr_ctrl_reg)(SCP_TOP_SCP_PWR_CTRL_ADDRESS);

/*-------------- Functions ---------------*/

void cons_pex_isr(void* context)
{
    pex_rng_config_t* rng_cfg = (pex_rng_config_t*)context;
    KNG_DIE_ID die_num = idsw_get_die_id(); //! get the die id

    scp_pwr_ctrl_proc_pex_int_status0 pex_int_status = {0};
    uint32_t pex_int_status_size = sizeof(pex_int_status.as_uint32) * CHAR_BIT;

    // Find PEX number that caused the interrupt
    for (uint32_t status_reg = 0; status_reg < 3; ++status_reg)
    {
        pex_int_status.as_uint32 = MMIO_READ32((uint32_t*)(&scp_pwr_ctrl_regs->proc_pex_int_status0 + status_reg));
        for (uint32_t index = 0; index < pex_int_status_size; index++)
        {
            uint32_t pex_num = 0;
            if (pex_int_status.proc_pex_int_status & (1U << index))
            {
                pex_num = (index + (status_reg * pex_int_status_size));
                pex_irq_handle(die_num, pex_num, rng_cfg);
            }
        }
    }
}

void pex_irq_handle(KNG_DIE_ID die_num, uint32_t pex_num, pex_rng_config_t* rng_cfg)
{
    FPFW_UNUSED(die_num);

    uint32_t status = MMIO_READ32((uint32_t*)&scp_pwr_ctrl_regs->proc_pex_int_status0 + (pex_num / 32));

    const uintptr_t cluster_pex_base_addr = (rng_cfg->cluster_pex_base + (rng_cfg->cluster_stride * pex_num));
    uint32_t ap_rng_base = cluster_pex_base_addr + PEX_RNG_ADDRESS;
    ptr_dvfs_reg dvfs_regs = (ptr_dvfs_reg)(cluster_pex_base_addr + PEX_DVFS_ROOT_ADDRESS);

    dvfs_scp_irq scp_irq = {.as_uint32 = MMIO_READ32((uint32_t*)&dvfs_regs->scp_irq)};
    if (scp_irq.rng_error == 1)
    {
        // Reset the RNG IP by disabling and re-enabling it
        reset_pex_rng(ap_rng_base);
    }

    // Clear interrupt source
    uint32_t nvic_irq_num = 0;
    int nvic_status = nvic_get_current_irq(&nvic_irq_num);
    if (nvic_status == NVIC_STATUS_SUCCESS)
    {
        nvic_irq_clear_pending(nvic_irq_num);
    }

    MMIO_SET_MASK32(((uint32_t*)&scp_pwr_ctrl_regs->proc_pex_int_status0 + (pex_num / 32)), (1U << (pex_num % 32)));

    if (status & (1U << (pex_num % 32)))
    {
        // Submit CPER
        acpi_err_sec_firmware_t sec_fw_cper_section = {.severity = ACPI_ERROR_SEVERITY_CORRECTED,
                                                       .record_id = (guid_t)PEX_GUID,
                                                       .param = {status, KNG_PEX_RNG_ERR, 0}};

        acpi_cper_section_t cper_section;
        cper_section.sec_fw = sec_fw_cper_section;

        hm_submit_cper(ACPI_ERROR_DOMAIN_SCP_PROC, ACPI_ERROR_SEVERITY_CORRECTED, &cper_section, sizeof(cper_section));
    }
}

static void enable_pex_interrupts()
{
    // Temporary disable PEX interrupts to avoid spurious interrupts on Silicon
    // Cluster PEX Int
    // pex_rng_config_t* rng_cfg = (pex_rng_config_t*)fpfw_init_get_handle("pex_rng");
    // register_scp_ecc_isr_with_param(HW_INT_CPU_CLSTR_31_0_PEX_INT, cons_pex_isr, rng_cfg); // Clusters 0-31 PEX Interrupts
    // register_scp_ecc_isr_with_param(HW_INT_CPU_CLSTR_63_32_PEX_INT, cons_pex_isr, rng_cfg); // Clusters 32-63 PEX Interrupts
    // register_scp_ecc_isr_with_param(HW_INT_CPU_CLSTR_67_64_PEX_INT, cons_pex_isr, rng_cfg); // Clusters 64-67 (64-95) PEX Interrupts
    // register_scp_ecc_isr_with_param(HW_INT_CPU_CLSTR_127_96_PEX_INT, cons_pex_isr, rng_cfg); // Clusters 96-127 PEX Interrupts
}

void register_pex_error_domain()
{
    //  Register the error domain
    hm_register_error_domain(ACPI_ERROR_DOMAIN_PEX, &PEX_GUID, PEX_FRU, mscp_error_injection_handler, NULL);

    // Register the error interrupt handlers
    enable_pex_interrupts();
}
