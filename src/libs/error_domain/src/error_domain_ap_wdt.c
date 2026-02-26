//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_domain_ap_wdt.c
 * This file contains ap_wdt ras related functionality.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FPFwInterrupts.h> // for FPFwCoreInterruptRegisterCallback
#include <FpFwAssert.h>     // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>
#include <ap_fw_crashdump.h>
#include <atu_api.h>
#include <bug_check.h>
#include <cper.h>
#include <crash_dump.h> // for crash_dump_type_context_t
#include <ddrss_reserved_regions.h>
#include <error_domain_ap_wdt.h>
#include <error_domain_i.h>
#include <fpfw_init.h>
#include <health_monitor.h>
#include <idhw.h>
#include <idsw_kng.h> // for KNG_DIE_ID, idsw_get_die_id
#include <interrupts.h>
#include <mscp_error_domain.h>
#include <nvic.h>
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#include <startup_shutdown.h>
#include <startup_shutdown_init.h>
#include <utils.h>
#include <warm_start.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <ap_top_regs.h>
#include <ap_watchdog_timer_control_registers_regs.h>
#include <scp_top_regs.h>

/*-- Symbolic Constant Macros (defines) --*/
#define AP_WDT_FRU "AP_WDT"

// For AP Watchdog Non-Secure Control Frame
#define ATU_MAPPING_AP_WDOG_NS(die_id)                                                                       \
    ATU_MAPPING((die_id == 0 ? AP_TOP_D0_AP_WDOG_C_FRAME_NS_ADDRESS : AP_TOP_D1_AP_WDOG_C_FRAME_NS_ADDRESS), \
                0,                                                                                           \
                (ALIGN_UP(AP_TOP_D0_AP_WDOG_C_FRAME_NS_SIZE, ATU_PAGE_SIZE) - 1),                            \
                {ATU_BUS_ATTR_ROOT})

// For AP Watchdog Secure Control Frame
#define ATU_MAPPING_AP_WDOG_S(die_id)                                                                          \
    ATU_MAPPING((die_id == 0 ? AP_TOP_D0_AP_R_WDOG_C_FRAME_S_ADDRESS : AP_TOP_D1_AP_R_WDOG_C_FRAME_S_ADDRESS), \
                0,                                                                                             \
                (ALIGN_UP(AP_TOP_D0_AP_R_WDOG_C_FRAME_S_SIZE, ATU_PAGE_SIZE) - 1),                             \
                {ATU_BUS_ATTR_ROOT})

#define CRASH_DUMP_MAX_RETRY_COUNT    10
#define CRASH_DUMP_MAX_RETRY_DELAY_US 1000000 // 1 seconds

/*-------- Function Prototypes -----------*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
static guid_t AP_WDT_GUID = ACPI_ERROR_TYPE_VENDOR_AP_WDT;

// GUID for AP WDT custom chunk
// {4E4E2C2B-5416-4800-B1D1-59B2C87B3614}
static const GUID CD_AP_WDT_CHUNK_GUID = {0x4e4e2c2b, 0x5416, 0x4800, {0xb1, 0xd1, 0x59, 0xb2, 0xc8, 0x7b, 0x36, 0x14}};

/*-------------- Functions ---------------*/

// Callback to get payload size
static uint32_t ap_wdt_chunk_get_size(void* userContext)
{
    uint32_t retry_count = 0;
    volatile kng_ap_fw_hdr* fw_hdr = (volatile kng_ap_fw_hdr*)userContext;

    // Check cd_status before getting size.
    while (fw_hdr->cd_status != KNG_CD_STATUS_COMPLETED && retry_count < CRASH_DUMP_MAX_RETRY_COUNT)
    {
        FPFW_DBGPRINT_WARNING("AP crashdump not ready. cd_status: 0x%08x. Retrying... (%u/%u)\n",
                              fw_hdr->cd_status,
                              retry_count + 1,
                              CRASH_DUMP_MAX_RETRY_COUNT);
        SLEEP_US(CRASH_DUMP_MAX_RETRY_DELAY_US);
        retry_count++;
    }

    if (fw_hdr->cd_status != KNG_CD_STATUS_COMPLETED)
    {
        FPFW_DBGPRINT_WARNING("AP crashdump transfer not complete.\n");
    }

    // Return the size from the mapped header, rounded up to natural alignment
    return DUMP_ROUND_UP(fw_hdr->size, DUMP_NATURAL_ALIGNMENT);
}

