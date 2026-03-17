//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_error_domain_ap_wdt_isr.cpp
 * Tests for the AP watchdog ISR functionality
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expect...

extern "C" {
#include <FPFwInterrupts.h> // for FPFwCoreInterruptRegisterCallback, FPFwCoreInterruptEnableVector
#include <FpFwUtils.h>      // for FPFW_UNUSED
#include <ap_fw_crashdump.h>
#include <ap_watchdog_timer_control_registers_regs.h>
#include <atu_api.h>
#include <atu_lib.h> // for atu_map_entry_t
#include <cper.h>
#include <crash_dump.h>
#include <error_domain_ap_wdt.h>
#include <health_monitor.h>
#include <idsw_kng.h> // for KNG_DIE_ID, idsw_get_die_id
#include <interrupts.h>
#include <mscp_error_domain.h>
#include <nvic.h> // for nvic_status_t
#include <startup_shutdown.h>
#include <stdint.h>
#include <warm_start.h>
#include <warm_start_id.h>

/*-- Symbolic Constant Macros (defines) --*/
#define TEST_IRQ_NUM      111
#define TEST_NS_BASE_ADDR 0x2a440000U
#define TEST_S_BASE_ADDR  0x2a460000U
#define TEST_WCS_VALUE    0x12345678U
#define RECORD_ID_AP_WDT  0x4000

// Mock FPFW_DBGPRINT_ERROR as empty macro
#define FPFW_DBGPRINT_ERROR(...)
#define FPFW_DBGPRINT_INFO(...)

#define CRASH_DUMP_BUFFER_SIZE 256

/*------------- Typedefs -----------------*/
// Add missing typedef for ap_wdt_chunk_t
typedef struct
{
    uint32_t size;
} ap_wdt_chunk_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static bool test_ap_wdt_occurred = false;

// Mock crash dump context
static FPFwCrashDumpCtx mock_crash_dump_ctx;
static crash_dump_type_context_t mock_type_ctx;
static crash_dump_context_t mock_cd_context;

// Mock AP FW crashdump header buffer
static uint8_t mock_crashdump_buffer[CRASH_DUMP_BUFFER_SIZE];
static FPFW_CD_CUSTOM_CHUNK_GET_SIZE_CALLBACK mock_ap_wdt_chunk_get_size = NULL;
static FPFW_CD_CUSTOM_CHUNK_UPDATE_CALLBACK mock_ap_wdt_chunk_write = NULL;
static uint32_t mock_ap_wdt_payload_size = 0;

/* Allow tests to control NVIC status so we can cover both branches in hm_ap_wdt_isr() */
static nvic_status_t g_nvic_get_current_irq_status = NVIC_STATUS_SUCCESS;

/* Optional: used to assert payloadSize passed to CdRegisterCustomChunk */
static uint32_t g_expected_chunk_payload_size = 0;
static void* mock_ap_wdt_client_ctx = NULL;

/*------------- Functions ----------------*/
//
// Mocks
//

crash_dump_context_t* __wrap_crash_dump_context(void)
{
    function_called();
    return &mock_cd_context;
}

nvic_status_t __wrap_nvic_get_current_irq(uint32_t* irq_num)
{
    assert_non_null(irq_num);
    *irq_num = mock_type(uint32_t);
    function_called();

    return g_nvic_get_current_irq_status;
}

nvic_status_t __wrap_nvic_irq_clear_pending(uint32_t irq_num)
{
    check_expected(irq_num);
    function_called();
    return NVIC_STATUS_SUCCESS;
}

void* __wrap_ws_data_put(mod_ws_data_id_t id, void* data, uint32_t size)
{
    check_expected(id);
    assert_non_null(data);
    check_expected(size);

    // Store the data for verification
    if (id == WARM_START_ID_AP_WDT && size == sizeof(bool))
    {
        test_ap_wdt_occurred = *(bool*)data;
    }

    function_called();
    return data;
}

void* __wrap_ws_data_get(mod_ws_data_id_t id, uint32_t* p_size)
{
    check_expected(id);
    assert_non_null(p_size);
    *p_size = sizeof(bool);

    function_called();

    return mock_type(void*);
}

void __wrap_hm_submit_cper(uint16_t error_domain_idx, acpi_error_severity_t err_severity, void* err_record_section, uint32_t err_record_section_size)
{
    check_expected(error_domain_idx);
    check_expected(err_severity);
    assert_non_null(err_record_section);
    check_expected(err_record_section_size);
    function_called();
}

int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_entry)
{
    check_expected(atu_id);
    assert_non_null(atu_entry);

    // Set mock address based on secure/non-secure
    atu_entry->mscp_start_address = mock_type(uint32_t);

    function_called();
    return SILIBS_SUCCESS;
}

