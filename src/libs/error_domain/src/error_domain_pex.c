//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_domain_pex.c
 * This file contains pex ras related functionality.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>
#include <bug_check.h>
#include <cper.h>
#include <dvfs.h>
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
#include <tx_api.h>
#include <tx_timer.h>
#include <utils.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <scp_top_regs.h>

/*-- Symbolic Constant Macros (defines) --*/
#define PEX_FRU "PEX"

/*-------- Function Prototypes -----------*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
static guid_t PEX_GUID = ACPI_ERROR_TYPE_VENDOR_PEX;
static vptr_scp_pwr_ctrl_reg scp_pwr_ctrl_regs = (vptr_scp_pwr_ctrl_reg)(SCP_TOP_SCP_PWR_CTRL_ADDRESS);
static TX_TIMER pex_poll_timer;
static pex_rng_config_t* g_rng_cfg = NULL;
static bool pex_polling_initialized = false;

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
                                                       .record_id = RECORD_ID_PEX,
                                                       .param = {status, KNG_PEX_RNG_ERR, 0}};

        acpi_cper_section_t cper_section;
        cper_section.sec_fw = sec_fw_cper_section;

        hm_submit_cper(ACPI_ERROR_DOMAIN_SCP_PROC, ACPI_ERROR_SEVERITY_CORRECTED, &cper_section, sizeof(cper_section));
    }
}

// Timer callback function for PEX polling
static void pex_poll_timer_callback(ULONG timer_input)
{
    FPFW_UNUSED(timer_input);

    if (g_rng_cfg == NULL)
    {
        return;
    }

    for (unsigned int core = 0; core < g_rng_cfg->core_count; ++core)
    {
        const corebits_t* enabled_cores = g_rng_cfg->platform_cores_in_die;
        if (!corebits_is_bit_set(enabled_cores, core))
        {
            continue;
        }
        const uintptr_t cluster_pex_base_addr = (g_rng_cfg->cluster_pex_base + (g_rng_cfg->cluster_stride * core));
        uint32_t ap_rng_base = cluster_pex_base_addr + PEX_RNG_ADDRESS;

        uint32_t scp_irq = 0;
        scp_irq = dvfs_get_pex_scp_irq(cluster_pex_base_addr);

        if ((scp_irq & 0x1) != 0)
        {
            FPFW_DBGPRINT_INFO("rng_error occurred\n");
            FPFW_DBGPRINT_INFO("Reset RNG IP\n");

            // Reset the RNG IP by disabling and re-enabling it
            reset_pex_rng(ap_rng_base);

            schedule_pex_error_handling_dfwk(g_rng_cfg);
        }

        if ((scp_irq & 0x2) != 0)
        {
            FPFW_DBGPRINT_INFO("dvfs_telem_overflow error occurred");
        }

        if ((scp_irq & 0x4) != 0)
        {
            FPFW_DBGPRINT_INFO("pvt_irq error occurred");
        }

        // Clear PEX errors
        if (scp_irq != 0)
        {
            dvfs_clr_pex_scp_irq(cluster_pex_base_addr, scp_irq);
        }
    }
}

// API to start PEX polling using ThreadX timer
static int32_t start_pex_polling(uint32_t poll_interval_ms)
{
    if (pex_polling_initialized)
    {
        FPFW_DBGPRINT_WARNING("PEX polling already initialized\n");
        return TX_SUCCESS;
    }

    // Convert milliseconds to ThreadX ticks
    ULONG timer_ticks = (poll_interval_ms * TX_TIMER_TICKS_PER_SECOND) / 1000;
    if (timer_ticks == 0)
    {
        timer_ticks = 1; // Minimum 1 tick
    }

    // Create the polling timer
    UINT status = tx_timer_create(&pex_poll_timer,
                                  "PEX Poll Timer",        // Timer name
                                  pex_poll_timer_callback, // Timer callback function
                                  0,                       // Timer input (unused)
                                  timer_ticks,             // Initial ticks
                                  timer_ticks,             // Reschedule ticks (periodic)
                                  TX_AUTO_ACTIVATE);       // Auto-activate timer

    if (status != TX_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("Failed to create PEX polling timer, status: 0x%x\n", status);
        return status;
    }

    pex_polling_initialized = true;
    FPFW_DBGPRINT_INFO("PEX polling started with %lu ms interval\n", poll_interval_ms);

    return TX_SUCCESS;
}

// Replace the enable_pex_interrupts function
static void enable_pex_polling()
{
    // Start PEX polling with 100ms interval (adjust as needed)
    const uint32_t POLL_INTERVAL_MS = 100;

    int32_t status = start_pex_polling(POLL_INTERVAL_MS);
    if (status != TX_SUCCESS)
    {
        FPFW_DBGPRINT_ERROR("Failed to start PEX polling, status: 0x%x\n", status);
        BUG_CHECK(KNG_BGCHK_BUGCHECK, status, POLL_INTERVAL_MS);
    }
}

void register_pex_error_domain(pex_rng_config_t* pex_config)
{
    FPFW_RUNTIME_ASSERT(pex_config != NULL);

    // Store the configuration for use in callbacks
    g_rng_cfg = pex_config;

    //  Register the error domain
    hm_register_error_domain(ACPI_ERROR_DOMAIN_PEX, &PEX_GUID, PEX_FRU, pex_error_injection_handler, NULL);

    // Register the error polling instead of interrupt handlers
    // ADO: 2885632
    enable_pex_polling();
}
