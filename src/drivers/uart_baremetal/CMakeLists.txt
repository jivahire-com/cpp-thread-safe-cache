enabled_on(arm-eabi-aarch)

set(TARGET ms_uart_driver)

# Add target for the driver
add_library(
    ${TARGET} 
        STATIC
            uart.c
)

target_include_directories(
    ${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::driver::uart ALIAS ${TARGET})
