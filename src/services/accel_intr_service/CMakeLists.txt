# define target for Accel Interrupt Service
set(TARGET kng.service.accel_intr_service)

add_library(${TARGET}
    STATIC
        src/accel_intr_service.c
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(${TARGET}
    PUBLIC
        ms::lib::utils
        ms::lib::accelerator_ip
        silibs::kng_silibs_platform
    PRIVATE
        fwlib.dfwk
        fwlib.lib.interface.platform.errorhandler
        fwlib.lib.interface.platform.threadx
        fwlib.lib.interface.platform.timers
        fwlib.lib.platform.impl.timers.threadx
        fwlib.lib.status
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::accel_intr_service ALIAS ${TARGET})
