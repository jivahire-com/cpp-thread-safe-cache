set (TARGET kng.lib.spi)

set (SOURCES 
        src/spi_ctrl_bridge.c
)

add_library(${TARGET}
    STATIC
        ${SOURCES}
)

target_link_libraries(
    ${TARGET}
        PUBLIC
            silibs::spi_bridge
            silibs::kng_silibs_platform
        PRIVATE
            fwlib.lib.interface.platform.dbgprint
            ms::lib::cortex_m7::nvic           
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::spi ALIAS ${TARGET})
