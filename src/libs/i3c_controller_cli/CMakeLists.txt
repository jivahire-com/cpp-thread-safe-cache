# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET i3c_controller_cli)

set(SRC
    ./src/i3c_controller_cli.c)

add_library(${TARGET} STATIC ${SRC})

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(
    ${TARGET}
        PRIVATE
            fwlib.service.cli
            fwlib.lib.mboxPrimitives
            ms::lib::hsp_firmware_headers

        PUBLIC
            fwlib.lib.assert
            fwlib.platform.dfwkhost.threadx
            fwlib.lib.interface.platform.threadx
            fwlib.lib.utils
            fwlib.lib.mboxPrimitives
            ms::lib::ddr_manager
            ms::lib::hsp_firmware_headers
            ms::lib::i3c_controller
            ms::lib::utils
            silibs::lib_i3c
            silibs::ddr_i3c
            fwlib.lib.interface.platform.dbgprint
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::i3c_controller::cli ALIAS ${TARGET})
