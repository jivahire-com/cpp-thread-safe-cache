# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET kng.lib.power_telemetry_common)

add_library(${TARGET}
    STATIC
        src/power_telemetry_common.c
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(${TARGET}
    PUBLIC
        fwlib.lib.cfg_mgr
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::power_telemetry_common ALIAS ${TARGET})
