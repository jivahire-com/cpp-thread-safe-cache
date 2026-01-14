# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET cli_ddr)

# Add target for the service
add_library(
    ${TARGET}
        STATIC
        cli_ddr.c
)

target_include_directories(
    ${TARGET}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/inc
            ${REPO_ROOT_DIR}/.externs/repo/silibs/soc_fw_shared/include
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    ${TARGET}
        PUBLIC
        silibs::ddrss
        ms::lib::ddr_manager
        ms::lib::ddrss_init
        PRIVATE
        fwlib.service.cli
        ms::lib::ddrss_init
        ms::lib::utils
        ms::lib::variable_services
        silibs::idsw
        silibs::platform
        silibs::kng_silibs_platform
        silibs::ras_arm
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::ddr::cli ALIAS ${TARGET})
