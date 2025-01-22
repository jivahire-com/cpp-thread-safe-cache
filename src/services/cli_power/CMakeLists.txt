set(TARGET cli_power)

# Add target for the service
add_library(
    ${TARGET}
        STATIC
        cli_power.c
        cli_power_set.c
        cli_power_status.c
        cli_power_config.c
        cli_power_log.c
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
            ms::build_data
            ms::lib::corebits
            ms::lib::threadx
            ms::service::power
            silibs::kng_silibs_platform
            fwlib.dfwk
            fwlib.service.cli
            fwlib.lib.assert
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::power::cli ALIAS ${TARGET})