// Callback to write payload
static void ap_wdt_chunk_write(FPFwCrashDumpCtx* ctx, uint64_t dumpOffset, void* userContext, uint32_t payloadSize)
{
    FPFW_UNUSED(payloadSize);
    uint32_t retry_count = 0;
    volatile kng_ap_fw_hdr* fw_hdr = (volatile kng_ap_fw_hdr*)userContext;

    // Check cd_status before memcpy.
    while (fw_hdr->cd_status != KNG_CD_STATUS_COMPLETED && retry_count < CRASH_DUMP_MAX_RETRY_COUNT)
    {
        FPFW_DBGPRINT_WARNING("AP crashdump not ready. cd_status: 0x%08x. Retrying... (%u/%u)\n",
                              fw_hdr->cd_status,
                              retry_count + 1,
                              CRASH_DUMP_MAX_RETRY_COUNT);
        SLEEP_US(CRASH_DUMP_MAX_RETRY_DELAY_US);
        retry_count++;
    }

    if (fw_hdr->cd_status == KNG_CD_STATUS_COMPLETED)
    {
        FPFW_DBGPRINT_INFO("AP crashdump transfer complete (size=%d).\n", fw_hdr->size);
    }

    // Copy the entire payload from the mapped AP_FW_CRASHDUMP_LOCATION
    ctx->memAPIs.FPFwCDMemcpyLocalToGlobal(&ctx->memAPIs, dumpOffset, (void*)fw_hdr, fw_hdr->size);
}

uint32_t map_ap_wdog_address(atu_map_entry_t* atu_entry, bool secure)
{
    BUG_ASSERT(atu_entry != NULL);
    KNG_DIE_ID die_id = idsw_get_die_id();

    if (secure)
    {
        *atu_entry = (atu_map_entry_t)ATU_MAPPING_AP_WDOG_S(die_id);
    }
    else
    {
        *atu_entry = (atu_map_entry_t)ATU_MAPPING_AP_WDOG_NS(die_id);
    }

    int status = atu_map(ATU_ID_MSCP, atu_entry);
    BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, 0);

    return atu_entry->mscp_start_address;
}

void unmap_ap_wdog_address(atu_map_entry_t* atu_entry)
{
    int status = atu_unmap(ATU_ID_MSCP, atu_entry);
    BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, 0);
}

/**
 * @brief AP Watchdog ISR handler
 *
 * This function handles AP watchdog timeout interrupts, logs the event,
 * stores warm start data, submits CPER record, registers crash dump data,
 * and triggers a bug check.
 *
 * @param p_context Context parameter (unused for AP WDT)
 */

void hm_ap_wdt_isr()
{
    // Clear the interrupt - get current IRQ and clear it
    uint32_t nvic_irq_num = 0;
    int nvic_status = nvic_get_current_irq(&nvic_irq_num);
    if (nvic_status == NVIC_STATUS_SUCCESS)
    {
        nvic_irq_clear_pending(nvic_irq_num);
    }

    // Store warm start data indicating AP WDT occurred
    bool ap_wdt_occurred = true;
    ws_data_put(WARM_START_ID_AP_WDT, &ap_wdt_occurred, sizeof(ap_wdt_occurred));

    // Create and submit CPER record
    acpi_err_sec_firmware_t sec_fw_cper_section = {.severity = ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL,
                                                   .record_id = RECORD_ID_AP_WDT, // AP_WDT record ID
                                                   .param = {nvic_irq_num, 0, 0}};

    acpi_cper_section_t cper_section = {0};
    cper_section.sec_fw = sec_fw_cper_section;

    // Submit CPER using AP_WDT error domain
    hm_submit_cper(ACPI_ERROR_DOMAIN_AP_WDT, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL, &cper_section, sizeof(cper_section));

    // Map and read AP watchdog registers
    atu_map_entry_t ap_wdog_ns_atu_entry;
    atu_map_entry_t ap_wdog_s_atu_entry;

    // Map Non-Secure watchdog
    uint32_t ns_base = map_ap_wdog_address(&ap_wdog_ns_atu_entry, false);
    static ap_watchdog_timer_control_registers_wcs ns_wcs;
    ns_wcs.as_uint32 = MMIO_READ32(ns_base + AP_WATCHDOG_TIMER_CONTROL_REGISTERS_WCS_ADDRESS);

    // Map Secure watchdog
    uint32_t s_base = map_ap_wdog_address(&ap_wdog_s_atu_entry, true);
    static ap_watchdog_timer_control_registers_wcs s_wcs;
    s_wcs.as_uint32 = MMIO_READ32(s_base + AP_WATCHDOG_TIMER_CONTROL_REGISTERS_WCS_ADDRESS);
    FPFW_DBGPRINT_INFO("Registering crash dump data.\n");
    // Register crash dump data
    crash_dump_register_address32(&ns_wcs, sizeof(ns_wcs), FPFW_CD_DUMP_PRIORITY_CRITICAL);
    crash_dump_register_address32(&s_wcs, sizeof(s_wcs), FPFW_CD_DUMP_PRIORITY_CRITICAL);

    // Unmap
    unmap_ap_wdog_address(&ap_wdog_ns_atu_entry);
    unmap_ap_wdog_address(&ap_wdog_s_atu_entry);

    KNG_DIE_ID die_id = idsw_get_die_id();

    if (die_id == 0)
    {
        // Map AP FW crashdump header
        atu_map_entry_t crashdump_atu_entry;
        int status = SILIBS_SUCCESS;

        crashdump_atu_entry = (atu_map_entry_t)ATU_MAPPING(AP_FW_CRASHDUMP_LOCATION,
                                                           0,
                                                           (ALIGN_UP(AP_FW_CRASHDUMP_REGION_SIZE, ATU_PAGE_SIZE) - 1),
                                                           {ATU_BUS_ATTR_ROOT});

        status = atu_map(ATU_ID_MSCP, &crashdump_atu_entry);
        BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, 0);

        kng_ap_fw_hdr* fw_hdr = (kng_ap_fw_hdr*)crashdump_atu_entry.mscp_start_address;

        // Register custom chunk with mapped crashdump address as userContext
        CdRegisterCustomChunk(&crash_dump_context()->type_ctx[CRASH_DUMP_TYPE_FULL]->crash_dump_ctx,
                              (GUID)CD_AP_WDT_CHUNK_GUID,
                              fw_hdr,
                              0,
                              ap_wdt_chunk_get_size,
                              ap_wdt_chunk_write,
                              FPFW_CD_DUMP_PRIORITY_CRITICAL);

        // Trigger bug check for AP watchdog timeout
        BUG_CHECK(KNG_AP_WDT_TIMEOUT, nvic_irq_num, RECORD_ID_AP_WDT);
        // Unmap after reading
        atu_unmap(ATU_ID_MSCP, &crashdump_atu_entry);
    }
}

