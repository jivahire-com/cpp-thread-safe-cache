set(TARGET kng.driver.scp_pcie_manager)

add_library(${TARGET}
    STATIC
    pcie_async_dispatch.c
    pcie_dfwk_init.c
    pcie_link_training.c
    pcie_rpss_init.c
    pcie_platform_overrides.c
)

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.assert
    fwlib.dfwk
    fwlib.lib.interface.platform.errorhandler
    fwlib.lib.utils
    fwlib.platform.dfwkhost.threadx
    silibs::atu
    silibs::idsw
    silibs::kng_silibs_platform
    silibs::pcie
    silibs::pciess
    silibs::pcr_rpss
    silibs::tower_rpss
    ms::lib::threadx
    silibs_platform
)

target_include_directories(${TARGET}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(ms::driver::pcie ALIAS kng.driver.scp_pcie_manager)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

target_compile_options(kng.driver.scp_pcie_manager PRIVATE "-O2")
