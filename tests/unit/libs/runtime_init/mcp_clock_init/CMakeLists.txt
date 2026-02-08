# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET mcp_clock_init.test)

set(SOURCES
    test_mcp_clock_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/mcp_clock_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.assert
        fwlib.lib.init
        fwlib.lib.utils
        ms::cmocka::testrunner
        ms::lib::crash_dump
        silibs::pik_clock
        silibs::platform
)

# Generic ports implemented within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    pik_clock_init
    pik_clock_power_transition
)
