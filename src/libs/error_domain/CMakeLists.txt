#
# Setup the SCP error domain
#

set(TARGET kng.lib.error_domain.scp)

set (SOURCES 
        src/error_domain_scp.c
        src/error_domain_shared.c
        src/error_domain_pex.c
        src/error_injection_scp.c
        src/exception_trigger.c
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
            fwlib.lib.interface.platform.dbgprint
            fwlib.lib.init
            ms::lib::arm_intrinsic
            ms::lib::cortex_m7::nvic
            ms::lib::crash_dump
            ms::lib::error_codes
            ms::lib::scp_interrupts
            ms::service::health_monitor
            ms::service::power
            silibs::atu
            silibs::cmsdk_wd
            silibs::idhw
            ms::lib::pex_rng
            silibs::idsw
            silibs::idsw_kng
            silibs::crypto::rng
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
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
        src/error_domain_mcp.c
        src/error_domain_shared.c
        src/error_injection_mcp.c
        src/exception_trigger.c
)

add_library(${TARGET}
    STATIC
        ${SOURCES}
)

target_link_libraries(
    ${TARGET}
        PUBLIC
            fwlib.lib.interface.platform.dbgprint
            ms::lib::cortex_m7::nvic
            ms::lib::mcp_interrupts
            fwlib.lib.status
        PRIVATE
            fwlib.lib.assert
            fwlib.service.iccBase
            ms::lib::arm_intrinsic
            ms::lib::crash_dump
            ms::lib::error_codes
            ms::lib::icc_platform_defines
            ms::service::health_monitor
            silibs::atu
            silibs::icc_mhu
            silibs::cmsdk_wd
            silibs::idhw
            silibs::idsw_kng
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src

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