set(TARGET kng.service.rhtlm_service.cli)

# Add target for the service
add_library(
    ${TARGET}
        STATIC
        cli_rhtlm.c
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
        PUBLIC
        silibs::ddrss
        ms::lib::ddr_manager
        ms::lib::ddrss_init
        PRIVATE
        fwlib.service.cli
        silibs::idsw
        silibs::platform
        silibs::kng_silibs_platform
        silibs::ras_arm
        ms::service::health_monitor
        ms::service::rhtlm_service
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::rhtlm_service::cli ALIAS ${TARGET})
