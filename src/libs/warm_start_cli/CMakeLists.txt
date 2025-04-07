set(TARGET warm_start_cli)

set(SRC
    ./warm_start_cli.c)

add_library(${TARGET} STATIC ${SRC})

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../warm_start/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../warm_start/src

)

target_link_libraries(
    ${TARGET}
        PRIVATE
            fwlib.service.cli
            fwlib.lib.init
            ms::lib::warm_start
            ms::lib::crash_dump
            ms::lib::error_codes
            ms::service::scp_startup_shutdown
            kng.lib.hsp_firmware_headers
        PUBLIC
            fwlib.lib.assert
            fwlib.dfwk
            fwlib.lib.utils
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::warm_start::cli ALIAS ${TARGET})
