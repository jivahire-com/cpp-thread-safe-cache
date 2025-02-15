set(TARGET kng.lib.gtimer)

add_library(${TARGET}
    STATIC
        gtimer.c
)

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.status
    fwlib.lib.tmr_queue
    fwlib.lib.interface.platform.interrupts
    silibs_platform
    ms::lib::crash_dump
    ms::lib::error_codes
    ms::lib::register_headers
    ms::lib::scp_interrupts
    silibs::gtimer
    silibs::idsw_kng
    silibs::platform
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::gtimer ALIAS ${TARGET})
