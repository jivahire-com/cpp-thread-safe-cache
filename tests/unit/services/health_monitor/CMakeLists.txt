set(TARGET health_monitor.test)

set(SOURCES
    test_hm_main.cpp
    test_hm_setup.cpp
    test_hm_error_report_ghes.cpp
    test_hm_error_domain_register.cpp
    test_hm_error_report_cper.cpp
    test_hm_error_injection.cpp
    test_hm_error_domain_mcp.cpp
    test_hm_error_domain_accel.cpp
    test_hm_error_domain_hsp.cpp
    ${CMAKE_SOURCE_DIR}/src/services/health_monitor/src/hm_main.c
    ${CMAKE_SOURCE_DIR}/src/services/health_monitor/src/hm_error_domain_register.c
    ${CMAKE_SOURCE_DIR}/src/services/health_monitor/src/hm_error_report_ghes.c
    ${CMAKE_SOURCE_DIR}/src/services/health_monitor/src/hm_error_domain_accel.c
    ${CMAKE_SOURCE_DIR}/src/services/health_monitor/src/hm_error_domain_hsp.c
    # ${CMAKE_SOURCE_DIR}/src/services/health_monitor/src/hm_error_domain_cded.c
    ${CMAKE_SOURCE_DIR}/src/services/health_monitor/src/hm_error_domain_apcore.c
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
        ms::service::health_monitor
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
    atu_svc_accel_atu_addr
    crash_dump_bug_check
    initialize_semaphore
    idhw_is_single_die_boot_en
    mscp_exp_spi_synchronize_dies
    wait_for_semaphore
    release_semaphore
    idsw_get_die_id
    fpfw_icc_base_send
    fpfw_icc_base_recv
    fpfw_icc_base_send_recv
    gpio_set_output
    mmio_write32
)
