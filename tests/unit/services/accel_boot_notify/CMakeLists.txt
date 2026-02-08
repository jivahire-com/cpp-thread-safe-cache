# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET accel_boot_notify.test)

# Compile test
add_executable(${TARGET}
    accel_boot_notify_test.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.assert
        ms::cmocka::testrunner
        ms::service::accel_boot_notify
        fwlib.lib.utils
        ms::lib::threadx
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    _txe_timer_create
    _txe_timer_deactivate
    notify_accelerators_uefi_boot
    gpio_get_input
    crash_dump_bug_check    
)