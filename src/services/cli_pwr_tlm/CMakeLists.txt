#
# Copyright (c) Microsoft Corporation. All rights reserved.
#

set(TARGET kng.service.pwr_tlm_mcp.cli)

add_library(${TARGET}
    STATIC
        pwr_tlm_cli_service.c
)

target_link_libraries(
    ${TARGET}
        PRIVATE
        fwlib.service.cli
        silibs::ddr_i3c
        silibs::ddrss
        silibs::idhw
        silibs::kng_silibs_platform
        silibs::platform
        silibs::ras_arm
        ms::lib::error_codes
        ms::lib::telemetry::fuses
        ms::service::pwr_tlm_mcp
        pwr_tlm_mcp.service.cmpnt.interfaces #give cli access to internal component headers
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

add_library(ms::service::pwr_tlm_mcp::cli ALIAS ${TARGET})

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
