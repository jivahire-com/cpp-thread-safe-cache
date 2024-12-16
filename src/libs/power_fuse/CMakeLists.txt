set(TARGET kng.lib.power_fuse)

add_library(${TARGET}
    STATIC
        src/power_tlm_fuse.c
        src/power_fuse_events.c
)

target_link_libraries(${TARGET}
    PUBLIC
    silibs::atu
    ms::lib::corebits
    
    PRIVATE
    fwlib.dfwk
    fwlib.platform.dfwkhost.threadx
    fwlib.lib.assert
    fwlib.lib.interface.platform.errorhandler
    fwlib.lib.init
    fwlib.lib.status
    ms::lib::error_codes
    ms::lib::event_trace::trace
    ms::lib::crash_dump
    ms::lib::debug
    ms::lib::fuse_artifacts_lib
    ms::lib::register_headers
    ms::lib::system_info
    ms::lib::threadx
    ms::lib::utils
    silibs::fuse
    silibs::idsw
    silibs::idsw_kng
    silibs::platform
    silibs::kng_silibs_platform
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
        ${REPO_ROOT_DIR}/src/libs/utils/include

    PRIVATE
        silibs::idsw
        silibs::idsw_kng
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::power_fuse ALIAS ${TARGET})
