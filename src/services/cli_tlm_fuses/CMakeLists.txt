#
# Copyright (c) Microsoft Corporation. All rights reserved.
#

set(TARGET kng.service.telemetry.fuses.cli)

add_library(${TARGET}
    STATIC
        tlm_fuses_cli_service.c
)

target_link_libraries(
    ${TARGET}
        PRIVATE
        fwlib.lib.status
        fwlib.lib.utils
        fwlib.service.cli
        silibs::kng_silibs_platform
        ms::lib::telemetry::fuses
        ms::lib::utils
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

add_library(ms::service::telemetry::fuses::cli ALIAS ${TARGET})

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