int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_entry)
{
    check_expected(atu_id);
    assert_non_null(atu_entry);
    function_called();
    return SILIBS_SUCCESS;
}

uint32_t __wrap_mmio_read32(volatile uint32_t* addr)
{
    check_expected(addr);
    function_called();
    return mock_type(uint32_t);
}

void __wrap_crash_dump_register_address32(void* addr, uint32_t size, uint32_t priority)
{
    assert_non_null(addr);
    check_expected(size);
    check_expected(priority);
    function_called();
}

void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    FPFW_UNUSED(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);
    function_called();
}
KNG_DIE_ID __wrap_idsw_get_die_id(void)
{
    function_called();
    return mock_type(KNG_DIE_ID);
}

bool __wrap_CdRegisterCustomChunk(FPFwCrashDumpCtx* ctx,
                                  GUID signature,
                                  void* clientContext,
                                  uint32_t payloadSize,
                                  FPFW_CD_CUSTOM_CHUNK_GET_SIZE_CALLBACK getSizeCallback,
                                  FPFW_CD_CUSTOM_CHUNK_UPDATE_CALLBACK updateCallback,
                                  FPFwCdDumpPriority priority)
{
    FPFW_UNUSED(signature);
    assert_non_null(ctx);
    assert_non_null(clientContext);
    assert_non_null(getSizeCallback);
    assert_non_null(updateCallback);

    /* After COB-99 fix, hm_ap_wdt_isr may pass a validated fixed payload size */
    if (g_expected_chunk_payload_size != 0)
    {
        assert_int_equal(payloadSize, g_expected_chunk_payload_size);
    }

    mock_ap_wdt_payload_size = payloadSize;
    mock_ap_wdt_client_ctx = clientContext;

    assert_int_equal(priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);

    mock_ap_wdt_chunk_get_size = getSizeCallback;
    mock_ap_wdt_chunk_write = updateCallback;

    function_called();
    return true;
}

void __wrap_DfwkAsyncRequestInitialize(PDFWK_ASYNC_REQUEST_HEADER Request, size_t RequestSize)
{
    assert_non_null(Request);
    assert_int_equal(RequestSize, sizeof(startup_shutdown_request_t));

    function_called();
}

void __wrap_hm_register_error_domain(uint16_t error_domain_idx,
                                     const guid_t* error_domain_guid,
                                     const char* error_domain_name,
                                     hm_error_injection_cb_t err_inject_cb,
                                     void* err_inject_ctx)
{
    assert_int_equal(error_domain_idx, ACPI_ERROR_DOMAIN_AP_WDT);
    assert_non_null(error_domain_guid);
    assert_string_equal(error_domain_name, "AP_WDT");
    assert_non_null(err_inject_cb);
    assert_null(err_inject_ctx);

    function_called();
}

uint32_t __wrap_FPFwCoreInterruptRegisterCallback(uint32_t irqnum, FPFwCoreInterruptHandler handler, void* arg)
{
    check_expected(irqnum);
    assert_non_null(handler);
    assert_non_null(arg);
    function_called();

    return 0; // Return success
}

uint32_t __wrap_FPFwCoreInterruptEnableVector(uint32_t irqnum)
{
    check_expected(irqnum);
    function_called();

    return 0; // Return success
}

uint64_t mock_FPFwCDMemcpyLocalToGlobal(struct FpFwCDMemoryAPIs* memAPIs, uint64_t dest, void* src, size_t n)
{
    assert_non_null(memAPIs);
    check_expected(dest);
    assert_non_null(src);
    check_expected(n);

    function_called();

    return 0;
}
}
/*------------- Test Functions -----------*/

