set(TARGET exception_handler_init.test)

set(SOURCES
    test_exception_handler_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/exception_handler_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.assert
        fwlib.lib.init
        fwlib.lib.utils
        ms::cmocka::testrunner
        ms::lib::cortex_m7::nvic
        ms::lib::exception_handler
        ms::lib::error_codes
        ms::lib::threadx
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# mock the functions needed
mock_target(${TARGET}
    nvic_set_isr_fault
    _tx_thread_stack_error_notify
)
