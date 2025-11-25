# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET kng.service.ap_advlog_pldm.cli)

# Add target for the service
add_library(${TARGET}
        STATIC
        cli_ap_advlog_pldm.c
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
            fwlib.lib.interface.platform.dbgprint
            fwlib.service.cli
            silibs::kng_silibs_platform
            silibs::platform
            ms::lib::utils
        PUBLIC
            silibs::idsw_kng
            ms::service::ap_advlog_pldm
)

add_library(ms::service::ap_advlog_pldm::cli ALIAS ${TARGET})

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

