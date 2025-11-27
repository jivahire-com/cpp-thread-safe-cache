set(TARGET kng.service.accel_boot_notify_service)

set (SOURCES 
        src/accel_boot_notify.c
        src/accel_boot_notify_events.c
)

add_library(${TARGET}
    STATIC
        ${SOURCES}
)

target_link_libraries(${TARGET}
    PUBLIC
        ms::lib::error_codes
    PRIVATE
        ms::lib::accelerator_ip
        ms::lib::threadx
        silibs::cmsdk_wd
        silibs::kng_silibs_platform
        silibs::gpio
        fwlib.lib.interface.platform.errorhandler
        ms::lib::crash_dump
        ms::lib::error_codes
        ms::lib::event_trace::trace
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::accel_boot_notify ALIAS ${TARGET})