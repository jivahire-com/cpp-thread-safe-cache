set(TARGET kng.driver.icc_mhu_trans)

add_library(${TARGET}
    STATIC
    src/icc_mhu_trans_dfwk.c
    src/icc_mhu_trans_prim.c
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
        fwlib.lib.interface.platform.errorhandler
        fwlib.driver.interface.iccTransport
        fwlib.lib.utils
        fwlib.platform.dfwkhost.threadx
        fwlib.service.cli
        silibs::icc_mhu
        silibs::idhw
        silibs::mhuv3
        ms::lib::arm_intrinsic
        ms::lib::register_headers
    PUBLIC
        ms::lib::debug
        silibs_platform
)


add_library(ms::driver::icc_mhu_trans ALIAS ${TARGET})

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
