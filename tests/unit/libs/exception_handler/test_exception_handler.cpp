//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_exception_handler.cpp
 * exception handler tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, Cmock...

extern "C" {
#include <cper.h>
#include <crash_dump.h>
#include <exception_handler.h>   // for exception_handler, threadx_stack_error_handler
#include <exception_handler_i.h> // for exception_stack_frame_t
#include <kng_error.h>           // for KNG_CD_HARDFAULT_EXCEPTION
#include <mscp_error_domain.h>
#include <nvic.h> // for nvic_status_t, nvic_set_isr_fault
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#include <scp_top_regs.h>
#include <shared_sram_ecc_ras_registers_regs.h>
#include <stdint.h> // for uint32_t, uintptr_t
#include <tx_api.h> // for TX_SUCCESS, TX_THREAD

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/** Typical EXC_RETURN value: thread mode, MSP, no FP context (bit 4 set) */
#define TEST_EXC_RETURN 0xFFFFFFF9U

/*------------- Functions ----------------*/
//
// Mocks
//
void crash_dump_wait_forever()
{
    function_called();
}

void __wrap_wdog_cmsdk_apb_lock_unlock(bool lock)
{
    check_expected(lock);
    function_called();
}

void __wrap_wdog_cmsdk_apb_disable()
{
    function_called();
}

int get_active_exception(void)
{
    function_called();
    return mock_type(int);
}

void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    check_expected(errorCode);
    check_expected(p1);
    check_expected(p2);
    check_expected(p3);
    check_expected(p4);
    function_called();
}

void __wrap_crash_dump_handler(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    check_expected(errorCode);
    check_expected(p1);
    check_expected(p2);
    check_expected(p3);
    check_expected(p4);
    function_called();
}

bool __wrap_crash_dump_bug_check_initiated_dump()
{
    function_called();
    return mock_type(bool);
}

nvic_status_t __wrap_nvic_set_isr_fault(isr_callback_fn_sans_params_t isr)
{
    check_expected_ptr(isr);
    function_called();

    return mock_type(nvic_status_t);
}

UINT __wrap__tx_thread_stack_error_notify(VOID (*stack_error_handler)(TX_THREAD* thread_ptr))
{
    check_expected_ptr(stack_error_handler);
    function_called();

    return mock_type(UINT);
}

void __wrap_hm_submit_cper_cd_state(uint16_t error_domain_idx,
                                    acpi_error_severity_t err_severity,
                                    void* err_record_section,
                                    uint32_t err_record_section_size)
{
    assert_true(error_domain_idx == ACPI_ERROR_DOMAIN_SCP_PROC || error_domain_idx == ACPI_ERROR_DOMAIN_MCP_PROC);
    check_expected(err_severity);
    assert_true(err_record_section != NULL);
    assert_true(err_record_section_size > 0);
    function_called();
}

static uint8_t mapped_region[0x20000] = {0};

int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    assert_true(atu_id == ATU_ID_MSCP);
    assert_non_null(atu_map_entry);

    atu_map_entry->mscp_start_address = (uint32_t)mapped_region;

    function_called();

    return 0;
}

int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    assert_true(atu_id == ATU_ID_MSCP);
    assert_non_null(atu_map_entry);

    function_called();

    return 0;
}

int __wrap_atu_find_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    assert_true(atu_id == ATU_ID_MSCP);
    assert_non_null(atu_map_entry);

    atu_map_entry->ap_base_address = mock_type(uint64_t);

    return mock_type(int);
}

void __wrap_get_arsm_ecc_atu_entry_die(mscp_arsm_ram_type_t type, uint8_t die_id, atu_map_entry_t* atu_entry)
{
    FPFW_UNUSED(type);
    check_expected(die_id);
    static atu_map_entry_t dummy_entry = {};
    assert_non_null(atu_entry);
    *atu_entry = dummy_entry; // Return a dummy entry for testing purposes

    function_called();
}

void __wrap_get_rsm_ecc_atu_entry_die(mscp_arsm_ram_type_t type, uint8_t die_id, atu_map_entry_t* atu_entry)
{
    FPFW_UNUSED(type);
    check_expected(die_id);
    static atu_map_entry_t dummy_entry = {};
    assert_non_null(atu_entry);
    *atu_entry = dummy_entry; // Return a dummy entry for testing purposes

    function_called();
}

