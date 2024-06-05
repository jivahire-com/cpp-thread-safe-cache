set(TARGET kng.lib.ioss.usb)

add_library(${TARGET}
    STATIC
        usb.c
)

target_link_libraries(${TARGET}
    PRIVATE
        kng_silibs_platform
        silibs_platform
        fwlib.lib.assert
        ppu_v1
        atu
        ioss
        pcr_ioss
        tower_ioss
        usb_init
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::ioss::usb ALIAS ${TARGET})