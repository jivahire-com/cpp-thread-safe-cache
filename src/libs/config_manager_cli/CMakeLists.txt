set(TARGET config_manager_cli)

set(SRC
    ./src/config_manager_cli.c)

add_library(${TARGET} STATIC ${SRC})

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(
    ${TARGET}
        PRIVATE
            silibs::idhw
            silibs::idsw
            silibs::idsw_kng
            fwlib.service.cli
            fwlib.lib.assert
            fwlib.lib.utils
            ms::lib::config_manager
            kng_silibs_platform
            silibs::platform
            fwlib.lib.status
            fwlib.lib.spinlock
            ms::lib::error_codes
            ms::lib::crash_dump
            ms::lib::variable_services
            ms::lib::utils
            fwlib.lib.interface.platform.dbgprint
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::config_manager::cli ALIAS ${TARGET})
