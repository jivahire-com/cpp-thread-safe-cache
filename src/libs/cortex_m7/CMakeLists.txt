enabled_on(arm-eabi-aarch)

set(TARGET kng.lib.cortex_m7)

add_library(${TARGET} 
    STATIC 
        cortex_m7.c
)

target_link_libraries(${TARGET} 
    PRIVATE
        
)

target_include_directories(${TARGET}
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::cortex_m7 ALIAS ${TARGET})
