set(TARGET kng.lib.i3c_controller)

add_library(${TARGET}
    STATIC
        src/i3c_controller.c
)

target_link_libraries(${TARGET}
    PUBLIC
    silibs::lib_i3c
    silibs::spi_bridge

    PRIVATE
    fwlib.driver.mboxIccTransport
    fwlib.lib.assert
    fwlib.lib.interface.platform.interrupts
    fwlib.lib.mboxPrimitives
    fwlib.service.iccBase
    silibs_platform
    ms::lib::threadx
    ms::lib::crash_dump
    ms::lib::error_codes
    ms::lib::hsp_firmware_headers
    ms::lib::register_headers
    ms::lib::scp_interrupts
    ms::lib::system_info
    silibs::lib_i3c
    silibs::ddr_i3c
    silibs::idhw
    silibs::idsw
    silibs::idsw_kng
    silibs::kng_silibs_platform
    silibs::ppu_v1
    silibs::spi_bridge
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::i3c_controller ALIAS ${TARGET})
