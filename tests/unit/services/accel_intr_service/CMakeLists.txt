set(TARGET accel_intr_service.test)

# Compile test
add_executable(${TARGET}
    accel_intr_service_test.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::testrunner
        ms::lib::threadx
        ms::mocks::event_trace
        ms::mocks::threadx
        ms::service::accel_intr_service
    PUBLIC
        fwlib.dfwk
        fwlib.lib.assert
        fwlib.lib.init
        fwlib.lib.linkedlist
        fwlib.lib.utils
        fwlib.platform.dfwkhost.threadx
        silibs::sdm_command_desc
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
    DfwkInterfaceInitialize
    DfwkDeviceInitialize
    DfwkQueueInitialize
    DfwkAsyncRequestComplete
    FpFwAssert
    accel_intr_handle_fatal_intr_recvd
    accel_intr_handle_mbox_recvd
    accel_intr_handle_sdm_msg_recvd
)

target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)