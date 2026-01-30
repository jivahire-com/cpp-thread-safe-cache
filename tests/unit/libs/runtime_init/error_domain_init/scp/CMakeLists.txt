# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET scp_error_domain_init.test)

set(SOURCES
    test_err_domain_scp_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/err_domain_scp_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.platform.dfwkhost.threadx
        silibs::atu
        silibs::idsw_kng
        silibs::semaphore
        silibs::kng_silibs_platform
        silibs::idhw
        silibs::idsw
        ms::cmocka::testrunner
        ms::service::atu_service
        ms::service::health_monitor
        ms::service::health_monitor::cli
        ms::lib::boot_status
        ms::lib::error_domain::scp
)

# Generic ports implement within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

target_compile_definitions(
    ${TARGET}
    PRIVATE
       SCP_RUNTIME_INIT
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    fpfw_init_get_handle
    mcp_error_injection_setup_listener
    register_scp_error_domain
    register_smmu_error_domain
    register_gic_error_domain
    register_ap_wdt_error_domain
    boot_status_notify_extd
    idsw_get_cpu_type
    idsw_get_die_id
)
