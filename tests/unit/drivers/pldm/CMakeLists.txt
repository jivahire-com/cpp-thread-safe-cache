set(TARGET kng.driver.pldm.test)

set(SOURCES
    test_pldm_drv.cpp
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    ms::driver::pldm
    fwlib.dfwk
    fwlib.lib.assert
    fwlib.lib.pldm.protocol
    fwlib.platform.dfwkhost.threadx
    fwlib.service.pldm
    ms::cmocka::mocks
    ms::cmocka::testrunner
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    DfwkDeviceInitialize
    DfwkQueueInitialize
    DfwkInterfaceInitialize
    DfwkInterfaceOpen
    DfwkQueueDequeueRequest
    DfwkQueueEnqueueRequest
    DfwkAsyncRequestComplete
    DfwkAsyncRequestInitialize
    DfwkInterfaceSendAsync
    DfwkAsyncRequestSetCompletionRoutine
    fpfw_pldm_service_register_platform_event_ready_notification
    fpfw_pldm_service_raise_platform_event
)
