set(TARGET kng.service.health_monitor.cli)

add_library(${TARGET}
    STATIC
    hm_cli_main.c
)

target_link_libraries(
    ${TARGET}
    PRIVATE
    fwlib.dfwk
    fwlib.lib.status
    fwlib.service.cli
    ms::lib::crash_dump
    ms::lib::utils
    ms::service::health_monitor
    silibs::icc_mhu
    silibs_platform
    silibs::semaphore
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(ms::service::health_monitor::cli ALIAS ${TARGET})

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
