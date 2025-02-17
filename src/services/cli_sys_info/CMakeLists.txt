# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET kng.service.sys_info.cli)

# Add target for the service
add_library(${TARGET}
        STATIC
        cli_sys_info.c
)

target_include_directories(
    ${TARGET}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/inc
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    ${TARGET}
        PRIVATE
            fwlib.service.iccBase
            fwlib.service.cli
            silibs::kng_silibs_platform
            silibs::platform
        PUBLIC
            silibs::idsw_kng
            ms::lib::system_info
)

add_library(ms::service::sys_info::cli ALIAS ${TARGET})

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

