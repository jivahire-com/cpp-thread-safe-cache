set(TARGET kng.service.pcie_manager.cli)

add_library(${TARGET}
    STATIC
    pcie_cli_main.c
    pcie_cli_pretty_print.c
)

target_link_libraries(
    ${TARGET}
    PRIVATE
    fwlib.dfwk
    fwlib.platform.dfwkhost.threadx
    fwlib.service.cli
    ms::driver::pcie
    ms::lib::threadx
    silibs::kng_silibs_platform
    silibs::pcie
    silibs::pciess
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(ms::service::pcie_manager::cli ALIAS ${TARGET})

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
