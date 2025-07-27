set(TARGET kng.lib.core_info)

add_library(${TARGET}
    STATIC
        core_info.c
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.cfg_mgr
        ms::lib::register_headers
        ms::lib::hsp_firmware_headers
        ms::lib::crash_dump
        silibs::kng_silibs_platform
        silibs::platform
    PUBLIC
        fwlib.service.iccBase
        ms::lib::corebits
        silibs::fuse
        ms::service::fuse
        silibs::fuse_common
        silibs::idsw_kng
        silibs::platform
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::core_info ALIAS ${TARGET})
