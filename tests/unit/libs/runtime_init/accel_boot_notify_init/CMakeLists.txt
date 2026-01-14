# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET accel_boot_notify_init.test)

set(SOURCES
    test_accel_boot_notify_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/accel_boot_notify_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.lib.interface.platform.dbgprint
        ms::cmocka::testrunner
        ms::lib::ift
        ms::service::accel_boot_notify
        silibs::platform
        ms::lib::system_info
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
accel_boot_notify_service
ift_is_enabled
system_info_is_warm_start
)