uint32_t __wrap_mmio_read32(volatile uint32_t* addr)
{
    check_expected(addr);
    function_called();
    return mock_type(uint32_t);
}

void __wrap_mmio_write32(volatile uint32_t* addr, uint32_t data)
{
    check_expected(addr);
    check_expected(data);
    function_called();
}

void __wrap_mmio_update32(volatile uint32_t* addr, uint32_t data, uint32_t mask)
{
    check_expected(addr);
    check_expected(data);
    check_expected(mask);
    function_called();
}
//
// Tests
//
void test_exception_handler_params(int exception, uint32_t error_code)
{
    exception_stack_frame_t stack_frame = {1, 2, 3, 4, 5, 6, 7, 8};

    // Set up expectations
    expect_function_call(__wrap_wdog_cmsdk_apb_disable);
    expect_value(__wrap_wdog_cmsdk_apb_lock_unlock, lock, true);
    expect_function_call(__wrap_wdog_cmsdk_apb_lock_unlock);

    expect_function_call(get_active_exception);
    will_return(get_active_exception, exception);

    expect_value(__wrap_hm_submit_cper_cd_state, err_severity, ACPI_ERROR_SEVERITY_CORRECTED);
    expect_function_call(__wrap_hm_submit_cper_cd_state);

    expect_value(__wrap_crash_dump_handler, errorCode, error_code);
    expect_any(__wrap_crash_dump_handler, p1); // __FILE__
    expect_any(__wrap_crash_dump_handler, p2); // __LINE__
    expect_value(__wrap_crash_dump_handler, p3, 0);
    expect_value(__wrap_crash_dump_handler, p4, 0);
    expect_function_call(__wrap_crash_dump_handler);

    expect_function_call(crash_dump_wait_forever);

    // Call API
    exception_handler(&stack_frame, TEST_EXC_RETURN);
}

TEST_FUNCTION(test_exception_handler_init, NULL, NULL)
{
    // Set up expectations
    expect_any(__wrap_nvic_set_isr_fault, isr);
    expect_function_call(__wrap_nvic_set_isr_fault); // expect general exception handler to be set
    will_return(__wrap_nvic_set_isr_fault, NVIC_STATUS_SUCCESS);

    expect_any(NVIC_SetVector, isr);
    expect_function_call(NVIC_SetVector); // expect DebugMonitor_IRQn handler to be set

    expect_any(NVIC_SetVector, isr);
    expect_function_call(NVIC_SetVector); // expect NonMaskableInt_IRQn handler to be set

    expect_any(__wrap__tx_thread_stack_error_notify, stack_error_handler);
    expect_function_call(__wrap__tx_thread_stack_error_notify); // expect stack error handler to be set
    will_return(__wrap__tx_thread_stack_error_notify, TX_SUCCESS);

    // Call API under test
    int32_t result = exception_handler_init();
    assert_true(result == KNG_SUCCESS);
}

TEST_FUNCTION(test_exception_handler, nullptr, nullptr)
{
    test_exception_handler_params(-13, KNG_CD_HARDFAULT_EXCEPTION);        // HardFault_IRQn
    test_exception_handler_params(-12, KNG_CD_MEMORYMANAGEMENT_EXCEPTION); // MemoryManagement_IRQn
    test_exception_handler_params(-11, KNG_CD_BUSFAULT_EXCEPTION);         // BusFault_IRQn
    test_exception_handler_params(-10, KNG_CD_USAGEFAULT_EXCEPTION);       // UsageFault_IRQn
    test_exception_handler_params(-14, KNG_CD_WDT_TIMEOUT);                // NonMaskableInt_IRQn
    test_exception_handler_params(0, KNG_CD_DEFAULT_EXCEPTION);            // Default
}

