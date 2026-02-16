# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET kng.service.event_trace.et_mts_clnt)

add_library(${TARGET} 
    STATIC
        et_mts_client.c
        et_svc_events.c
)

target_include_directories(${TARGET} 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/inc_priv
)

target_link_libraries(${TARGET} 
    PRIVATE
        fwlib.lib.init
        ms::build_data
        ms::cmsis::interface
        ms::lib::crash_dump
        ms::lib::event_trace::trace
        ms::lib::gtimer
        ms::lib::register_headers
        ms::service::event_trace::collector
        ms::service::event_trace::decoder
        ms::service::event_trace::etc_etd
        ms::service::mts
        silibs::idsw
        silibs::idsw_kng
        silibs::platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::event_trace::et_mts_clnt ALIAS ${TARGET})
