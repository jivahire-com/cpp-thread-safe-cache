# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET cli_init.test)

set(SOURCES
    test_cli_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/cli_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.service.cli
        fwlib.dfwk
        fwlib.lib.init
        fwlib.lib.interface.platform.dbgprint
        fwlib.lib.utils
        ms::cmocka::testrunner
        ms::driver::textio::pl011
        ms::lib::threadx
        ms::lib::system_info
        silibs::common::lib_uart_pl011
        silibs::platform
        silibs::kng_silibs_platform
        ms::lib::utils
)

# Generic ports implmented within this project
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
    textio_pl011_device_interface_initialize
    FpFwCliInitialize
    fpfw_init_get_handle
    FpFwCliStart
    system_info_get_cli_enable
)
