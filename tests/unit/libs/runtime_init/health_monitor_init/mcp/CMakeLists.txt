set(TARGET health_monitor_init_mcp.test)

set(SOURCES
    test_health_monitor_init_mcp.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/health_monitor_mcp_init.c
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
        ms::driver::pldm # PLDM_DRV_WORKAROUND
        ms::lib::accelerator_ip
        ms::service::atu_service
        ms::service::health_monitor
        ms::service::health_monitor::cli
        ms::lib::crash_dump
        fwlib.service.pldm
        ms::lib::pldm_pdr
        ms::lib::pldm_common_headers
        extern.libpldm
)

target_compile_definitions(
    ${TARGET}
    PRIVATE
       PLDM_DRV_WORKAROUND
)

# Generic ports implmented within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    accel_is_isolation_enabled
    fpfw_init_get_handle
    idsw_get_die_id
    hm_pre_ddr_init
    hm_post_ddr_init
    hm_post_intercore_init
    hm_cli_init
    idhw_is_single_die_boot_en
    # fpfw_pldm_service_register_platform_event_ready_notification  # !PLDM_DRV_WORKAROUND
    hm_set_pldm_ready_status
    DfwkAsyncRequestSetCompletionRoutine                # PLDM_DRV_WORKAROUND
    pldm_drv_register_platform_event_ready_notification # PLDM_DRV_WORKAROUND
)
