set(TARGET oob_telm_mcp_init.test)

set(SOURCES
    test_oob_telm_mcp_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/oob_uart_mcp_init.c
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.driver.interface.text_io
        fwlib.lib.init
        fwlib.lib.mctp
        fwlib.lib.mctp_uart_binding
        fwlib.lib.status
        fwlib.lib.utils
        fwlib.platform.dfwkhost.threadx
        fwlib.service.pldm
        ms::cmocka::testrunner
        ms::driver::textio::pl011
        ms::lib::config_manager
        ms::lib::interrupts
        ms::lib::mcp_interrupts
        ms::lib::pldm_pdr
        ms::lib::register_headers
        ms::lib::threadx
        silibs::idsw
        silibs::idsw_kng
        silibs::common::lib_uart_pl011
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    config_get_uart_mcp_reassign
    idsw_get_die_id
    idsw_get_platform_sdv  
    DfwkDeviceInitialize
    fpfw_init_get_handle
    textio_pl011_device_initialize
    textio_pl011_device_interface_initialize
    fpfw_mctp_uart_binding_init
    fpfw_mctp_init
    pldm_pdr_get_timestamp
    fpfw_pldm_service_init
    fpfw_pldm_service_register_platform_event_ready_notification
)