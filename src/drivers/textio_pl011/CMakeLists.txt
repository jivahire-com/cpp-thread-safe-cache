
set(TARGET ms_textio_pl011_driver)

# Add target for the driver
add_library(${TARGET} 
    STATIC
        textio_pl011.c
)

target_include_directories(${TARGET} 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET} 
    PUBLIC 
        fwlib.dfwk
        fwlib.DriverInterface.TextIo
        silibs::common::lib_uart_pl011
        ms::lib::debug
        ms::lib::threadx
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::driver::textio::pl011 ALIAS ${TARGET})
