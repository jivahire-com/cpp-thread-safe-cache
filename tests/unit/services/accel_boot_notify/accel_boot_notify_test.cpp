//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file accel_boot_notify_test.cpp
 * Accel Boot Test
 */
/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep
extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <accel_boot_notify.h>
#include <stddef.h> // for NULL
#include <tx_api.h> // for TX_AUTO_ACTIVATE, TX_SUCCESS

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

VOID (*notify_boot_complete_cb)(ULONG id) = nullptr;
static jmp_buf mock_jump_buf;
static bool should_return = true;

/*------------- Functions ----------------*/
//
// Mocks
//

void __wrap_crash_dump_bug_check(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    FPFW_UNUSED(p0);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);

    function_called();

    /* Handle noreturn, allowing control to return to test */
    if (!should_return)
    {
        longjmp(mock_jump_buf, 1);
    }
}

UINT __wrap__txe_timer_create(TX_TIMER* timer_ptr,
                              CHAR* name_ptr,
                              VOID (*expiration_function)(ULONG id),
                              ULONG expiration_input,
                              ULONG initial_ticks,
                              ULONG reschedule_ticks,
                              UINT auto_activate,
                              UINT timer_control_block_size)
{
    FPFW_UNUSED(timer_ptr);
    FPFW_UNUSED(name_ptr);
    assert_non_null(expiration_function);
    FPFW_UNUSED(expiration_input);
    FPFW_UNUSED(initial_ticks);
    FPFW_UNUSED(reschedule_ticks);
    FPFW_UNUSED(auto_activate);
    FPFW_UNUSED(timer_control_block_size);

    notify_boot_complete_cb = expiration_function;

    function_called();

    return 0;
}

UINT __wrap__txe_timer_deactivate(TX_TIMER* timer_ptr)
{
    FPFW_UNUSED(timer_ptr);

    function_called();

    return 0;
}

int32_t __wrap_notify_accelerators_uefi_boot(void)
{
    function_called();

    return mock_type(int32_t);
}

int __wrap_gpio_get_input(uint32_t gpio_ctrl_pin_id, uint32_t* level)
{
    check_expected(gpio_ctrl_pin_id);
    assert_non_null(level);
    *level = mock_type(uint32_t);

    function_called();

    return 0;
}

} // extern "C"

/****************************
 * TESTS
 ****************************/

/**
 * @brief Tests call paths in accel_boot_notify
 */
TEST_FUNCTION(test_accel_boot_notify, NULL, NULL)
{
    expect_function_call(__wrap__txe_timer_create);

    accel_boot_notify_service();

    expect_function_call(__wrap_gpio_get_input);
    will_return(__wrap_gpio_get_input, true);
    expect_value(__wrap_gpio_get_input, gpio_ctrl_pin_id, 4);

    expect_function_call(__wrap_notify_accelerators_uefi_boot);
    will_return(__wrap_notify_accelerators_uefi_boot, 0); // accelerators were notified

    expect_function_call(__wrap__txe_timer_deactivate);

    notify_boot_complete_cb(0);
}

TEST_FUNCTION(test_accel_boot_notify_err, NULL, NULL)
{
    expect_function_call(__wrap__txe_timer_create);

    accel_boot_notify_service();

    expect_function_call(__wrap_gpio_get_input);
    will_return(__wrap_gpio_get_input, true);
    expect_value(__wrap_gpio_get_input, gpio_ctrl_pin_id, 4); // MSCP_EXP_GPIO_4

    expect_function_call(__wrap_notify_accelerators_uefi_boot);
    will_return(__wrap_notify_accelerators_uefi_boot, 1); // accelerators were not notified

    expect_function_call(__wrap_crash_dump_bug_check);

    notify_boot_complete_cb(0);
}
