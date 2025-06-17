set(TARGET kng.service.gpio.cli)

add_library(${TARGET}
    STATIC
    gpio_cli_main.c
)

target_link_libraries(
    ${TARGET}
    PRIVATE
    fwlib.dfwk
    fwlib.lib.cfg_mgr
    fwlib.service.cli
    ms::driver::gpio
    silibs::idsw_kng
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(ms::service::gpio::cli ALIAS ${TARGET})

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
