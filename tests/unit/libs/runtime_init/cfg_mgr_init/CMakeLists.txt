# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET cfg_mgr_init.test)

set(SOURCES
    test_cfg_mgr_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/cfg_mgr_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.platform.dfwkhost.threadx
        fwlib.lib.linkedlist
        fwlib.lib.utils
        ms::cmocka::testrunner
        ms::lib::boot_status
        ms::lib::scp_interrupts
        ms::lib::threadx
        silibs_platform
        silibs::idsw_kng
        silibs::kng_silibs_platform
        ms::lib::config_manager
        ms::lib::config_manager::cli
        ms::lib::variable_services
        ms::service::atu_service
        fwlib.lib.interface.platform.dbgprint
        ms::lib::system_info
        ms::lib::utils
)

# Provide compile time definition
target_compile_definitions(
    ${TARGET}
    PRIVATE
        MSCP_EXP_SCP_CFGMGR_VARIABLE_SERVICE_PAYLOAD_BASE=MSCP_ATU_AP_WINDOW_VAR_SVC_BASE_ADDR
        MSCP_EXP_SCP_CFGMGR_VARIABLE_SERVICE_PAYLOAD_SIZE=MSCP_ATU_AP_WINDOW_VAR_SVC_DIE_SIZE
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    cfg_mgr_init
    cfg_mgr_cli_init
    idsw_get_platform_sdv
    system_info_get_bmc_profile
    boot_status_notify_extd
    idsw_get_cpu_type
    idsw_get_die_id
)
