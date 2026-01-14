# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET kng.service.event_trace_relay.cli)

add_library(${TARGET}
    STATIC
    cli_et_telemetry.c
)

target_link_libraries(
    ${TARGET}
    PRIVATE
    fwlib.lib.init
    fwlib.lib.utils
    fwlib.service.cli
    ms::lib::crash_dump
    ms::service::event_trace::relay
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

add_library(ms::service::event_trace::relay::cli ALIAS ${TARGET})
