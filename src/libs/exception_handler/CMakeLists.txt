# SCP Exception handler
set(TARGET kng.lib.exception_handler.scp)

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
        ms::lib::crash_dump::scp
        ms::lib::error_codes
        ms::lib::threadx
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::exception_handler::scp ALIAS ${TARGET})


# SCP Exception handler
set(TARGET kng.lib.exception_handler.mcp)

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
        ms::lib::crash_dump::mcp
        ms::lib::error_codes
        ms::lib::threadx
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::exception_handler::mcp ALIAS ${TARGET})
