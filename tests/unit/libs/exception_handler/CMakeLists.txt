set(TARGET exception_handler.test)

set(SOURCES
    test_exception_handler.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::testrunner
        ms::lib::cortex_m7::nvic
        ms::lib::error_codes
        ms::lib::exception_handler
        ms::lib::threadx
        ms::lib::crash_dump
)

target_include_directories(${TARGET}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/libs/exception_handler/src
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# mock the functions needed
mock_target(${TARGET}
    crash_dump_bug_check
    crash_dump_handler
    crash_dump_bug_check_initiated_dump
    nvic_set_isr_fault
    _tx_thread_stack_error_notify
)
