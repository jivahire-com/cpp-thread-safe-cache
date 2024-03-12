set(TARGET kng.service.scp_etc)

add_library(${TARGET} 
    STATIC 
        scp_event_trace_collector_events.c
        scp_event_trace_collector.c
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

add_library(ms::service::scp_etc ALIAS kng.service.scp_etc)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
