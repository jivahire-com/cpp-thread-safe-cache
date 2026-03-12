# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET wdog_init.test)

set(SOURCES
    test_watchdog_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/watchdog_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.assert
        fwlib.lib.init
        fwlib.lib.interface.platform.dbgprint
        fwlib.lib.utils
        ms::cmocka::testrunner
        ms::service::wdog
        ms::lib::crash_dump
        ms::lib::config_manager
        ms::lib::threadx
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::platform
        ms::lib::utils
)

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
    config_get_wdog_en
    idsw_get_platform_sdv
    wdog_service_init
)
