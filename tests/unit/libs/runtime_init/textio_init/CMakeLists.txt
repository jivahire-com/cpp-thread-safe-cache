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
        fwlib.init
        fwlib.utils
        ms::cmocka::testrunner
        ms::driver::textio::pl011
        ms::lib::stdio_textio
        silibs::platform
        silibs::kng_silibs_platform
)

target_compile_definitions(${TARGET} PRIVATE UART_BASE_ADDR=0)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    textio_pl011_device_initialize
    textio_pl011_device_interface_initialize
)
