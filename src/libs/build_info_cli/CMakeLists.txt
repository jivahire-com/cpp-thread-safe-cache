# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET build_info_cli)

# Add target for the service
add_library(
    ${TARGET}
        STATIC
            build_info_cli.c
)

target_include_directories(
    ${TARGET}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(
    ${TARGET}
        PUBLIC
        ms::build_data
        PRIVATE
        fwlib.service.cli
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::build_info_cli ALIAS ${TARGET})
