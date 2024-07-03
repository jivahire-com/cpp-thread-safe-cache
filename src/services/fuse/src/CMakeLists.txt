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
        fwlib.lib.init
        fwlib.lib.mboxPrimitives
        fwlib.driver.mboxIccTransport
        fwlib.service.cli
        ms::lib::event_trace::trace
        ms::lib::register_headers
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

