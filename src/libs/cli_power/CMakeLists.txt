set(TARGET cli_power)

# Add target for the service
add_library(
    ${TARGET}
        STATIC
        cli_power.c
)

target_include_directories(
    ${TARGET}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(
    ${TARGET}
        PRIVATE
        ms::build_data
        fwlib.service.cli
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::power::cli ALIAS ${TARGET})