TEST_FUNCTION(test_exception_handler_bug_check, nullptr, nullptr)
{
    exception_stack_frame_t stack_frame;

    stack_frame.R0 = KNG_E_NOTIMPL; // error_code
    stack_frame.R1 = 1;             // p1
    stack_frame.R2 = 2;             // p2
    stack_frame.R3 = 3;             // p3
    stack_frame.R12 = 12;
    stack_frame.LR = 14;
    stack_frame.PC = 15;
    stack_frame.PSR = 16;
    g_core_crash_context.r4 = 4; // p4

    // Set up expectations
    expect_function_call(__wrap_wdog_cmsdk_apb_disable);
    expect_value(__wrap_wdog_cmsdk_apb_lock_unlock, lock, true);
    expect_function_call(__wrap_wdog_cmsdk_apb_lock_unlock);

    expect_function_call(get_active_exception);
    will_return(get_active_exception, -4); // DebugMonitor_IRQn

    expect_function_call(__wrap_crash_dump_bug_check_initiated_dump);
    will_return(__wrap_crash_dump_bug_check_initiated_dump, true);

    expect_value(__wrap_hm_submit_cper_cd_state, err_severity, ACPI_ERROR_SEVERITY_CORRECTED);
    expect_function_call(__wrap_hm_submit_cper_cd_state);

    expect_value(__wrap_crash_dump_handler, errorCode, (uint32_t)KNG_E_NOTIMPL);
    expect_value(__wrap_crash_dump_handler, p1, 1);
    expect_value(__wrap_crash_dump_handler, p2, 2);
    expect_value(__wrap_crash_dump_handler, p3, 3);
    expect_value(__wrap_crash_dump_handler, p4, 4);
    expect_function_call(__wrap_crash_dump_handler);

    expect_function_call(crash_dump_wait_forever);

    // Call API
    exception_handler(&stack_frame, TEST_EXC_RETURN);

    // Set up expectations
    expect_function_call(__wrap_wdog_cmsdk_apb_disable);
    expect_value(__wrap_wdog_cmsdk_apb_lock_unlock, lock, true);
    expect_function_call(__wrap_wdog_cmsdk_apb_lock_unlock);

    expect_function_call(get_active_exception);
    will_return(get_active_exception, -4); // DebugMonitor_IRQn

    expect_function_call(__wrap_crash_dump_bug_check_initiated_dump);
    will_return(__wrap_crash_dump_bug_check_initiated_dump, false);

    expect_value(__wrap_crash_dump_handler, errorCode, 0);
    expect_value(__wrap_crash_dump_handler, p1, 0);
    expect_value(__wrap_crash_dump_handler, p2, 0);
    expect_value(__wrap_crash_dump_handler, p3, 0);
    expect_value(__wrap_crash_dump_handler, p4, 0);
    expect_function_call(__wrap_crash_dump_handler);

    expect_function_call(crash_dump_wait_forever);

    // Call API
    exception_handler(&stack_frame, TEST_EXC_RETURN);
}

TEST_FUNCTION(test_bus_fault_handler_d0_arsm_UE, nullptr, nullptr)
{
    exception_stack_frame_t stack_frame;

    stack_frame.R0 = KNG_E_NOTIMPL; // error_code
    stack_frame.R1 = 1;             // p1
    stack_frame.R2 = 2;             // p2
    stack_frame.R3 = 3;             // p3
    stack_frame.R12 = 12;
    stack_frame.LR = 14;
    stack_frame.PC = 15;
    stack_frame.PSR = 16;
    g_core_crash_context.r4 = 4; // p4

    // Set up expectations
    expect_function_call(__wrap_wdog_cmsdk_apb_disable);
    expect_value(__wrap_wdog_cmsdk_apb_lock_unlock, lock, true);
    expect_function_call(__wrap_wdog_cmsdk_apb_lock_unlock);

    expect_function_call(get_active_exception);
    will_return(get_active_exception, -11); // BusFault_IRQn

    // Set up expectations for shared ram UE
    SCB->CFSR = SCB_CFSR_MMARVALID_Msk;
    SCB->MMFAR = 0x12345678;
    will_return(__wrap_atu_find_map, 0); // AP address (AP_TOP_D0_ARSM_SHARED_SRAM_ADDRESS)
    will_return(__wrap_atu_find_map, 0);

    expect_value(__wrap_get_arsm_ecc_atu_entry_die, die_id, 0);
    expect_function_call(__wrap_get_arsm_ecc_atu_entry_die);
    expect_function_call(__wrap_atu_map);
    expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ADDRESS);
    will_return(__wrap_mmio_read32,
                SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK |
                    SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ER_MASK |
                    SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_MASK);
    expect_function_call(__wrap_mmio_read32);
    expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRADDR_ADDRESS);
    will_return(__wrap_mmio_read32, 0x12345678);
    expect_function_call(__wrap_mmio_read32);

    expect_value(__wrap_mmio_write32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ADDRESS);
    expect_value(__wrap_mmio_write32,
                 data,
                 (SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK |
                  SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ER_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_MASK |
                  SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_SERR_MASK));
    expect_function_call(__wrap_mmio_write32);
    expect_function_call(__wrap_atu_unmap);

    expect_value(__wrap_hm_submit_cper_cd_state, err_severity, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
    expect_function_call(__wrap_hm_submit_cper_cd_state);

    expect_value(__wrap_crash_dump_handler, errorCode, (uint32_t)KNG_HM_ARSM_UE);
    expect_any(__wrap_crash_dump_handler, p1);
    expect_any(__wrap_crash_dump_handler, p2);
    expect_any(__wrap_crash_dump_handler, p3);
    expect_any(__wrap_crash_dump_handler, p4);
    expect_function_call(__wrap_crash_dump_handler);

    expect_function_call(crash_dump_wait_forever);

    // Call API
    exception_handler(&stack_frame, TEST_EXC_RETURN);
}

