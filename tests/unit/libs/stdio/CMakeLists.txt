# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET stdio_test.test)

# Compile test
add_executable(${TARGET} stdio_test.cpp
                         )

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.driver.interface.text_io
        ms::cmocka::testrunner
        ms::lib::stdio_textio
        ms::driver::textio::pl011
        ms::lib::crash_dump
        silibs::common::lib_uart_pl011
    PUBLIC
        fwlib.dfwk
        silibs::kng_silibs_platform
 )

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:ms::lib::scp_avs,INCLUDE_DIRECTORIES>)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    DfwkClientInterfaceOpen
    _txe_mutex_create
    _txe_mutex_get
    _txe_mutex_put
    uart_pl011_write_byte
    uart_pl011_write_assume_ready
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")