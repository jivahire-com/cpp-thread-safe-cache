# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET mesh_cli)

set(SRC
    ./src/mesh_cli.c)

add_library(${TARGET} STATIC ${SRC})

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(
    ${TARGET}
        PRIVATE
            fwlib.service.cli
            fwlib.service.iccBase
            ms::lib::hsp_firmware_headers
            silibs_platform
            silibs::cmn800
            silibs::cmn800_ras_sequence
            silibs::kng_silibs_platform
            silibs::idhw

        PUBLIC
            fwlib.lib.assert
            fwlib.platform.dfwkhost.threadx
            fwlib.lib.interface.platform.threadx
            fwlib.lib.utils
            ms::lib::hsp_firmware_headers
            ms::lib::mesh
            ms::lib::utils
            fwlib.lib.interface.platform.dbgprint
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::mesh::cli ALIAS ${TARGET})
