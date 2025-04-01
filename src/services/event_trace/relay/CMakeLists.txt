set(TARGET kng.service.event_trace.relay)

add_library(${TARGET}
    STATIC
        event_trace_relay.c
)

target_link_libraries(
    ${TARGET}
    PRIVATE
        fwlib.lib.assert
    PUBLIC
        fwlib.lib.interface.platform.errorhandler
        fwlib.lib.interface.platform.lock
        fwlib.lib.interface.platform.threadx
        fwlib.service.iccBase
        IFPFW.EventTracing
        ms::lib::hsp_firmware_headers
        ms::lib::interface::in_band_telemetry_ddr
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

add_library(ms::service::event_trace::relay ALIAS ${TARGET})