TEST_FUNCTION(test_bus_fault_handler_d1_arsm_UE, nullptr, nullptr)
{
    exception_stack_frame_t stack_frame;

    stack_frame.R0 = KNG_E_NOTIMPL; // error_code
    stack_frame.R1 = 1;             // p1
    stack_frame.R2 = 2;             // p2
    stack_frame.R3 = 3;             // p3
    stack_frame.R12 = 12;
    stack_frame.LR = 14;
    stack_frame.PC = 15;
    stack_frame.PSR = 16;
    g_core_crash_context.r4 = 4; // p4

    // Set up expectations
    expect_function_call(__wrap_wdog_cmsdk_apb_disable);
    expect_value(__wrap_wdog_cmsdk_apb_lock_unlock, lock, true);
    expect_function_call(__wrap_wdog_cmsdk_apb_lock_unlock);

    expect_function_call(get_active_exception);
    will_return(get_active_exception, -11); // BusFault_IRQn

    // Set up expectations for shared ram UE
    SCB->CFSR = SCB_CFSR_MMARVALID_Msk;
    SCB->MMFAR = 0x12345678;
    will_return(__wrap_atu_find_map, AP_TOP_D1_ARSM_SHARED_SRAM_ADDRESS); // AP address
    will_return(__wrap_atu_find_map, 0);

    expect_value(__wrap_get_arsm_ecc_atu_entry_die, die_id, 1);
    expect_function_call(__wrap_get_arsm_ecc_atu_entry_die);
    expect_function_call(__wrap_atu_map);
    expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ADDRESS);
    will_return(__wrap_mmio_read32,
                SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK |
                    SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ER_MASK |
                    SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_MASK);
    expect_function_call(__wrap_mmio_read32);
    expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRADDR_ADDRESS);
    will_return(__wrap_mmio_read32, 0x12345678);
    expect_function_call(__wrap_mmio_read32);

    expect_value(__wrap_mmio_write32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ADDRESS);
    expect_value(__wrap_mmio_write32,
                 data,
                 (SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK |
                  SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ER_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_MASK |
                  SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_SERR_MASK));
    expect_function_call(__wrap_mmio_write32);
    expect_function_call(__wrap_atu_unmap);

    expect_value(__wrap_hm_submit_cper_cd_state, err_severity, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
    expect_function_call(__wrap_hm_submit_cper_cd_state);

    expect_value(__wrap_crash_dump_handler, errorCode, (uint32_t)KNG_HM_ARSM_UE);
    expect_any(__wrap_crash_dump_handler, p1);
    expect_any(__wrap_crash_dump_handler, p2);
    expect_any(__wrap_crash_dump_handler, p3);
    expect_any(__wrap_crash_dump_handler, p4);
    expect_function_call(__wrap_crash_dump_handler);

    expect_function_call(crash_dump_wait_forever);

    // Call API
    exception_handler(&stack_frame, TEST_EXC_RETURN);
}

