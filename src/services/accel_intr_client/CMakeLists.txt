# Copyright (c) Microsoft Corporation. All rights reserved.

# define target for Accel Interrupt Client service
set(TARGET kng.service.accel_intr_client_service)

add_library(${TARGET}
    STATIC
        src/accel_intr_client.c
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(${TARGET}
    PUBLIC
        ms::lib::utils
        ms::lib::accelerator_ip
        ms::service::accel_intr_service
        silibs::kng_silibs_platform
        silibs::sdm_em_ext_interrupts
    PRIVATE
        fwlib.dfwk
        fwlib.lib.interface.platform.errorhandler
        fwlib.lib.interface.platform.threadx
        fwlib.lib.interface.platform.timers
        fwlib.lib.platform.impl.timers.threadx
        fwlib.lib.status
        ms::lib::cortex_m7::nvic
        ms::lib::error_codes
        ms::lib::event_trace::trace
        ms::lib::interrupts
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::accel_intr_client ALIAS ${TARGET})
