# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET sos.test)

# Compile test
add_executable(${TARGET}
    startup_shutdown_interface_test.cpp
    startup_shutdown_scp_test.cpp
    startup_shutdown_test.cpp
    startup_shutdown_thread_test.cpp
    startup_shutdown_remote_sync_test.cpp
    startup_shutdown_local_sync_test.cpp
)

target_compile_definitions(kng.service.scp_startup_shutdown PRIVATE START_SHUTDOWN_TEST_OVERRIDE)

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::testrunner
        ms::lib::threadx
        ms::lib::error_codes
        ms::lib::system_info
        ms::driver::icc_mhu_trans
        ms::mocks::event_trace
        ms::mocks::threadx
        ms::service::scp_startup_shutdown
        kng.lib.hsp_firmware_headers
    PUBLIC
        fwlib.lib.assert
        fwlib.dfwk
        fwlib.lib.linkedlist
        fwlib.lib.utils
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
 )

# Get private includes for module under test
target_include_directories(${TARGET}
    PRIVATE
        $<TARGET_PROPERTY:ms::service::scp_startup_shutdown,INCLUDE_DIRECTORIES>
)

# Generic ports implmented within this project
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
    DfwkAsyncRequestComplete
    DfwkAsyncRequestInitialize
    DfwkAsyncRequestSetCompletionRoutine
    DfwkDeviceInitialize
    DfwkQueueInitialize
    DfwkInterfaceInitialize
    DfwkInterfaceSendAsync
    DfwkInterfaceSendSync
    DfwkInterfaceOpen
    FpFwListInitialize
    FpFwListInsertTail
    sos_core_boot_stages
    sos_core_boot_stage_count
    sos_queue_shutdown
    sos_queue_quiesce
    sos_queue_start_phase
    sos_thread_init
    sos_quiesce
    sos_shutdown
    ssi_shutdown
    ssi_quiesce
    ssi_startup_stage_complete
    ssi_startup_stage_start
    _tx_thread_sleep
    _txe_event_flags_create
    _txe_event_flags_get
    _txe_event_flags_set
    _txe_queue_create
    _txe_queue_send
    _txe_thread_create
    idhw_is_single_die_boot_en
    idsw_get_cpu_type
    idsw_get_die_id
    idsw_get_platform_sdv
    FpFwLockAcquire
    FpFwLockRelease
    fpfw_icc_base_send
    fpfw_icc_base_recv
    reset_complete_wait_forever
    wait_for_semaphore
    release_semaphore
    accel_is_isolation_enabled
    accel_core_suspend
    mmio_read32
    mmio_write32
    wdog_cmsdk_apb_disable
    wdog_cmsdk_apb_lock_unlock
)

target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)
