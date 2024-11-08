set(TARGET kng.driver.icc_mhu_trans.test)

set(SOURCES
    test_icc_mhu_trans.cpp
    icc_mhu_mock.c
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
    icc_mhu_send_message
    icc_mhu_get_message
    icc_mhu_check_message_received
    mhuv3_clr_mdbcw_status
    icc_mhu_trans_get_message
    fpfw_icc_transport_try_send_sync_req
    _txe_timer_activate
    _txe_timer_deactivate
    _txe_timer_create
    _txe_timer_info_get
    FPFwCoreInterruptRegisterCallback
    FPFwCoreInterruptEnableVector
    FPFwCoreInterruptDisableVector
    FpFwLockAcquire
    FpFwLockRelease
    DfwkAsyncRequestComplete
)
