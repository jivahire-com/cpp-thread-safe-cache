set(TARGET kng.driver.gpio.test)

set(SOURCES
    dfwk_mock.cpp
    fpfw_mock.cpp
    gpio_lib_mock.cpp
    nvic_mock.cpp
    tx_mock.cpp
    test_gpio_driver.cpp
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    ms::driver::gpio

    fwlib.dfwk
    fwlib.lib.assert
    fwlib.platform.dfwkhost.threadx
    ms::cmocka::mocks
    ms::cmocka::testrunner
    ms::mocks::threadx
    ms::lib::interrupts
    ms::lib::scp_interrupts
    silibs::kng_silibs_platform
    silibs_platform
    ms::lib::cortex_m7::nvic
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    fpfw_init_get_handle
    FpFwLockAcquire
    FpFwLockRelease
    DfwkDeviceInitialize
    DfwkQueueInitialize
    DfwkInterfaceInitialize
    DfwkQueueDequeueRequest
    DfwkQueueEnqueueRequest
    DfwkAsyncRequestComplete
    DfwkScheduleQueueDispatch
    _txe_queue_create
    _txe_thread_create
    _txe_queue_receive
    _txe_queue_send
    nvic_irq_set_isr_with_param
    nvic_irq_clear_pending
    nvic_irq_enable
    nvic_get_current_irq
    gpio_get_interrupt_status
    gpio_get_input
    gpio_clear_interrupt_status
    thread_loop
)
