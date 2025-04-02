set(TARGET kng.drivers.vab)

add_library(${TARGET}
    STATIC
        vab_common_setup.c
        vab_irq.c
)

target_link_libraries(
    ${TARGET}
        PRIVATE
            fwlib.dfwk
            fwlib.lib.assert
            fwlib.platform.dfwkhost.threadx
            fwlib.lib.cfg_mgr
            ms::driver::pcie
            ms::lib::debug
            ms::lib::interrupts
            ms::lib::register_headers
            ms::lib::scp_interrupts
            ms::lib::threadx
            silibs::atu
            silibs::intu
            silibs::kng_silibs_platform
            silibs::pcie
            silibs::platform
            silibs::smmu
            silibs::vab_init
            silibs_platform
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(ms::driver::vab ALIAS kng.drivers.vab)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
