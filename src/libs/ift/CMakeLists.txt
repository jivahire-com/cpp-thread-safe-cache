set(TARGET kng.lib.ift)

add_library(${TARGET}
    STATIC
        src/ift_dfwk.c
        src/ift_icc.c
        src/ift.c
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.interface.platform.dbgprint
        ms::client_interface::startup_shutdown
        ms::lib::cortex_m7::mpu
        ms::cmsis::interface
        ms::lib::crash_dump
        ms::lib::fuse_artifacts_lib
        ms::lib::register_headers
        ms::lib::system_info
        ms::lib::utils
        silibs::fuse_common
        silibs::idsw_kng
        silibs::idsw
        silibs::spi_bridge
    PUBLIC
        fwlib.lib.status
        fwlib.service.iccBase
        ms::lib::hsp_firmware_headers
        ms::lib::icc_platform_defines
        ms::service::atu_service
        silibs::ift
        silibs::kng_silibs_platform
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::ift ALIAS ${TARGET})
