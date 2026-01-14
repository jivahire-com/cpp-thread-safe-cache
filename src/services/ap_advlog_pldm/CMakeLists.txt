# Copyright (c) Microsoft Corporation. All rights reserved.

# MCP
set(TARGET kng.service.ap_advlog_pldm_mcp)

set (SOURCES
        ap_advlog_pldm.c
        ap_advlog_parse.c
        ap_advlog_pldm_events.c
)
add_library(${TARGET}  STATIC ${SOURCES})

target_link_libraries(
    ${TARGET}
        PUBLIC
            fwlib.lib.status
            ms::cmsis::interface
        PRIVATE
            fwlib.lib.assert
            fwlib.lib.utils
            fwlib.service.pldm
            kng_silibs_platform
            ms::lib::crash_dump
            ms::lib::cortex_m7::atomics
            ms::lib::pldm_pdr
            ms::lib::pldm_common_headers
            ms::lib::zlib
            ms::service::atu_service
            silibs::idsw
            silibs::idsw_kng
            silibs::atu
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::ap_advlog_pldm ALIAS ${TARGET})