TEST_FUNCTION(test_bus_fault_handler_d0_rsm_UE, nullptr, nullptr)
{
    exception_stack_frame_t stack_frame;

    stack_frame.R0 = KNG_E_NOTIMPL; // error_code
    stack_frame.R1 = 1;             // p1
    stack_frame.R2 = 2;             // p2
    stack_frame.R3 = 3;             // p3
    stack_frame.R12 = 12;
    stack_frame.LR = 14;
    stack_frame.PC = 15;
    stack_frame.PSR = 16;
    g_core_crash_context.r4 = 4; // p4

    // Set up expectations
    expect_function_call(__wrap_wdog_cmsdk_apb_disable);
    expect_value(__wrap_wdog_cmsdk_apb_lock_unlock, lock, true);
    expect_function_call(__wrap_wdog_cmsdk_apb_lock_unlock);

    expect_function_call(get_active_exception);
    will_return(get_active_exception, -11); // BusFault_IRQn

    // Set up expectations for shared ram UE
    SCB->CFSR = SCB_CFSR_MMARVALID_Msk;
    SCB->MMFAR = 0x12345678;
    will_return(__wrap_atu_find_map, 0x2F000000UL); // AP address
    will_return(__wrap_atu_find_map, 0);

    expect_value(__wrap_get_rsm_ecc_atu_entry_die, die_id, 0);
    expect_function_call(__wrap_get_rsm_ecc_atu_entry_die);
    expect_function_call(__wrap_atu_map);
    expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ADDRESS);
    will_return(__wrap_mmio_read32,
                SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK |
                    SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ER_MASK |
                    SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_MASK);
    expect_function_call(__wrap_mmio_read32);
    expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRADDR_ADDRESS);
    will_return(__wrap_mmio_read32, 0x12345678);
    expect_function_call(__wrap_mmio_read32);

    expect_value(__wrap_mmio_write32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ADDRESS);
    expect_value(__wrap_mmio_write32,
                 data,
                 (SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK |
                  SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ER_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_MASK |
                  SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_SERR_MASK));
    expect_function_call(__wrap_mmio_write32);
    expect_function_call(__wrap_atu_unmap);

    expect_value(__wrap_hm_submit_cper_cd_state, err_severity, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
    expect_function_call(__wrap_hm_submit_cper_cd_state);

    expect_value(__wrap_crash_dump_handler, errorCode, (uint32_t)KNG_HM_RSM_UE);
    expect_any(__wrap_crash_dump_handler, p1);
    expect_any(__wrap_crash_dump_handler, p2);
    expect_any(__wrap_crash_dump_handler, p3);
    expect_any(__wrap_crash_dump_handler, p4);
    expect_function_call(__wrap_crash_dump_handler);

    expect_function_call(crash_dump_wait_forever);

    // Call API
    exception_handler(&stack_frame, TEST_EXC_RETURN);
}

