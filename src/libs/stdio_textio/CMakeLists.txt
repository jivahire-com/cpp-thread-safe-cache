enabled_on(arm-eabi-aarch)

set(TARGET stdio_textio)

set(SRC
    ./stdio_textio.c
)

set(LIBS
        PRIVATE
            fwlib.driver.interface.text_io
            fwlib.lib.utils
            ms::lib::crash_dump
)

add_library(${TARGET} STATIC ${SRC})
target_link_libraries(${TARGET} ${LIBS})
target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::stdio_textio ALIAS ${TARGET})
