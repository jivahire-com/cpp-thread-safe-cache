# define target for fuse service
set(TARGET kng.service.fuse_service)

add_library(${TARGET}
    STATIC
        fuse_main.c
        fuse_events.c
        fuse_cli.c
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.dfwk
    fwlib.lib.interface.platform.errorhandler
    fwlib.lib.interface.platform.threadx
    fwlib.lib.init
    fwlib.lib.status
    fwlib.service.iccBase
    fwlib.service.cli
    ms::lib::crash_dump::scp
    ms::lib::error_codes
    ms::boot_loader::interface
    ms::lib::error_codes
    ms::lib::event_trace::trace
    ms::lib::debug
    ms::lib::hsp_firmware_headers
    ms::lib::register_headers
    ms::lib::threadx
    ms::lib::utils
    silibs::fuse
    silibs::idsw
    silibs::idsw_kng
    silibs::platform
    silibs::kng_silibs_platform    
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::fuse ALIAS ${TARGET})