/**
 * @brief Enable and register AP watchdog interrupts
 *
 * This function registers the AP watchdog ISR for the appropriate interrupt vectors.
 */
static void enable_ap_wdt_interrupts(void)
{
    nvic_status_t intr_status = 0;

    // Register AP Non-Secure Watchdog WS1 interrupt
    intr_status = FPFwCoreInterruptRegisterCallback(HW_INT_AP_NS_WDOG_WS1_INT, hm_ap_wdt_isr, (void*)HW_INT_AP_NS_WDOG_WS1_INT);
    intr_status |= FPFwCoreInterruptEnableVector(HW_INT_AP_NS_WDOG_WS1_INT);
    BUG_ASSERT_PARAM(intr_status == 0, intr_status, HW_INT_AP_NS_WDOG_WS1_INT);

    // Register AP Secure Watchdog WS0 interrupt
    intr_status = FPFwCoreInterruptRegisterCallback(HW_INT_AP_S_WDOG_WS0_INT, hm_ap_wdt_isr, (void*)HW_INT_AP_S_WDOG_WS0_INT);
    intr_status |= FPFwCoreInterruptEnableVector(HW_INT_AP_S_WDOG_WS0_INT);
    BUG_ASSERT_PARAM(intr_status == 0, intr_status, HW_INT_AP_S_WDOG_WS0_INT);

    FPFW_DBGPRINT_INFO("AP WDT interrupt registration\n");
}

void register_ap_wdt_error_domain()
{
    uint32_t size = 0;
    bool ap_wdt_occurred_prev_boot = false;

    static startup_shutdown_request_t shutdown_request;
    DfwkAsyncRequestInitialize((void*)&shutdown_request.header, sizeof(shutdown_request));

    bool* p_apt_wdt_occurred_ws = ws_data_get(WARM_START_ID_AP_WDT, &size);

    if (p_apt_wdt_occurred_ws == NULL)
    {
        ap_wdt_occurred_prev_boot = false;
    }
    else
    {
        ap_wdt_occurred_prev_boot = *p_apt_wdt_occurred_ws;
    }

    if (ap_wdt_occurred_prev_boot)
    {
        // If AP WDT occurred on a previous boot, log the event and do not register the error domain or enable interrupts to avoid potential repeated crashes.
        // The AP WDT error domain will be registered and interrupts will be enabled on the next cold boot.
        FPFW_DBGPRINT_INFO("AP WDT occurred on a previous boot.\n");
    }
    else
    {
        //  Register the error domain
        hm_register_error_domain(ACPI_ERROR_DOMAIN_AP_WDT, &AP_WDT_GUID, AP_WDT_FRU, hm_ap_wdt_error_injection_handler, NULL);

        // Enable AP WDT interrupts
        enable_ap_wdt_interrupts();
    }
}
