# Copyright (c) Microsoft Corporation. All rights reserved.

enabled_on(arm-eabi-aarch)

set(TARGET cli_accel_int)

# Add target for the service
add_library(
    ${TARGET}
        STATIC
            cli_accel_int.c
            cli_accel_int_events.c
)

target_include_directories(
    ${TARGET}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(
    ${TARGET}
        PRIVATE
            fwlib.service.cli
            ms::lib::accel_intr
            ms::lib::cortex_m7::nvic
            ms::lib::event_trace::trace
            ms::lib::register_headers
            ms::lib::threadx
            ms::lib::utils
            silibs::accelip_ss_init
            silibs::kng_silibs_platform
            silibs::platform
            silibs::sdm_command_desc
            silibs::sdm_init
        PUBLIC
            silibs::atu
            silibs::kng_silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::cli_accel_int ALIAS ${TARGET})
