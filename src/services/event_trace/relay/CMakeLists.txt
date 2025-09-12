set(TARGET kng.service.event_trace.relay)

add_library(${TARGET}
    STATIC
        etr_hsp_manager.c
        event_trace_relay.c
)

target_link_libraries(
    ${TARGET}
    PRIVATE
        IFPFW.EventTracing
        fwlib.lib.assert
        fwlib.lib.interface.platform.dbgprint
        fwlib.lib.interface.platform.errorhandler
        fwlib.lib.interface.platform.specialization.mts
        fwlib.service.iccBase
        ms::lib::crash_dump
        ms::lib::hsp_firmware_headers
        ms::service::atu_service
        ms::service::mts
        silibs::idsw_kng
    PUBLIC
        fwlib.lib.interface.platform.threadx
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
