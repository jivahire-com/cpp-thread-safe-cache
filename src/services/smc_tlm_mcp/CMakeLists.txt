# Copyright (c) Microsoft Corporation. All rights reserved.

# MCP
set(TARGET kng.service.smc_tlm_mcp)

set (SOURCES
        smc_tlm_mcp.c
        smc_tlm_mcp_events.c
)
add_library(${TARGET}  STATIC ${SOURCES})

target_link_libraries(
    ${TARGET}
        PUBLIC
            fwlib.lib.status
            ms::cmsis::interface
            ms::lib::event_trace::trace
            ms::service::atu_service
            silibs::atu
        PRIVATE
            fwlib.lib.assert
            fwlib.lib.utils
            kng_silibs_platform
            ms::lib::crash_dump
            ms::lib::cortex_m7::atomics
            ms::lib::gtimer
            ms::lib::zlib
            silibs::idsw
            silibs::idsw_kng
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::smc_tlm_mcp ALIAS ${TARGET})
