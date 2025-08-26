set(TARGET service.event_trace.relay.test)

set(SOURCES
    event_trace_relay_test.cpp
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.lib.utils
    fwlib.service.iccBase
    ms::cmocka::testrunner
    ms::lib::error_handler
    ms::lib::threadx
    ms::mocks::threadx
    ms::service::event_trace::relay
    silibs::idsw_kng
)

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:ms::service::event_trace::relay,INCLUDE_DIRECTORIES>)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")


lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    FpFwLockAcquire
    FpFwLockRelease
    
    _txe_block_pool_create
    _txe_block_release
    _txe_queue_create
    _txe_thread_create
    _txe_queue_send
    _txe_queue_receive
    _txe_event_flags_create
    _txe_event_flags_get
    _txe_queue_info_get
    
    fpfw_icc_base_recv
    fpfw_icc_base_send_sync
    
    atu_svc_accel_atu_addr
    
    mts_client_register
    mts_client_send_trp_response
    mts_client_send_new_trp_msg

    mts_get_this_die_id
    mts_get_this_core_id
    transfer_rly_is_primary_node
)
