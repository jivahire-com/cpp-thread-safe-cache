set(TARGET gtimer.test)

set(SOURCES
    test_gtimer.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.status
        fwlib.lib.tmr_queue
        fwlib.lib.utils
        ms::cmocka::testrunner
        ms::lib::interrupts
        ms::lib::gtimer
        ms::lib::register_headers
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    system_counter_init
    gtimer_init
    gtimer_enable_timer
    gtimer_set_timer
    gtimer_get_counter
    FPFwCoreInterruptEnableVector
    FPFwCoreInterruptDisableVector
    FPFwCoreInterruptRegisterCallback
    fpfw_tmr_queue_add_oneshot
    fpfw_tmr_queue_add_periodic
    fpfw_tmr_queue_expire
)