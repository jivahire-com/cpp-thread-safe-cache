set(TARGET kng.lib.pex_rng)

add_library(${TARGET}
    STATIC
        src/pex_rng.c
)

target_link_libraries(${TARGET}
    PUBLIC
    silibs::atu
    ms::lib::corebits
    
    PRIVATE
    fwlib.dfwk
    fwlib.lib.init
    fwlib.lib.interface.platform.dbgprint
    fwlib.platform.dfwkhost.threadx
    ms::lib::crash_dump
    ms::lib::threadx
    silibs_platform
    silibs::idsw
    silibs::idsw_kng
    ms::lib::register_headers
    silibs::crypto::rng
    silibs::dvfs
    ms::service::health_monitor
    ms::lib::error_domain::scp 
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

add_library(ms::lib::pex_rng ALIAS ${TARGET})
