# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET icc_cli)

set(SRC
    src/icc_cli.c
    src/icc_hsp_mbox_cli.c
    src/icc_large_fifo_mbox_cli.c
    src/icc_mhu_cli.c
    src/icc_rmss_d2d_mbox_cli.c
)

set(LIBS
        PRIVATE
            fwlib.service.iccBase
            fwlib.service.icc_dispatcher
            fwlib.service.cli
            fwlib.lib.utils
            fwlib.lib.mboxPrimitives
            fwlib.lib.interface.platform.interrupts
            ms::driver::icc_mhu_trans
            ms::lib::crash_dump
            ms::lib::icc_platform_defines
            ms::lib::interface::mhu_icc_payloads
            ms::lib::hsp_firmware_headers
            ms::lib::utils		
            silibs::kng_silibs_platform
            silibs::platform
            silibs::icc_mhu
            silibs::idhw
            silibs::idsw
            silibs::idsw_kng
            silibs::spi_bridge
)

add_library(${TARGET} STATIC ${SRC})
target_link_libraries(${TARGET} ${LIBS})
target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:fwlib.service.iccBase,INCLUDE_DIRECTORIES>)

lint_target(${TARGET})
format_target(${TARGET})

add_library(ms::lib::icc::cli ALIAS ${TARGET})
