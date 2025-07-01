# client interface code
set(TARGET kng.client_interface.startup_shutdown)

add_library(${TARGET}
    STATIC
    startup_shutdown_client_interface.c
)

target_link_libraries(${TARGET}
    PUBLIC
        ms::lib::error_codes
        ms::lib::system_info
        fwlib.lib.interface.platform.dbgprint
    PRIVATE
        fwlib.dfwk
        fwlib.lib.interface.platform.errorhandler
        kng.lib.hsp_firmware_headers
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::client_interface::startup_shutdown ALIAS kng.client_interface.startup_shutdown)


# scp/mcp service
set(STARTUP_COMMON_SOURCE
    startup_shutdown.c
    startup_shutdown_events.c
    startup_shutdown_ssi.c
    startup_shutdown_thread.c
    startup_shutdown_local_sync.c
    startup_shutdown_remote_sync.c
    startup_shutdown_core_comm.c
)

set(STARTUP_COMMON_LIBS
    fwlib.dfwk
    fwlib.lib.cfg_mgr
    fwlib.lib.interface.platform.errorhandler
    fwlib.lib.init
    ms::build_data
    ms::lib::arm_intrinsic
    ms::lib::threadx
    ms::lib::debug
    ms::lib::utils
    ms::lib::crash_dump
    ms::lib::event_trace::trace
    ms::lib::icc_platform_defines
    ms::lib::hsp_firmware_headers
    ms::driver::icc_mhu_trans
    ms::client_interface::startup_shutdown
    silibs::idhw
    silibs::idsw
    silibs::idsw_kng
    silibs::kng_silibs_platform
    silibs::spi_bridge
    ms::lib::hw_semaphore
)

# scp

set(TARGET kng.service.scp_startup_shutdown)

add_library(${TARGET}
    STATIC
        ${STARTUP_COMMON_SOURCE}
        startup_shutdown_scp.c
)

target_link_libraries(${TARGET}
    PUBLIC
        ms::lib::error_codes
    PRIVATE
        ${STARTUP_COMMON_LIBS}
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::scp_startup_shutdown ALIAS kng.service.scp_startup_shutdown)

# mcp

set(TARGET kng.service.mcp_startup_shutdown)

add_library(${TARGET}
    STATIC
        ${STARTUP_COMMON_SOURCE}
        startup_shutdown_mcp.c
)

target_link_libraries(${TARGET}
    PRIVATE
        ${STARTUP_COMMON_LIBS}
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::service::mcp_startup_shutdown ALIAS kng.service.mcp_startup_shutdown)
