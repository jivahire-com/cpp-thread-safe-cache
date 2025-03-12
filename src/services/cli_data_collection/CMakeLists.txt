set(TARGET kng.service.dcs.cli)

add_library(${TARGET}
    STATIC
        dcs_cli_service.c
)

target_link_libraries(
    ${TARGET}
        PRIVATE
        fwlib.service.cli
        ms::lib::error_codes
        ms::service::dcs
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# access private headers for cli
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:ms::service::dcs,INCLUDE_DIRECTORIES>)

add_library(ms::service::dcs::cli ALIAS ${TARGET})

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
