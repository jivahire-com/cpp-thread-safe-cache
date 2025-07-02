set(TARGET kng.service.scp_pcie_manager)

add_library(${TARGET}
    STATIC
        pcie_async_requests.c
        pcie_error_handling.c
        pcie_error_injection.c
        pcie_manager_init.c
        pcie_rpss_manager_thread.c
        pcie_config_manager_thread.c
        pcie_link_management.c
        pcie_lt_events.c
        pcie_manager_callbacks.c
        pcie_rp_event_handler.c
        pcie_sync_requests.c
        pcie_phy_load_events.c
)

target_link_libraries(
    ${TARGET}
        PRIVATE
        fwlib.dfwk
        fwlib.platform.dfwkhost.threadx
        fwlib.lib.interface.platform.dbgprint
        fwlib.lib.interface.platform.errorhandler
        fwlib.lib.cfg_mgr
        kng_silibs_platform
        ms::driver::pcie
        ms::lib::debug
        ms::lib::threadx
        ms::lib::variable_services
        ms::lib::error_codes
        ms::lib::crash_dump
        ms::lib::system_info
        ms::lib::accelerator_ip
        ms::service::health_monitor
        ms::service::scp_startup_shutdown
        silibs::idsw
        silibs::idsw_kng
        silibs::pciess
        silibs::pcie
        silibs::ras_framework
        silibs_platform
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
        ${REPO_ROOT_DIR}/.externs/repo/silibs/soc_fw_shared/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(ms::service::pcie_manager ALIAS kng.service.scp_pcie_manager)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
