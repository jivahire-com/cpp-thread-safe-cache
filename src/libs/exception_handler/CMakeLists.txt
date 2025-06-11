# Exception handler for SCP
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
        ms::lib::crash_dump
        ms::lib::error_codes
        ms::lib::error_domain::scp
        ms::lib::scp_interrupts
        ms::lib::threadx
        ms::service::health_monitor
        silibs::atu
        silibs::cmsdk_wd
        silibs::idsw_kng
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_compile_definitions(
    ${TARGET}
    PRIVATE
       SCP_RUNTIME_INIT
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::exception_handler::scp ALIAS ${TARGET})

# Exception handler for SCP
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
        ms::lib::crash_dump
        ms::lib::error_codes
        ms::lib::error_domain::mcp
        ms::lib::mcp_interrupts
        ms::lib::threadx
        ms::service::health_monitor
        silibs::atu
        silibs::cmsdk_wd
        silibs::idsw_kng
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_compile_definitions(
    ${TARGET}
    PRIVATE
       MCP_RUNTIME_INIT
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::exception_handler::mcp ALIAS ${TARGET})