set(TARGET kng.service.health_monitor.cli)

add_library(${TARGET}
    STATIC
    hm_cli_main.c
)

target_link_libraries(
    ${TARGET}
    PRIVATE
    fwlib.dfwk
    fwlib.service.cli
    fwlib.lib.status
    silibs_platform
    ms::service::health_monitor
    silibs::semaphore
    silibs::icc_mhu
    ms::lib::crash_dump
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
