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
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <ap_watchdog_timer_control_registers_regs.h>
#include <atu_api.h>
#include <atu_lib.h> // for atu_map_entry_t
#include <crash_dump.h>
#include <error_domain_ap_wdt.h>
#include <health_monitor.h>
#include <idsw_kng.h> // for KNG_DIE_ID, idsw_get_die_id
#include <mscp_error_domain.h>
#include <nvic.h> // for nvic_status_t
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

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static bool test_ap_wdt_occurred = false;

/*------------- Functions ----------------*/
//
// Mocks
//

nvic_status_t __wrap_nvic_get_current_irq(uint32_t* irq_num)
{
    assert_non_null(irq_num);
    *irq_num = mock_type(uint32_t);
    function_called();

    return NVIC_STATUS_SUCCESS;
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
}
/*------------- Test Functions -----------*/

TEST_FUNCTION(test_hm_ap_wdt_isr_success, nullptr, nullptr)
{
    test_ap_wdt_occurred = false;

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

    expect_function_call(__wrap_crash_dump_bug_check);

    // Call the ISR
    hm_ap_wdt_isr();

    // Verify warm start data was set
    assert_true(test_ap_wdt_occurred);
}
