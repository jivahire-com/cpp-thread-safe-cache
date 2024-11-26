set(TARGET kng.service.telemetry.cmpnt.event_trace)

add_library(${TARGET}
    STATIC
        telemetry_events.c
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
    PUBLIC
        ms::lib::event_trace::trace
    PRIVATE

)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
