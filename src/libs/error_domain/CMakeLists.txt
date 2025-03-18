#
# Setup the SCP error domain
#

set(TARGET kng.lib.error_domain.scp)

set (SOURCES 
        src/error_domain_register_scp.c
)

add_library(${TARGET}
    STATIC
        ${SOURCES}
)

target_link_libraries(
    ${TARGET}
        PUBLIC
            fwlib.lib.status
        PRIVATE
            fwlib.lib.assert
            ms::lib::crash_dump
            ms::lib::error_codes
            ms::service::health_monitor
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

add_library(ms::lib::error_domain::scp ALIAS ${TARGET})

#
# Setup the MCP error domain
#

set(TARGET kng.lib.error_domain.mcp)

set (SOURCES 
        src/error_domain_register_mcp.c
)

add_library(${TARGET}
    STATIC
        ${SOURCES}
)

target_link_libraries(
    ${TARGET}
        PUBLIC
            fwlib.lib.status
        PRIVATE
            fwlib.lib.assert
            fwlib.service.iccBase
            ms::lib::crash_dump
            ms::lib::error_codes
            ms::lib::icc_platform_defines
            ms::service::health_monitor
            silibs::icc_mhu
            
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

add_library(ms::lib::error_domain::mcp ALIAS ${TARGET})