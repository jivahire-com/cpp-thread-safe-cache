set(TARGET kng.lib.ioss_init)

add_library(${TARGET} 
    STATIC 
        ioss_init.c
)

target_link_libraries(${TARGET} 
    PRIVATE
        kng_silibs_platform
        silibs_platform
        fwlib.assert
        ppu_v1
        atu
        pcr_ioss
        tower_ioss
        idsw
)

target_include_directories(${TARGET}
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::ioss_init ALIAS ${TARGET})