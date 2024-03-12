set(TARGET kng.lib.event_trace.collector)

add_library(${TARGET} 
    STATIC 
        event_trace_collector.c
)

target_link_libraries(
    ${TARGET} 
    PRIVATE
        EventTracingDecoder
        EventTracing
        fwlib.platform.errorhandler
        ms::lib::threadx
    PUBLIC
        IFPFW.EventTracing
)

target_include_directories(${TARGET}
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::event_trace::collector ALIAS ${TARGET})
