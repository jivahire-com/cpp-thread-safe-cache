# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET kng.lib.mesh)

add_library(${TARGET}
    STATIC
        src/mesh.c
        src/mesh_error_handler.c
)

target_link_libraries(${TARGET}
    PUBLIC
    fwlib.lib.interface.platform.dbgprint
    ms::service::health_monitor
    PRIVATE
    fwlib.lib.cfg_mgr
    fwlib.lib.interface.platform.interrupts
    fwlib.service.iccBase
    silibs_platform
    ms::lib::crash_dump
    ms::lib::d2d_cntr_sync
    ms::lib::error_codes
    ms::lib::hsp_firmware_headers
    ms::lib::register_headers
    ms::lib::scp_interrupts
    ms::lib::system_info
    ms::lib::variable_services
    ms::service::health_monitor
    silibs::atu
    ms::lib::i3c_controller
    silibs::cml
    silibs::cmn800
    silibs::cmn800_skus
    silibs::cmn800_sequence
    silibs::cmn800_ras_sequence
    silibs::ddr_i3c
    silibs::idhw
    silibs::idsw
    silibs::idsw_kng
    silibs::kng_silibs_platform
    silibs::ppu_v1
    silibs::ras_arm
    silibs::spi_bridge
)

target_include_directories(${TARGET}
    PRIVATE
        $<TARGET_PROPERTY:ms::service::health_monitor,INCLUDE_DIRECTORIES>
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::mesh ALIAS ${TARGET})
