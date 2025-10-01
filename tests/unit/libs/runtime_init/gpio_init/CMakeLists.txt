set(TARGET gpio_init.test)

set(SOURCES
    test_gpio_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/gpio_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_include_directories(${TARGET}
    PRIVATE
        ${CRASH_DUMP_SOURCE_DIR}/inc
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.cfg_mgr
        fwlib.lib.init
        fwlib.lib.interface.platform.dbgprint
        fwlib.platform.dfwkhost.threadx
        fwlib.service.cli
        ms::cmocka::testrunner
        ms::driver::gpio
        ms::lib::crash_dump
        ms::lib::interrupts
        ms::lib::scp_interrupts
        ms::lib::threadx
        ms::lib::variable_services
        ms::service::gpio::cli
        silibs::idsw
        silibs::idsw_kng
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    fpfw_init_get_handle
    FpFwCliRegisterTable
    gpio_afm_init
    gpio_device_init
    gpio_init
    gpio_interface_init
    gpio_override_uart_afmsel
    idsw_get_cpu_type
    idsw_get_die_id
    idsw_get_platform_sdv
    mmio_read32
    mmio_write32
)
