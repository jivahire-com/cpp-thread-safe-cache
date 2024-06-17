set(TARGET kng.lib.ddrss_init)

add_library(${TARGET}
    STATIC
        src/ddrss.c
)

target_link_libraries(${TARGET}
    PRIVATE
    ms::lib::register_headers
    silibs::platform
    kng_silibs_platform
    ddrss
    atu
    idsw
    silibs::ras_arm
    fwlib.lib.assert
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::ddrss_init ALIAS ${TARGET})