TEST_FUNCTION(test_bus_fault_handler_d1_rsm_UE, nullptr, nullptr)
{
    exception_stack_frame_t stack_frame;

    stack_frame.R0 = KNG_E_NOTIMPL; // error_code
    stack_frame.R1 = 1;             // p1
    stack_frame.R2 = 2;             // p2
    stack_frame.R3 = 3;             // p3
    stack_frame.R12 = 12;
    stack_frame.LR = 14;
    stack_frame.PC = 15;
    stack_frame.PSR = 16;
    g_core_crash_context.r4 = 4; // p4

    // Set up expectations
    expect_function_call(__wrap_wdog_cmsdk_apb_disable);
    expect_value(__wrap_wdog_cmsdk_apb_lock_unlock, lock, true);
    expect_function_call(__wrap_wdog_cmsdk_apb_lock_unlock);

    expect_function_call(get_active_exception);
    will_return(get_active_exception, -11); // BusFault_IRQn

    // Set up expectations for shared ram UE
    SCB->CFSR = SCB_CFSR_MMARVALID_Msk;
    SCB->MMFAR = 0x12345678;
    will_return(__wrap_atu_find_map, 0x102F000000UL); // AP address
    will_return(__wrap_atu_find_map, 0);

    expect_value(__wrap_get_rsm_ecc_atu_entry_die, die_id, 1);
    expect_function_call(__wrap_get_rsm_ecc_atu_entry_die);
    expect_function_call(__wrap_atu_map);
    expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ADDRESS);
    will_return(__wrap_mmio_read32,
                SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK |
                    SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ER_MASK |
                    SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_MASK);
    expect_function_call(__wrap_mmio_read32);
    expect_value(__wrap_mmio_read32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRADDR_ADDRESS);
    will_return(__wrap_mmio_read32, 0x12345678);
    expect_function_call(__wrap_mmio_read32);

    expect_value(__wrap_mmio_write32, addr, (uint32_t)mapped_region + SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ADDRESS);
    expect_value(__wrap_mmio_write32,
                 data,
                 (SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_V_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_UE_MASK |
                  SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_ER_MASK | SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_AV_MASK |
                  SHARED_SRAM_ECC_RAS_REGISTERS_SRAMECC_ERRSTATUS_SERR_MASK));
    expect_function_call(__wrap_mmio_write32);
    expect_function_call(__wrap_atu_unmap);

    expect_value(__wrap_hm_submit_cper_cd_state, err_severity, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
    expect_function_call(__wrap_hm_submit_cper_cd_state);

    expect_value(__wrap_crash_dump_handler, errorCode, (uint32_t)KNG_HM_RSM_UE);
    expect_any(__wrap_crash_dump_handler, p1);
    expect_any(__wrap_crash_dump_handler, p2);
    expect_any(__wrap_crash_dump_handler, p3);
    expect_any(__wrap_crash_dump_handler, p4);
    expect_function_call(__wrap_crash_dump_handler);

    expect_function_call(crash_dump_wait_forever);

    // Call API
    exception_handler(&stack_frame, TEST_EXC_RETURN);
}

TEST_FUNCTION(test_hard_fault_handler_scf_ram_UE, nullptr, nullptr)
{
    exception_stack_frame_t stack_frame;

    stack_frame.R0 = KNG_E_NOTIMPL; // error_code
    stack_frame.R1 = 1;             // p1
    stack_frame.R2 = 2;             // p2
    stack_frame.R3 = 3;             // p3
    stack_frame.R12 = 12;
    stack_frame.LR = 14;
    stack_frame.PC = 15;
    stack_frame.PSR = 16;
    g_core_crash_context.r4 = 4; // p4

    // Set up expectations
    expect_function_call(__wrap_wdog_cmsdk_apb_disable);
    expect_value(__wrap_wdog_cmsdk_apb_lock_unlock, lock, true);
    expect_function_call(__wrap_wdog_cmsdk_apb_lock_unlock);

    expect_function_call(get_active_exception);
    will_return(get_active_exception, -13); // HardFault_IRQn

    // Set up expectations for scf ram UE
    SCB->CFSR = SCB_CFSR_BFARVALID_Msk;
    SCB->BFAR = SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_RAM_ADDRESS;
    will_return(__wrap_atu_find_map, 0);  // AP address
    will_return(__wrap_atu_find_map, -1); // Not found for SCF
    expect_value(__wrap_mmio_read32, addr, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_ADDRESS);
    will_return(__wrap_mmio_read32, SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_UE_MASK); // Simulate SCF UE
    expect_function_call(__wrap_mmio_read32);

    expect_value(__wrap_mmio_read32, addr, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_SCFRAM_SCP_ERRADDR_REG_ADDRESS);
    will_return(__wrap_mmio_read32, 0x12345678); // Mock Address
    expect_function_call(__wrap_mmio_read32);

    // Clear UE (Update)
    expect_value(__wrap_mmio_update32, addr, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_ADDRESS);
    expect_value(__wrap_mmio_update32, data, SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_UE_MASK);
    expect_value(__wrap_mmio_update32,
                 mask,
                 SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_UE_MASK | SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_CE_MASK |
                     SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_OF_MASK);
    expect_function_call(__wrap_mmio_update32);

    expect_value(__wrap_hm_submit_cper_cd_state, err_severity, ACPI_ERROR_SEVERITY_CORRECTED);
    expect_function_call(__wrap_hm_submit_cper_cd_state);

    expect_value(__wrap_crash_dump_handler, errorCode, (uint32_t)KNG_HM_SCF_UE);
    expect_any(__wrap_crash_dump_handler, p1);
    expect_any(__wrap_crash_dump_handler, p2);
    expect_any(__wrap_crash_dump_handler, p3);
    expect_any(__wrap_crash_dump_handler, p4);
    expect_function_call(__wrap_crash_dump_handler);

    expect_function_call(crash_dump_wait_forever);

    // Call API
    exception_handler(&stack_frame, TEST_EXC_RETURN);
}

