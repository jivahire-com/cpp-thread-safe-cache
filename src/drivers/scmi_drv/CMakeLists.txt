set(TARGET kng.driver.scmi_drv)

add_library(${TARGET}
    STATIC
    src/scmi_prim.c
)


target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.assert
        fwlib.dfwk
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.lib.status
        fwlib.service.cli
        silibs::icc_mhu
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
        ms::lib::arm_intrinsic
        ms::lib::register_headers
        ms::driver::icc_mhu_trans
        ms::service::ap_core
        ms::service::atu_service
        ms::client_interface::startup_shutdown
        ms::lib::crash_dump
        kng.lib.hsp_firmware_headers
    PUBLIC
        ms::cmsis::interface
        ms::lib::debug
        silibs_platform
)

add_library(ms::driver::scmi_drv ALIAS ${TARGET})

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
