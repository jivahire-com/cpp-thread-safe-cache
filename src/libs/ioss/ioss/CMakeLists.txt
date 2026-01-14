# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET kng.lib.ioss_ini)

add_library(${TARGET}
    STATIC
        ioss_ini.c
)

target_link_libraries(${TARGET}
    PRIVATE
        kng_silibs_platform
        silibs_platform
        fwlib.lib.cfg_mgr
        silibs::atu
        silibs::ioss_init
        silibs::usb_init
        ms::lib::crash_dump
        ms::lib::variable_services
        ms::lib::error_codes
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${REPO_ROOT_DIR}/.externs/repo/silibs/soc_fw_shared/include
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::ioss_ini ALIAS ${TARGET})