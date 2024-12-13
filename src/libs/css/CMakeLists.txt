set(TARGET kng.lib.css)

add_library(${TARGET}
    STATIC
        css.c
)

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.assert
    fwlib.lib.cfg_mgr
    ms::lib::register_headers
    silibs_platform
    silibs::atu
    silibs::clocks_sequence
    silibs::kng_silibs_platform
    silibs::ppu_v1
    silibs::tower_system_control
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::css ALIAS ${TARGET})
