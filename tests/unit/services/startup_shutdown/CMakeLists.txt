set(TARGET sos.test)

# Compile test
add_executable(${TARGET}
    startup_shutdown_interface_test.cpp
    startup_shutdown_scp_test.cpp
    startup_shutdown_test.cpp
    startup_shutdown_thread_test.cpp
    startup_shutdown_remote_sync_test.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::testrunner
        ms::lib::threadx
        ms::lib::error_codes
        ms::mocks::event_trace
        ms::mocks::threadx
        ms::service::scp_startup_shutdown
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
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

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
    sos_queue_start_phase
    sos_thread_init
    ssi_shutdown_quiesce
    ssi_startup_stage_complete
    ssi_startup_stage_start
    _txe_event_flags_create
    _txe_event_flags_get
    _txe_event_flags_set
    _txe_queue_create
    _txe_queue_send
    _txe_thread_create
    idhw_is_single_die_boot_en
    idsw_get_die_id
    mscp_exp_spi_synchronize_dies
    FpFwLockAcquire
    FpFwLockRelease
    fpfw_icc_base_send
)

target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)