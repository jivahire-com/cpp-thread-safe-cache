set(TARGET accel_intr_client.test)

# Compile test
add_executable(${TARGET}
    accel_intr_client_test.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::testrunner
        ms::lib::threadx
        ms::mocks::event_trace
        ms::mocks::threadx
        ms::service::accel_intr_client
        ms::service::accel_intr_service
    PUBLIC
        fwlib.lib.assert
        fwlib.dfwk
        fwlib.lib.linkedlist
        fwlib.lib.utils
 )

target_compile_definitions(${TARGET} PRIVATE UNIT_TEST)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    DfwkAsyncRequestSetCompletionRoutine
    DfwkInterfaceSendAsync
    DfwkAsyncRequestInititalize
    DfwkClientInterfaceOpen
)

target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)