# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET kng.lib.core_info)

add_library(${TARGET}
    STATIC
        core_info.c
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.cfg_mgr
        fwlib.lib.interface.platform.dbgprint
        ms::lib::crash_dump
        ms::lib::hsp_firmware_headers
        ms::lib::register_headers
        silibs::kng_silibs_platform
        silibs::platform
    PUBLIC
        fwlib.service.iccBase
        ms::lib::corebits
        ms::service::fuse
        silibs::fuse
        silibs::fuse_common
        silibs::idsw_kng
        silibs::platform
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::core_info ALIAS ${TARGET})
