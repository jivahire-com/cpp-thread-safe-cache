set(TARGET health_monitor_mcp.test)

set(SOURCES
    test_hm_main.cpp
    test_hm_setup.cpp
    test_hm_error_report_cper.cpp
    #${CMAKE_SOURCE_DIR}/src/services/health_monitor/src/mcp/hm_main.c
    #${CMAKE_SOURCE_DIR}/src/services/health_monitor/src/mcp/hm_error_report_ghes.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.platform.dfwkhost.threadx
        ms::cmocka::testrunner
        ms::service::health_monitor::mcp
        silibs::idsw_kng
        silibs::kng_silibs_platform
        silibs::idhw
        silibs::idsw
        ms::lib::crash_dump
)

# Get private includes for module under test
target_include_directories(${TARGET}
    PRIVATE
        $<TARGET_PROPERTY:ms::service::health_monitor,INCLUDE_DIRECTORIES>
        ./
)

# Generic ports implmented within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    wait_for_semaphore
    release_semaphore
    idsw_get_die_id
    gpio_set_output
    mmio_write32
    AP_GHES_ADDR
    MSCP_GHES_ADDR
)