TEST_FUNCTION(test_bus_fault_handler_rmss_ram0_UE, nullptr, nullptr)
{
    exception_stack_frame_t stack_frame;

    stack_frame.R0 = KNG_E_NOTIMPL; // error_code
    stack_frame.R1 = 1;             // p1
    stack_frame.R2 = 2;             // p2
    stack_frame.R3 = 3;             // p3
    stack_frame.R12 = 12;
    stack_frame.LR = 14;
    stack_frame.PC = 15;
    stack_frame.PSR = 16;
    g_core_crash_context.r4 = 4; // p4

    // Set up expectations
    expect_function_call(__wrap_wdog_cmsdk_apb_disable);
    expect_value(__wrap_wdog_cmsdk_apb_lock_unlock, lock, true);
    expect_function_call(__wrap_wdog_cmsdk_apb_lock_unlock);

    expect_function_call(get_active_exception);
    will_return(get_active_exception, -11); // BusFault_IRQn

    // Set up expectations for rmss ram0 UE
    SCB->CFSR = SCB_CFSR_BFARVALID_Msk;
    SCB->BFAR = SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS;
    will_return(__wrap_atu_find_map, 0);  // AP address
    will_return(__wrap_atu_find_map, -1); // Not found for RMSS RAM0
    expect_value(__wrap_mmio_read32, addr, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM0_SCP_ERRSTATUS_REG_ADDRESS);
    will_return(__wrap_mmio_read32, SCP_EXP_CSR_RMSS_RAM0_SCP_ERRSTATUS_REG_UE_MASK); // Simulate RMSS RAM0 UE
    expect_function_call(__wrap_mmio_read32);

    expect_value(__wrap_mmio_read32, addr, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM0_SCP_ERRADDR_REG_ADDRESS);
    will_return(__wrap_mmio_read32, 0x12345678); // Mock Address
    expect_function_call(__wrap_mmio_read32);

    // Clear UE (Update)
    expect_value(__wrap_mmio_update32, addr, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM0_SCP_ERRSTATUS_REG_ADDRESS);
    expect_value(__wrap_mmio_update32, data, SCP_EXP_CSR_RMSS_RAM0_SCP_ERRSTATUS_REG_UE_MASK);
    expect_value(__wrap_mmio_update32,
                 mask,
                 SCP_EXP_CSR_RMSS_RAM0_SCP_ERRSTATUS_REG_CE_MASK | SCP_EXP_CSR_RMSS_RAM0_SCP_ERRSTATUS_REG_UE_MASK |
                     SCP_EXP_CSR_RMSS_RAM0_SCP_ERRSTATUS_REG_OF_MASK);
    expect_function_call(__wrap_mmio_update32);

    expect_value(__wrap_hm_submit_cper_cd_state, err_severity, ACPI_ERROR_SEVERITY_CORRECTED);
    expect_function_call(__wrap_hm_submit_cper_cd_state);

    expect_value(__wrap_crash_dump_handler, errorCode, (uint32_t)KNG_HM_RMSS_RAM0_UE);
    expect_any(__wrap_crash_dump_handler, p1);
    expect_any(__wrap_crash_dump_handler, p2);
    expect_any(__wrap_crash_dump_handler, p3);
    expect_any(__wrap_crash_dump_handler, p4);
    expect_function_call(__wrap_crash_dump_handler);

    expect_function_call(crash_dump_wait_forever);

    // Call API
    exception_handler(&stack_frame, TEST_EXC_RETURN);
}

