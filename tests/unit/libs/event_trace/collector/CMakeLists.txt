set(TARGET event_trace_collector.test)

set(SOURCES
    event_trace_collector.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::testrunner
        ms::lib::error_handler
        ms::lib::event_trace::collector
        ms::lib::threadx
        ms::mocks::threadx
        ms::mocks::event_trace
        fwlib.utils
)

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:ms::lib::event_trace::collector,INCLUDE_DIRECTORIES>)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.interface.win32")


lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
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
