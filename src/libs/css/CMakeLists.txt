set(TARGET kng.lib.css)

add_library(${TARGET} 
    STATIC 
        css.c
)

target_link_libraries(${TARGET} 
    PRIVATE
    fwlib.assert
    atu
    clocks_sequence
    kng_silibs_platform
    ppu_v1
    silibs_platform
    tower_system_control
)

target_include_directories(${TARGET}
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
        ${REPO_ROOT_DIR}/.externs/azpkg/kingsgate_register_header.0.9.3/ap_addr_map/registerheader
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::css ALIAS ${TARGET})
