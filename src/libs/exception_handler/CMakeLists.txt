# Exception handler
set(TARGET kng.lib.exception_handler)

set (SOURCES
    src/exception_handler.c
)

add_library(${TARGET}
    STATIC
        ${SOURCES}
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.assert
        ms::lib::cortex_m7::nvic
        ms::lib::crash_dump
        ms::lib::error_codes
        ms::lib::threadx
        silibs::cmsdk_wd
        ms::service::health_monitor
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::exception_handler ALIAS ${TARGET})
