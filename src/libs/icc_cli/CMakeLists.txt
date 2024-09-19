set(TARGET icc_cli)

set(SRC
    src/icc_cli.c
    src/icc_hsp_mbox_cli.c
    src/icc_rmss_d2d_mbox_cli.c
    src/status_decoder.c
)

set(LIBS
        PRIVATE
            fwlib.lib.assert
            fwlib.service.iccBase
            fwlib.service.icc_dispatcher
            fwlib.service.cli
            fwlib.lib.utils
            ms::driver::icc_mhu_trans
            ms::lib::icc_platform_defines
            kng.lib.hsp_firmware_headers
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

lint_target(${TARGET})
format_target(${TARGET})

add_library(ms::lib::icc::cli ALIAS ${TARGET})
