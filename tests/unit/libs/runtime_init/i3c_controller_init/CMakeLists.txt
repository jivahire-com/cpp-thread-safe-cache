# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET i3c_controller_init.test)

set(SOURCES
    test_i3c_controller_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/i3c_controller_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

if (i386-pc-windows-msvc)
    set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
    set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
endif()

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.lib.interface.platform.interrupts
        fwlib.lib.assert
        idhw
        ms::cmocka::testrunner
        ms::lib::crash_dump
        ms::lib::register_headers
        ms::lib::scp_interrupts
        ms::lib::i3c_controller
        silibs_platform
        ms::lib::boot_status
        ms::lib::utils
)

# Defining platform APIs
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
    i3c_controller
    idsw_get_die_id
    boot_status_notify_extd
)
