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
            fwlib.lib.mboxPrimitives
            ms::lib::hsp_firmware_headers

        PUBLIC
            fwlib.lib.assert
            fwlib.platform.dfwkhost.threadx
            fwlib.lib.interface.platform.threadx
            fwlib.lib.utils
            fwlib.lib.mboxPrimitives
            ms::lib::hsp_firmware_headers
            ms::lib::mesh
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::mesh::cli ALIAS ${TARGET})