TEST_FUNCTION(test_hm_ap_wdt_isr_success, nullptr, nullptr)
{
    g_nvic_get_current_irq_status = NVIC_STATUS_SUCCESS;
    test_ap_wdt_occurred = false;
    g_expected_chunk_payload_size = 0;

    // Initialize mock crash dump context
    memset(&mock_crash_dump_ctx, 0, sizeof(mock_crash_dump_ctx));
    memset(&mock_type_ctx, 0, sizeof(mock_type_ctx));
    memset(&mock_cd_context, 0, sizeof(mock_cd_context));
    mock_crash_dump_ctx.memAPIs.FPFwCDMemcpyLocalToGlobal = mock_FPFwCDMemcpyLocalToGlobal;
    mock_type_ctx.crash_dump_ctx = mock_crash_dump_ctx;
    mock_cd_context.type_ctx[CRASH_DUMP_TYPE_FULL] = &mock_type_ctx;

    // Initialize mock crashdump buffer with completed status
    memset(mock_crashdump_buffer, 0, sizeof(mock_crashdump_buffer));
    kng_ap_fw_hdr* mock_fw_hdr = (kng_ap_fw_hdr*)mock_crashdump_buffer;
    mock_fw_hdr->magic = 0xDEADBEEF;                  // Magic value
    mock_fw_hdr->version_major = 1;                   // Version
    mock_fw_hdr->version_minor = 0;                   // Version
    mock_fw_hdr->size = 0x100;                        // Test payload size (256 bytes)
    mock_fw_hdr->cd_status = KNG_CD_STATUS_COMPLETED; // Crashdump completed

    // Mock nvic_get_current_irq to return success
    expect_function_call(__wrap_nvic_get_current_irq);
    will_return(__wrap_nvic_get_current_irq, TEST_IRQ_NUM); // For *irq_num = mock_type(uint32_t)

    // Expect NVIC interrupt clear
    expect_value(__wrap_nvic_irq_clear_pending, irq_num, TEST_IRQ_NUM);
    expect_function_call(__wrap_nvic_irq_clear_pending);

    // Expect warm start data storage
    expect_value(__wrap_ws_data_put, id, WARM_START_ID_AP_WDT);
    expect_value(__wrap_ws_data_put, size, sizeof(bool));
    expect_function_call(__wrap_ws_data_put);

    // Expect CPER submission
    expect_value(__wrap_hm_submit_cper, error_domain_idx, ACPI_ERROR_DOMAIN_AP_WDT);
    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));
    expect_function_call(__wrap_hm_submit_cper);

    // First call to map_ap_wdog_address (NS) - idsw_get_die_id then atu_map
    will_return(__wrap_idsw_get_die_id, 0);
    expect_function_call(__wrap_idsw_get_die_id);

    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, TEST_NS_BASE_ADDR);
    expect_function_call(__wrap_atu_map);

    expect_value(__wrap_mmio_read32, addr, TEST_NS_BASE_ADDR + AP_WATCHDOG_TIMER_CONTROL_REGISTERS_WCS_ADDRESS);
    will_return(__wrap_mmio_read32, TEST_WCS_VALUE);
    expect_function_call(__wrap_mmio_read32);

    // Second call to map_ap_wdog_address (S) - idsw_get_die_id then atu_map
    will_return(__wrap_idsw_get_die_id, 0);
    expect_function_call(__wrap_idsw_get_die_id);

    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, TEST_S_BASE_ADDR);
    expect_function_call(__wrap_atu_map);

    expect_value(__wrap_mmio_read32, addr, TEST_S_BASE_ADDR + AP_WATCHDOG_TIMER_CONTROL_REGISTERS_WCS_ADDRESS);
    will_return(__wrap_mmio_read32, TEST_WCS_VALUE);
    expect_function_call(__wrap_mmio_read32);

    // Expect crash dump registration (twice)
    expect_value(__wrap_crash_dump_register_address32, size, sizeof(ap_watchdog_timer_control_registers_wcs));
    expect_value(__wrap_crash_dump_register_address32, priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    expect_function_call(__wrap_crash_dump_register_address32);

    expect_value(__wrap_crash_dump_register_address32, size, sizeof(ap_watchdog_timer_control_registers_wcs));
    expect_value(__wrap_crash_dump_register_address32, priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    expect_function_call(__wrap_crash_dump_register_address32);

    // Expect ATU unmapping (twice - for NS and S)
    expect_value(__wrap_atu_unmap, atu_id, ATU_ID_MSCP);
    expect_function_call(__wrap_atu_unmap);

    expect_value(__wrap_atu_unmap, atu_id, ATU_ID_MSCP);
    expect_function_call(__wrap_atu_unmap);

    // Third call to idsw_get_die_id for AP FW crashdump mapping
    will_return(__wrap_idsw_get_die_id, 0);
    expect_function_call(__wrap_idsw_get_die_id);

    // Expect atu_map for AP FW crashdump region
    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, (uint32_t)mock_crashdump_buffer); // Return address of mock buffer
    expect_function_call(__wrap_atu_map);

    // Expect crash_dump_context call
    expect_function_call(__wrap_crash_dump_context);

    // Expect custom chunk registration
    expect_function_call(__wrap_CdRegisterCustomChunk);

    // Expect crash dump bug check (since cd_status is KNG_CD_STATUS_COMPLETED)
    expect_function_call(__wrap_crash_dump_bug_check);

    // Expect atu_unmap after reading cd_status
    expect_value(__wrap_atu_unmap, atu_id, ATU_ID_MSCP);
    expect_function_call(__wrap_atu_unmap);

    // Call the ISR
    hm_ap_wdt_isr();

    /* Ensure callbacks were registered and we captured the client context */
    assert_non_null(mock_ap_wdt_chunk_get_size);
    assert_non_null(mock_ap_wdt_chunk_write);
    assert_non_null(mock_ap_wdt_client_ctx);

    // Verify warm start data was set
    assert_true(test_ap_wdt_occurred);

    // Set up mock callbacks for custom chunk

    // After COB-99 fix, callbacks expect the opaque clientContext, not kng_ap_fw_hdr* */
    uint32_t payload_size = mock_ap_wdt_chunk_get_size(mock_ap_wdt_client_ctx);

    expect_value(mock_FPFwCDMemcpyLocalToGlobal, dest, 0x1234);
    expect_value(mock_FPFwCDMemcpyLocalToGlobal, n, payload_size);
    expect_function_call(mock_FPFwCDMemcpyLocalToGlobal);

    // IMPORTANT: pass clientContext (opaque) and payload_size (even if production ignores payloadSize)
    mock_ap_wdt_chunk_write(&mock_cd_context.type_ctx[CRASH_DUMP_TYPE_FULL]->crash_dump_ctx, 0x1234, mock_ap_wdt_client_ctx, payload_size);
}

