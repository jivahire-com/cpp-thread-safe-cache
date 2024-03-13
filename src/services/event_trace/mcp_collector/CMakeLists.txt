set(TARGET kng.service.mcp_etc)

add_library(${TARGET} 
    STATIC 
        mcp_event_trace_collector_events.c
        mcp_event_trace_collector.c
)

target_link_libraries(${TARGET} 
    PUBLIC
        IFPFW.EventTracing
    PRIVATE
        fwlib.platform.errorhandler
        ms::build_data
        ms::lib::event_trace::collector
        ms::lib::event_trace::trace
        ms::lib::threadx

)

target_include_directories(${TARGET}
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(ms::service::mcp_etc ALIAS kng.service.mcp_etc)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
