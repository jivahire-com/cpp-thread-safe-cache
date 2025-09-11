set(TARGET variable_services_cli)

set(SRC
    variable_services_cli.c
)

set(LIBS
        PRIVATE
            fwlib.lib.assert
            fwlib.service.cli
            fwlib.lib.utils
            ms::lib::crash_dump
            ms::lib::error_codes
            ms::lib::variable_services
            silibs::kng_silibs_platform
            silibs::platform
            ms::lib::utils
)

add_library(${TARGET} STATIC ${SRC})
target_link_libraries(${TARGET} ${LIBS})
target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

lint_target(${TARGET})
format_target(${TARGET})

add_library(ms::lib::variable_services::cli ALIAS ${TARGET})
