set(TARGET kng.service.scp_pcie_manager)

add_library(${TARGET}
    STATIC
        pcie_manager_init.c
        pcie_rpss_manager_thread.c
        pcie_config_manager_thread.c
        pcie_manager_callbacks.c
)

target_link_libraries(
    ${TARGET}
        PRIVATE
        fwlib.dfwk
        fwlib.platform.dfwkhost.threadx
        fwlib.lib.interface.platform.errorhandler
        kng_silibs_platform
        ms::driver::pcie
        ms::lib::debug
        ms::lib::threadx
        silibs::idsw
        silibs::idsw_kng
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
