set (TARGET kng.lib.bandgap)

set (SOURCES 
        src/bandgap.c
)

add_library(${TARGET}
    STATIC
        ${SOURCES}
)

target_link_libraries(
    ${TARGET}
        PUBLIC
            fwlib.lib.status
        PRIVATE
            fwlib.lib.assert
            fwlib.lib.interface.platform.dbgprint
            ms::lib::cortex_m7::nvic
            ms::lib::scp_interrupts
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::bandgap ALIAS ${TARGET})