TEST_FUNCTION(test_hm_ap_wdt_isr_cd_not_completed, nullptr, nullptr)
{
    test_ap_wdt_occurred = false;
    g_expected_chunk_payload_size = 0;

    memset(&mock_crash_dump_ctx, 0, sizeof(mock_crash_dump_ctx));
    memset(&mock_type_ctx, 0, sizeof(mock_type_ctx));
    memset(&mock_cd_context, 0, sizeof(mock_cd_context));
    mock_crash_dump_ctx.memAPIs.FPFwCDMemcpyLocalToGlobal = mock_FPFwCDMemcpyLocalToGlobal;
    mock_type_ctx.crash_dump_ctx = mock_crash_dump_ctx;
    mock_cd_context.type_ctx[CRASH_DUMP_TYPE_FULL] = &mock_type_ctx;

    memset(mock_crashdump_buffer, 0, sizeof(mock_crashdump_buffer));
    kng_ap_fw_hdr* fw_hdr = (kng_ap_fw_hdr*)mock_crashdump_buffer;
    fw_hdr->size = 0x80;
    fw_hdr->cd_status = 0U; // NOT completed

    /* NVIC */
    expect_function_call(__wrap_nvic_get_current_irq);
    will_return(__wrap_nvic_get_current_irq, TEST_IRQ_NUM);

    expect_value(__wrap_nvic_irq_clear_pending, irq_num, TEST_IRQ_NUM);
    expect_function_call(__wrap_nvic_irq_clear_pending);

    /* Warm start */
    expect_value(__wrap_ws_data_put, id, WARM_START_ID_AP_WDT);
    expect_value(__wrap_ws_data_put, size, sizeof(bool));
    expect_function_call(__wrap_ws_data_put);

    /* CPER */
    expect_value(__wrap_hm_submit_cper, error_domain_idx, ACPI_ERROR_DOMAIN_AP_WDT);
    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));
    expect_function_call(__wrap_hm_submit_cper);

    /* Map NS watchdog */
    will_return(__wrap_idsw_get_die_id, 0);
    expect_function_call(__wrap_idsw_get_die_id);

    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, TEST_NS_BASE_ADDR);
    expect_function_call(__wrap_atu_map);

    expect_value(__wrap_mmio_read32, addr, TEST_NS_BASE_ADDR + AP_WATCHDOG_TIMER_CONTROL_REGISTERS_WCS_ADDRESS);
    will_return(__wrap_mmio_read32, TEST_WCS_VALUE);
    expect_function_call(__wrap_mmio_read32);

    /* Map S watchdog */
    will_return(__wrap_idsw_get_die_id, 0);
    expect_function_call(__wrap_idsw_get_die_id);

    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, TEST_S_BASE_ADDR);
    expect_function_call(__wrap_atu_map);

    expect_value(__wrap_mmio_read32, addr, TEST_S_BASE_ADDR + AP_WATCHDOG_TIMER_CONTROL_REGISTERS_WCS_ADDRESS);
    will_return(__wrap_mmio_read32, TEST_WCS_VALUE);
    expect_function_call(__wrap_mmio_read32);

    /* crash_dump_register_address32 ×2 */
    expect_value(__wrap_crash_dump_register_address32, size, sizeof(ap_watchdog_timer_control_registers_wcs));
    expect_value(__wrap_crash_dump_register_address32, priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    expect_function_call(__wrap_crash_dump_register_address32);

    expect_value(__wrap_crash_dump_register_address32, size, sizeof(ap_watchdog_timer_control_registers_wcs));
    expect_value(__wrap_crash_dump_register_address32, priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    expect_function_call(__wrap_crash_dump_register_address32);

    /* Unmap NS + S */
    expect_value(__wrap_atu_unmap, atu_id, ATU_ID_MSCP);
    expect_function_call(__wrap_atu_unmap);
    expect_value(__wrap_atu_unmap, atu_id, ATU_ID_MSCP);
    expect_function_call(__wrap_atu_unmap);

    /* Map AP FW crashdump header */
    will_return(__wrap_idsw_get_die_id, 0);
    expect_function_call(__wrap_idsw_get_die_id);

    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, (uint32_t)mock_crashdump_buffer);
    expect_function_call(__wrap_atu_map);

    /* crash_dump_context */
    expect_function_call(__wrap_crash_dump_context);

    /* CdRegisterCustomChunk IS still called even if cd_status != COMPLETED */
    expect_function_call(__wrap_CdRegisterCustomChunk);

    /* Bug check still happens */
    expect_function_call(__wrap_crash_dump_bug_check);

    /* Unmap crashdump */
    expect_value(__wrap_atu_unmap, atu_id, ATU_ID_MSCP);
    expect_function_call(__wrap_atu_unmap);

    hm_ap_wdt_isr();

    assert_true(test_ap_wdt_occurred);
}

