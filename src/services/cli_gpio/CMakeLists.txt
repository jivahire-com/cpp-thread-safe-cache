# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET kng.service.gpio.cli)

add_library(${TARGET}
    STATIC
    gpio_cli_main.c
)

target_link_libraries(
    ${TARGET}
    PRIVATE
    fwlib.dfwk
    fwlib.service.cli
    ms::driver::gpio
    ms::lib::utils
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::gpio::cli ALIAS ${TARGET})
