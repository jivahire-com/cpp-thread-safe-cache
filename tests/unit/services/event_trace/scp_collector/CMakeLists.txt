set(TARGET scp_event_trace_collector.test)

set(SOURCES
    scp_collector.cpp
)

set(LIBS
    PRIVATE
        ms::cmocka::testrunner
        ms::lib::event_trace::collector
        ms::lib::threadx
        ms::mocks::event_trace
        ms::mocks::threadx
        ms::service::scp_etc
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    ${LIBS}
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.interface.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})
mock_target(${TARGET}
    _txe_queue_create
    _txe_semaphore_create
    _txe_thread_create
    _txe_queue_send
    _txe_queue_receive
    _txe_semaphore_get
    _txe_semaphore_put
    FPFwETDecoderInit
    FPFwETTraceInitialize
    FPFwETDecoderPrintEvent
    FPFwETControllerRecycleBuffer
)
