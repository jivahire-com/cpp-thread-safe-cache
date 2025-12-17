set(TARGET kng.driver.scp_pcie_manager)

# Needed to ensure that silibs enums > 32-bit are compatible with MSCP
add_compile_options(-DMSCP_SILIBS_COMPILATION)

add_library(${TARGET}
    STATIC
    pcie_async_dispatch.c
    pcie_bdat.c
    pcie_config.c
    pcie_cli_helpers.c
    pcie_dfwk_init.c
    pcie_einj_helpers.c
    pcie_events.c
    pcie_irq.c
    pcie_rpss_init.c
    pcie_platform_overrides.c
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.cfg_mgr
        fwlib.lib.interface.platform.dbgprint
        fwlib.lib.interface.platform.errorhandler
        fwlib.lib.utils
        fwlib.platform.dfwkhost.threadx
        silibs::atu
        silibs::idsw
        silibs::idsw_kng
        silibs::intu
        silibs::kng_silibs_platform
        silibs::pcie
        silibs::pciess
        silibs::pcr_rpss
        silibs::tower_rpss
        ms::driver::vab
        ms::lib::crash_dump
        ms::lib::event_trace::trace
        ms::lib::ift
        ms::lib::scp_interrupts
        ms::lib::system_info
        ms::lib::threadx
        ms::lib::utils
        ms::service::health_monitor
        silibs_platform
    PUBLIC
        ms::cmsis::interface
)

target_include_directories(${TARGET}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${REPO_ROOT_DIR}/.externs/repo/silibs/soc_fw_shared/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(ms::driver::pcie ALIAS kng.driver.scp_pcie_manager)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

target_compile_options(kng.driver.scp_pcie_manager PRIVATE "-Os")
