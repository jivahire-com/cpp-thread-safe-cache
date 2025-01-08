set(TARGET kng.lib.power_fuse)

add_library(${TARGET}
    STATIC
        src/power_tlm_fuse.c
        src/power_fuse_events.c
)

target_link_libraries(${TARGET}
    PUBLIC
   
    PRIVATE
    ms::lib::event_trace::trace
    ms::lib::fuse_artifacts_lib
    ms::lib::register_headers
    silibs::fuse
    silibs::idsw_kng
    silibs::kng_silibs_platform
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src       
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::power_fuse ALIAS ${TARGET})
