# Copyright (c) Microsoft Corporation. All rights reserved.

# Exception handler
set(TARGET kng.lib.d2d_cntr_sync)

set (SOURCES
    src/d2d_cntr_sync.c
)

add_library(${TARGET}
    STATIC
        ${SOURCES}
)

target_link_libraries(${TARGET}
    PUBLIC
        fwlib.lib.interface.platform.dbgprint

    PRIVATE
        fwlib.lib.cfg_mgr
        ms::lib::crash_dump
        silibs::d2d_counter_sync
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
        silibs_platform
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::d2d_cntr_sync ALIAS ${TARGET})