# Copyright (c) Microsoft Corporation. All rights reserved.

# MCP 
set(TARGET kng.service.power_pldm_mcp)

set (SOURCES 
        power_pldm.c
        power_pldm_events.c
)
add_library(${TARGET}  STATIC ${SOURCES})

target_link_libraries(
    ${TARGET}
        PUBLIC
            fwlib.lib.status
        PRIVATE
            fwlib.lib.assert
            fwlib.lib.utils
            fwlib.service.iccBase
            fwlib.service.pldm
            ms::lib::crash_dump
            ms::lib::pldm_pdr
            ms::lib::pldm_common_headers
            silibs::icc_mhu
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

add_library(ms::service::pwr_pldm_mcp ALIAS ${TARGET})
