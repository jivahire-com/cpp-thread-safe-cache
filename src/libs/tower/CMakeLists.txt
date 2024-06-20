set(TARGET kng.lib.tower)

add_library(${TARGET}
    STATIC
        tower.c
)

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.assert
    silibs_platform
    ms::lib::register_headers
    silibs::atu
    silibs::kng_silibs_platform
    silibs::tower_sdmss
    silibs::tower_sequence
    idsw
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::tower ALIAS ${TARGET})
