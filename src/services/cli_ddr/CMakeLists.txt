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
)

target_link_libraries(
    ${TARGET}
        PRIVATE
        fwlib.cli
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::ddr::cli ALIAS ${TARGET})
