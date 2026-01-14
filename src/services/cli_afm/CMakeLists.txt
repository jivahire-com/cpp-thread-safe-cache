# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET kng.service.afm.cli)

add_library(${TARGET}
    STATIC
    afm_cli.c
)

target_link_libraries(
    ${TARGET}
    PRIVATE
    fwlib.lib.init
    fwlib.service.cli
    ms::driver::gpio
    ms::lib::crash_dump
    ms::lib::icc_platform_defines
    silibs::icc_mhu
    silibs::idsw_kng
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

add_library(ms::service::afm::cli ALIAS ${TARGET})
