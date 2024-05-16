set(TARGET cli_init.test)

set(SOURCES
    test_cli_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/cli_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.cli
        fwlib.dfwk
        fwlib.init
        fwlib.utils
        ms::cmocka::testrunner
        ms::driver::textio::pl011
        ms::lib::threadx
        silibs::common::lib_uart_pl011
        silibs::platform
        silibs::kng_silibs_platform
)

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
)
