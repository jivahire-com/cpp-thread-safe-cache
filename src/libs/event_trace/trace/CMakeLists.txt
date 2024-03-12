set(TARGET kng.lib.event_trace.trace)

add_library(${TARGET} INTERFACE)

target_link_libraries(${TARGET} 
    INTERFACE
        fwlib.utils
        EventTracing
        IFPFW.EventTracing
)

target_include_directories(${TARGET} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/inc)

add_library(ms::lib::event_trace::trace ALIAS ${TARGET})
