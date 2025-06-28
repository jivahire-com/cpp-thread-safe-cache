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
        fwlib.driver.interface.text_io
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.platform.dfwkhost.threadx
        ms::build_data
        ms::cmocka::testrunner
        ms::driver::textio::pl011
        ms::lib::scp_interrupts
        ms::lib::stdio_textio
        ms::lib::systick_update
        ms::lib::threadx
        silibs::common::lib_uart_pl011
        silibs::idsw
        silibs::idsw_kng
        silibs::platform
        silibs::kng_silibs_platform
)

target_compile_definitions(${TARGET} 
    PRIVATE 
        UART_BASE_ADDR=123456
        UART_IRQ=654321
        VUART_BASE_ADDR=123456
        VUART_IRQ=654321)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    fpfw_init_get_handle
    systick_get_emcpu_clock
    textio_pl011_device_initialize
    textio_pl011_device_interface_initialize
    idsw_get_platform_sdv
)