TEST_FUNCTION(test_bus_fault_handler_rmss_ram1_UE, nullptr, nullptr)
{
    exception_stack_frame_t stack_frame;

    stack_frame.R0 = KNG_E_NOTIMPL; // error_code
    stack_frame.R1 = 1;             // p1
    stack_frame.R2 = 2;             // p2
    stack_frame.R3 = 3;             // p3
    stack_frame.R12 = 12;
    stack_frame.LR = 14;
    stack_frame.PC = 15;
    stack_frame.PSR = 16;
    g_core_crash_context.r4 = 4; // p4

    // Set up expectations
    expect_function_call(__wrap_wdog_cmsdk_apb_disable);
    expect_value(__wrap_wdog_cmsdk_apb_lock_unlock, lock, true);
    expect_function_call(__wrap_wdog_cmsdk_apb_lock_unlock);

    expect_function_call(get_active_exception);
    will_return(get_active_exception, -11); // BusFault_IRQn

    // Set up expectations for rmss ram1 UE
    SCB->CFSR = SCB_CFSR_BFARVALID_Msk;
    SCB->BFAR = SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS;
    will_return(__wrap_atu_find_map, 0);  // AP address
    will_return(__wrap_atu_find_map, -1); // Not found for RMSS RAM1
    expect_value(__wrap_mmio_read32, addr, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM1_SCP_ERRSTATUS_REG_ADDRESS);
    will_return(__wrap_mmio_read32, SCP_EXP_CSR_RMSS_RAM1_SCP_ERRSTATUS_REG_UE_MASK); // Simulate RMSS RAM1 UE
    expect_function_call(__wrap_mmio_read32);

    expect_value(__wrap_mmio_read32, addr, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM1_SCP_ERRADDR_REG_ADDRESS);
    will_return(__wrap_mmio_read32, 0x12345678); // Mock Address
    expect_function_call(__wrap_mmio_read32);

    // Clear UE (Update)
    expect_value(__wrap_mmio_update32, addr, SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM1_SCP_ERRSTATUS_REG_ADDRESS);
    expect_value(__wrap_mmio_update32, data, SCP_EXP_CSR_RMSS_RAM1_SCP_ERRSTATUS_REG_UE_MASK);
    expect_value(__wrap_mmio_update32,
                 mask,
                 SCP_EXP_CSR_RMSS_RAM1_SCP_ERRSTATUS_REG_CE_MASK | SCP_EXP_CSR_RMSS_RAM1_SCP_ERRSTATUS_REG_UE_MASK |
                     SCP_EXP_CSR_RMSS_RAM1_SCP_ERRSTATUS_REG_OF_MASK);
    expect_function_call(__wrap_mmio_update32);

    expect_value(__wrap_hm_submit_cper_cd_state, err_severity, ACPI_ERROR_SEVERITY_CORRECTED);
    expect_function_call(__wrap_hm_submit_cper_cd_state);

    expect_value(__wrap_crash_dump_handler, errorCode, (uint32_t)KNG_HM_RMSS_RAM1_UE);
    expect_any(__wrap_crash_dump_handler, p1);
    expect_any(__wrap_crash_dump_handler, p2);
    expect_any(__wrap_crash_dump_handler, p3);
    expect_any(__wrap_crash_dump_handler, p4);
    expect_function_call(__wrap_crash_dump_handler);

    expect_function_call(crash_dump_wait_forever);

    // Call API
    exception_handler(&stack_frame, TEST_EXC_RETURN);
}

TEST_FUNCTION(test_threadx_stack_error_handler, nullptr, nullptr)
{
    TX_THREAD tx_thread;

    // Set up expectations
    expect_value(__wrap_crash_dump_bug_check, errorCode, (uint32_t)KNG_CD_THREAD_STACK_ERROR);
    expect_any(__wrap_crash_dump_bug_check, p1);
    expect_any(__wrap_crash_dump_bug_check, p2);
    expect_value(__wrap_crash_dump_bug_check, p3, &tx_thread);
    expect_value(__wrap_crash_dump_bug_check, p4, 0);
    expect_function_call(__wrap_crash_dump_bug_check);

    // Call API under test
    threadx_stack_error_handler(&tx_thread);
}
}