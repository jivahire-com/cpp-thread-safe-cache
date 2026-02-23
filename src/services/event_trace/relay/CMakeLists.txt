# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET kng.service.event_trace.relay)

add_library(${TARGET}
    STATIC
        etr_hsp_manager.c
        event_trace_relay.c
        event_trace_relay_events.c
        event_trace_relay_quiesce.c
)

target_link_libraries(
    ${TARGET}
    PRIVATE
        IFPFW.EventTracing
        fwlib.dfwk
        fwlib.lib.cfg_mgr
        fwlib.lib.init
        fwlib.lib.interface.platform.dbgprint
        fwlib.lib.interface.platform.errorhandler
        fwlib.lib.interface.platform.specialization.mts
        fwlib.service.iccBase
        fwlib.service.mts
        fwlib.service.utc_sync.client
        ms::cmsis::interface
        ms::lib::accelerator_ip
        ms::lib::crash_dump
        ms::lib::ddr_manager
        ms::lib::error_codes
        ms::lib::event_trace::trace
        ms::lib::gtimer
        ms::lib::hsp_firmware_headers
        ms::lib::register_headers
        ms::service::atu_service
        ms::service::event_trace::collector
        ms::service::event_trace::et_mts_clnt
        ms::service::mts
        ms::service::scp_startup_shutdown
        silibs::idsw_kng
    PUBLIC
        fwlib.lib.interface.platform.threadx
        ms::lib::interface::in_band_telemetry_ddr
        ms::lib::telemetry::fuses
        ms::service::mts
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