TEST_FUNCTION(test_hm_ap_wdt_isr_nvic_failure, nullptr, nullptr)
{

    /*
     * NOTE:
     * In hm_ap_wdt_isr(), NVIC status only gates nvic_irq_clear_pending().
     * ws_data_put(), CPER submission, watchdog register reads, crash_dump_register_address32,
     * and unmap are unconditional. The crashdump/BUG_CHECK path is only executed when die_id == 0.
     * To keep this test minimal and deterministic, force die_id != 0 so BUG_CHECK is skipped.
     */

    /* NVIC: return an IRQ number; wrapper returns SUCCESS so clear_pending will be called */
    g_nvic_get_current_irq_status = NVIC_STATUS_SUCCESS;
    expect_function_call(__wrap_nvic_get_current_irq);
    will_return(__wrap_nvic_get_current_irq, 0); // irq value unused

    expect_value(__wrap_nvic_irq_clear_pending, irq_num, 0);
    expect_function_call(__wrap_nvic_irq_clear_pending);

    /* Warm start: unconditional */
    expect_value(__wrap_ws_data_put, id, WARM_START_ID_AP_WDT);
    expect_value(__wrap_ws_data_put, size, sizeof(bool));
    expect_function_call(__wrap_ws_data_put);

    /* CPER: unconditional */
    expect_value(__wrap_hm_submit_cper, error_domain_idx, ACPI_ERROR_DOMAIN_AP_WDT);
    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));
    expect_function_call(__wrap_hm_submit_cper);

    /* Map NS watchdog: idsw_get_die_id then atu_map + mmio_read32 */
    will_return(__wrap_idsw_get_die_id, 1); /* die_id != 0 to skip crashdump/BUG_CHECK later */
    expect_function_call(__wrap_idsw_get_die_id);

    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, TEST_NS_BASE_ADDR);
    expect_function_call(__wrap_atu_map);

    expect_value(__wrap_mmio_read32, addr, TEST_NS_BASE_ADDR + AP_WATCHDOG_TIMER_CONTROL_REGISTERS_WCS_ADDRESS);
    will_return(__wrap_mmio_read32, TEST_WCS_VALUE);
    expect_function_call(__wrap_mmio_read32);

    /* Map S watchdog: idsw_get_die_id then atu_map + mmio_read32 */
    will_return(__wrap_idsw_get_die_id, 1);
    expect_function_call(__wrap_idsw_get_die_id);

    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, TEST_S_BASE_ADDR);
    expect_function_call(__wrap_atu_map);

    expect_value(__wrap_mmio_read32, addr, TEST_S_BASE_ADDR + AP_WATCHDOG_TIMER_CONTROL_REGISTERS_WCS_ADDRESS);
    will_return(__wrap_mmio_read32, TEST_WCS_VALUE);
    expect_function_call(__wrap_mmio_read32);

    /* crash_dump_register_address32 x2: unconditional */
    expect_value(__wrap_crash_dump_register_address32, size, sizeof(ap_watchdog_timer_control_registers_wcs));
    expect_value(__wrap_crash_dump_register_address32, priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    expect_function_call(__wrap_crash_dump_register_address32);

    expect_value(__wrap_crash_dump_register_address32, size, sizeof(ap_watchdog_timer_control_registers_wcs));
    expect_value(__wrap_crash_dump_register_address32, priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    expect_function_call(__wrap_crash_dump_register_address32);

    /* Unmap NS + S: unconditional */
    expect_value(__wrap_atu_unmap, atu_id, ATU_ID_MSCP);
    expect_function_call(__wrap_atu_unmap);
    expect_value(__wrap_atu_unmap, atu_id, ATU_ID_MSCP);
    expect_function_call(__wrap_atu_unmap);

    /* Final idsw_get_die_id inside hm_ap_wdt_isr(): keep die_id != 0 so crashdump/BUG_CHECK is skipped */
    will_return(__wrap_idsw_get_die_id, 1);
    expect_function_call(__wrap_idsw_get_die_id);

    hm_ap_wdt_isr();
}

/*
 * NEW: Cover nvic_status != NVIC_STATUS_SUCCESS branch.
 * Only nvic_irq_clear_pending is gated; the rest of hm_ap_wdt_isr() is unconditional.
 * Force die_id != 0 to avoid the crashdump/BUG_CHECK block and keep expectations minimal.
 */
TEST_FUNCTION(test_hm_ap_wdt_isr_nvic_error_no_clear_pending, nullptr, nullptr)
{
    g_nvic_get_current_irq_status = NVIC_STATUS_ERROR;
    test_ap_wdt_occurred = false;

    /* NVIC called, but clear_pending must NOT be called */
    expect_function_call(__wrap_nvic_get_current_irq);
    will_return(__wrap_nvic_get_current_irq, TEST_IRQ_NUM);

    /* Warm start is unconditional */
    expect_value(__wrap_ws_data_put, id, WARM_START_ID_AP_WDT);
    expect_value(__wrap_ws_data_put, size, sizeof(bool));
    expect_function_call(__wrap_ws_data_put);

    /* CPER is unconditional */
    expect_value(__wrap_hm_submit_cper, error_domain_idx, ACPI_ERROR_DOMAIN_AP_WDT);
    expect_value(__wrap_hm_submit_cper, err_severity, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
    expect_value(__wrap_hm_submit_cper, err_record_section_size, sizeof(acpi_cper_section_t));
    expect_function_call(__wrap_hm_submit_cper);

    /* Map NS watchdog */
    will_return(__wrap_idsw_get_die_id, 1);
    expect_function_call(__wrap_idsw_get_die_id);
    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, TEST_NS_BASE_ADDR);
    expect_function_call(__wrap_atu_map);
    expect_value(__wrap_mmio_read32, addr, TEST_NS_BASE_ADDR + AP_WATCHDOG_TIMER_CONTROL_REGISTERS_WCS_ADDRESS);
    will_return(__wrap_mmio_read32, TEST_WCS_VALUE);
    expect_function_call(__wrap_mmio_read32);

    /* Map S watchdog */
    will_return(__wrap_idsw_get_die_id, 1);
    expect_function_call(__wrap_idsw_get_die_id);
    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, TEST_S_BASE_ADDR);
    expect_function_call(__wrap_atu_map);
    expect_value(__wrap_mmio_read32, addr, TEST_S_BASE_ADDR + AP_WATCHDOG_TIMER_CONTROL_REGISTERS_WCS_ADDRESS);
    will_return(__wrap_mmio_read32, TEST_WCS_VALUE);
    expect_function_call(__wrap_mmio_read32);

    /* crash_dump_register_address32 x2 */
    expect_value(__wrap_crash_dump_register_address32, size, sizeof(ap_watchdog_timer_control_registers_wcs));
    expect_value(__wrap_crash_dump_register_address32, priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    expect_function_call(__wrap_crash_dump_register_address32);
    expect_value(__wrap_crash_dump_register_address32, size, sizeof(ap_watchdog_timer_control_registers_wcs));
    expect_value(__wrap_crash_dump_register_address32, priority, FPFW_CD_DUMP_PRIORITY_CRITICAL);
    expect_function_call(__wrap_crash_dump_register_address32);

    /* Unmap NS + S */
    expect_value(__wrap_atu_unmap, atu_id, ATU_ID_MSCP);
    expect_function_call(__wrap_atu_unmap);
    expect_value(__wrap_atu_unmap, atu_id, ATU_ID_MSCP);
    expect_function_call(__wrap_atu_unmap);

    /* Final die_id check (skip crashdump/BUG_CHECK) */
    will_return(__wrap_idsw_get_die_id, 1);
    expect_function_call(__wrap_idsw_get_die_id);

    hm_ap_wdt_isr();
    assert_true(test_ap_wdt_occurred);
}

TEST_FUNCTION(test_register_ap_wdt_error_domain_cold_boot, nullptr, nullptr)
{
    expect_function_call(__wrap_DfwkAsyncRequestInitialize);
    expect_value(__wrap_ws_data_get, id, WARM_START_ID_AP_WDT);
    will_return(__wrap_ws_data_get, NULL); // No previous AP WDT occurrence
    expect_function_call(__wrap_ws_data_get);
    expect_function_call(__wrap_hm_register_error_domain);
    expect_value(__wrap_FPFwCoreInterruptRegisterCallback, irqnum, HW_INT_AP_NS_WDOG_WS1_INT);
    expect_function_call(__wrap_FPFwCoreInterruptRegisterCallback);
    expect_value(__wrap_FPFwCoreInterruptEnableVector, irqnum, HW_INT_AP_NS_WDOG_WS1_INT);
    expect_function_call(__wrap_FPFwCoreInterruptEnableVector);
    expect_value(__wrap_FPFwCoreInterruptRegisterCallback, irqnum, HW_INT_AP_S_WDOG_WS0_INT);
    expect_function_call(__wrap_FPFwCoreInterruptRegisterCallback);
    expect_value(__wrap_FPFwCoreInterruptEnableVector, irqnum, HW_INT_AP_S_WDOG_WS0_INT);
    expect_function_call(__wrap_FPFwCoreInterruptEnableVector);

    register_ap_wdt_error_domain();
}

TEST_FUNCTION(test_register_ap_wdt_error_domain_warm_boot_prev_ap_wdt, nullptr, nullptr)
{
    test_ap_wdt_occurred = true; // Simulate previous AP WDT occurrence

    expect_function_call(__wrap_DfwkAsyncRequestInitialize);
    expect_value(__wrap_ws_data_get, id, WARM_START_ID_AP_WDT);
    will_return(__wrap_ws_data_get, &test_ap_wdt_occurred); // Previous AP WDT occurrence
    expect_function_call(__wrap_ws_data_get);

    register_ap_wdt_error_domain();
}