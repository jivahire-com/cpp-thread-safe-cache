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
        fwlib.lib.utils
        fwlib.service.cli
        silibs::kng_silibs_platform
        ms::lib::register_headers		
        ms::driver::icc_mhu_trans	
	PUBLIC
        ms::lib::debug
        silibs_platform
)

add_library(ms::driver::scmi_drv ALIAS ${TARGET})

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
