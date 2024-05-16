set(TARGET textio_init.test)

set(SOURCES
    test_textio_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/textio_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.drivers.interface.text_io
        fwlib.init
        fwlib.utils
        fwlib.platform.dfwkhost.threadx
        ms::cmocka::testrunner
        ms::driver::textio::pl011
        ms::lib::stdio_textio
        ms::lib::threadx
        silibs::common::lib_uart_pl011
        silibs::platform
        silibs::kng_silibs_platform
)

target_compile_definitions(${TARGET} PRIVATE UART_BASE_ADDR=123456)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    fpfw_init_get_handle
    textio_pl011_device_initialize
    textio_pl011_device_interface_initialize
)
