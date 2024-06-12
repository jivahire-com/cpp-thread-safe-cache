# define target for fuse service
set(TARGET kng.service.fuse_service)

add_library(${TARGET}
    STATIC
        fuse_main.c
        fuse_events.c
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
        ms::lib::event_trace::trace
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::fuse ALIAS ${TARGET})
