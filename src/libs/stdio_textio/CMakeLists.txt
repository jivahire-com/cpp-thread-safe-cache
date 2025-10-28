set(TARGET stdio_textio)

set(SRC
    ./stdio_textio.c
)

set(LIBS
        PRIVATE
            fwlib.driver.interface.text_io
            fwlib.lib.utils
            ms::driver::textio::pl011
            ms::lib::crash_dump
            silibs::common::lib_uart_pl011
)

add_library(${TARGET} STATIC ${SRC})
target_link_libraries(${TARGET} ${LIBS})
target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::stdio_textio ALIAS ${TARGET})
