//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_telem_interrupt_handler.cpp
 * Power service interrupt handler tests
 */

/*------------- Includes -----------------*/
// #include <cstddef> // for NULL
// #include <cstdlib>
// #include <cmath>

#include "power_test.h" // for POWER_TEST

extern "C" {
#include "power_interrupt_handler.h"
#include "power_runconfig_i.h"

#include <CMockaWrapper.h>
#include <FpFwUtils.h>
#include <corebits.h>
#include <interrupts.h>
#include <nvic.h> // for nvic_status_t
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
void __real_core_pll_error_status(uint32_t core_idx, bool is_unlock);
static power_service_config_t sconfig = {};
static power_runconfig_t test_runconfig = {.p_sconfig = &sconfig};

/*------------- Functions ----------------*/
//
// Mocks
//

// --- Provide a raw buffer for the register block ---

uint32_t __wrap_mmio_read32(const volatile uint32_t* addr)
{
    check_expected(addr);
    return mock_type(uint32_t);
}
void __wrap_mmio_write32(volatile uint32_t* addr, uint32_t data)
{
    check_expected(addr);
    check_expected(data);
}
void __wrap_core_pll_error_status(uint32_t core_idx, bool is_unlock)
{
    check_expected(core_idx);
    check_expected(is_unlock);
}
nvic_status_t __wrap_nvic_get_current_irq(uint32_t* irq_num)
{
    assert_non_null(irq_num);
    *irq_num = mock_type(uint32_t);
    function_called();

    return NVIC_STATUS_SUCCESS;
}

power_runconfig_t* __wrap_power_runconfig_get()
{
    return mock_type(power_runconfig_t*);
}

void __wrap_power_loops_control_handle_event(int event, const void* event_data)
{
    FPFW_UNUSED(event);
    FPFW_UNUSED(event_data);
}
} // extern "C"

// end mocks

/*-------- Function Prototypes -----------*/

/*------------- Functions ----------------*/

POWER_TEST(test_pll_isr_lock_status, NULL, NULL)
{
    // Setup: IRQ is HW_INT_CPU_67_64_31_0_PLL_LOCK_INT
    uint32_t irq_num = HW_INT_CPU_67_64_31_0_PLL_LOCK_INT;

    // nvic_get_current_irq
    expect_function_call(__wrap_nvic_get_current_irq);
    will_return(__wrap_nvic_get_current_irq, irq_num);

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, 0x5); // first call

    expect_any(__wrap_mmio_write32, addr);
    expect_any(__wrap_mmio_write32, data);
    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, 0x0); // second call

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, 0x0); // third call

    // core_pll_error_status for core 0 and 2
    expect_value(__wrap_core_pll_error_status, core_idx, 0);
    expect_any(__wrap_core_pll_error_status, is_unlock);
    expect_value(__wrap_core_pll_error_status, core_idx, 2);
    expect_any(__wrap_core_pll_error_status, is_unlock);

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, 0x0);

    // Call the ISR
    pll_isr();
}

POWER_TEST(test_pll_isr_unlock_status, NULL, NULL)
{

    // Setup: IRQ is HW_INT_CPU_67_64_31_0_PLL_UNLOCK_INT
    uint32_t irq_num = HW_INT_CPU_67_64_31_0_PLL_UNLOCK_INT;

    // nvic_get_current_irq
    expect_function_call(__wrap_nvic_get_current_irq);
    will_return(__wrap_nvic_get_current_irq, irq_num);

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, 0x5); // first call

    expect_any(__wrap_mmio_write32, addr);
    expect_any(__wrap_mmio_write32, data);
    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, 0x0); // second call

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, 0x0); // third call

    // core_pll_error_status for core 0 and 2
    expect_value(__wrap_core_pll_error_status, core_idx, 0);
    expect_any(__wrap_core_pll_error_status, is_unlock);
    expect_value(__wrap_core_pll_error_status, core_idx, 2);
    expect_any(__wrap_core_pll_error_status, is_unlock);

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, 0x0);

    // Call the ISR
    pll_isr();
}

POWER_TEST(test_core_pll_error_status, NULL, NULL)
{
    will_return(__wrap_power_runconfig_get, &test_runconfig);

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, 0x1234);

    expect_any(__wrap_mmio_read32, addr);
    will_return(__wrap_mmio_read32, 0x5678);

    // Call the function under test
    __real_core_pll_error_status(0, false);
}
