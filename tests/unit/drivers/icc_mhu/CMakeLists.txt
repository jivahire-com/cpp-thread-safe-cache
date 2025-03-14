set(TARGET kng.driver.icc_mhu_trans.test)

set(SOURCES
    test_icc_mhu_trans.cpp
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    kng.driver.icc_mhu_trans
    fwlib.lib.assert
    fwlib.dfwk
    fwlib.lib.interface.platform.errorhandler
    fwlib.lib.interface.platform.interrupts
    fwlib.driver.interface.iccTransport
    fwlib.platform.dfwkhost.threadx
    fwlib.lib.utils
    ms::cmocka::mocks
    ms::cmocka::testrunner
    ms::lib::debug
    ms::lib::error_handler
    ms::lib::arm_intrinsic
    ms::lib::register_headers
    ms::mocks::threadx
    silibs::icc_mhu
    silibs::idhw
    silibs::mhuv3
    silibs::kng_silibs_platform
    silibs_platform
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:kng.driver.icc_mhu_trans,INCLUDE_DIRECTORIES>)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    DfwkAsyncRequestComplete
    DfwkDeviceInitialize
    DfwkQueueEnqueueRequest
    FPFwCoreInterruptRegisterCallback
    FPFwCoreInterruptEnableVector
    FPFwCoreInterruptDisableVector
    fpfw_timer_create
    fpfw_timer_enable
    fpfw_timer_reset
    icc_mhu_clear_channel
    icc_mhu_get_packet
    icc_mhu_send_packet
    icc_mhu_check_packet_pending
)
