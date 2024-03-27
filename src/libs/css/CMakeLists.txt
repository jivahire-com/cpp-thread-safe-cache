enabled_on(arm-eabi-aarch)

set(TARGET kng.lib.css)

add_library(${TARGET} 
    STATIC 
        css.c
)

target_link_libraries(${TARGET} 
    PRIVATE
    fwlib.assert
    clocks_sequence
    ppu_v1
    silibs_platform
    kng_silibs_platform        
)

target_include_directories(${TARGET}
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::css ALIAS ${TARGET})
