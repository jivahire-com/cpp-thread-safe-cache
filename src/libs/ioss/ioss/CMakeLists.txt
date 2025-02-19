set(TARGET kng.lib.ioss_ini)

add_library(${TARGET}
    STATIC
        ioss_ini.c
)

target_link_libraries(${TARGET}
    PRIVATE
        kng_silibs_platform
        silibs_platform
        fwlib.lib.assert
        silibs::atu
        silibs::idsw
        silibs::idsw_kng
        silibs::ioss_init
        silibs::usb_init
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::ioss_ini ALIAS ${TARGET})