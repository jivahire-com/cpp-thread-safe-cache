set(TARGET kng.lib.tower)

add_library(${TARGET}
    STATIC
        tower.c
        tower_isr.c
)

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.assert
    fwlib.lib.cfg_mgr
    fwlib.lib.interface.platform.dbgprint
    fwlib.service.iccBase
    silibs_platform
    ms::lib::accelerator_ip
    ms::lib::crash_dump
    ms::lib::error_codes
    ms::lib::hsp_firmware_headers
    ms::lib::register_headers
    ms::lib::system_info
    ms::service::health_monitor
    silibs::atu
    silibs::kng_silibs_platform
    silibs::tower_sequence
    silibs::tower_utility
    silibs::idhw
    silibs::idsw
    silibs::idsw_kng
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::tower ALIAS ${TARGET})
