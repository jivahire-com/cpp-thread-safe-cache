set(TARGET kng.driver.scp_pcie_manager)

add_library(${TARGET}
    STATIC
    pcie_async_dispatch.c
    pcie_dfwk_init.c
    pcie_link_training.c
)

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.assert
    fwlib.dfwk
    fwlib.platform.errorhandler
    fwlib.platform.dfwkhost.threadx
    fwlib.utils
    kng_silibs_platform
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
