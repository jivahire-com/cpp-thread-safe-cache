set(TARGET pwr_tlm_svc_mcp_init.test)

set(SOURCES
    test_tlm_svc_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/pwr_tlm_svc_mcp_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.utils
        ms::lib::config_manager
        ms::service::pwr_tlm_mcp
        ms::service::pwr_tlm_mcp::cli
        ms::cmocka::testrunner
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs_platform
)

# Defining platform APIs
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
    config_get_pwr_tlm_knobs
    idsw_get_die_id
    idhw_is_single_die_boot_en
    telemetry_service_init
    pwr_tlm_cli_svc_init
)
