set(TARGET textio_pl011_test.test)

set(SOURCES
    textio_pl011_test.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.utils
        ms::cmocka::testrunner
        ms::driver::textio::pl011
        ms::lib::error_handler
        ms::lib::threadx
        ms::mocks::threadx
        silibs::common::lib_uart_pl011
)
# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:ms::driver::textio::pl011,INCLUDE_DIRECTORIES>)


# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    _tx_thread_interrupt_control
    _txe_timer_activate
    _txe_timer_create
    _txe_timer_deactivate
    uart_pl011_get_intr_status
    uart_pl011_read
    uart_pl011_read_byte
    uart_pl011_rx_fifo_is_empty
    uart_pl011_tx_fifo_is_full
    uart_pl011_write_byte
)
