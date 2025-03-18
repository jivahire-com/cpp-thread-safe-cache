set(TARGET mcp_error_domain.test)

set(SOURCES
    test_error_domain_register_mcp.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/error_domain/src/error_domain_register_mcp.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.platform.dfwkhost.threadx
        silibs::idsw_kng
        silibs::semaphore
        silibs::kng_silibs_platform
        silibs::idhw
        silibs::idsw
        ms::cmocka::testrunner
        ms::service::atu_service
        ms::service::health_monitor
        ms::service::health_monitor::cli
        ms::lib::error_domain::mcp
        ms::lib::crash_dump
)

# Generic ports implement within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_compile_definitions(
    ${TARGET}
    PRIVATE
       MCP_RUNTIME_INIT
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    fpfw_icc_base_send
    fpfw_icc_base_recv
)
