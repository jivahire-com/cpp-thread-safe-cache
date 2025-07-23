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
        ${REPO_ROOT_DIR}/src/libs/icc_platform_defines
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.dfwk
    fwlib.platform.dfwkhost.threadx
    fwlib.lib.cfg_mgr
    fwlib.lib.interface.platform.errorhandler
    fwlib.lib.init
    fwlib.lib.status
    fwlib.service.iccBase
    fwlib.service.cli
    ms::boot_loader::interface
    ms::lib::crash_dump
    ms::lib::error_codes
    ms::lib::error_codes
    ms::lib::event_trace::trace
    ms::lib::debug
    ms::lib::fuse_artifacts_lib
    ms::lib::hsp_firmware_headers
    ms::lib::register_headers
    ms::lib::system_info
    ms::lib::threadx
    ms::lib::utils
    ms::service::sds_service
    silibs::fuse
    silibs::fuse_common
    silibs::idsw
    silibs::idhw
    silibs::idsw_kng
    silibs::platform
    silibs::kng_silibs_platform
    ms::lib::mesh
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::fuse ALIAS ${TARGET})
