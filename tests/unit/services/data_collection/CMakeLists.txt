set(TARGET kng.service.dcs.test)

set(SOURCES
    test_dcs.cpp
    test_dcp_svc_client.cpp
    test_dcs_manifest.cpp
    test_tlm_relay.cpp
    dcs_mock.c
)


set(LIBS
    PRIVATE
        kng.service.dcs
        ms::lib::interface::in_band_telemetry_ddr
        ms::cmocka::mocks
        ms::cmocka::testrunner
        silibs::icc_mhu

)

# Compile test
add_executable(${TARGET} ${SOURCES})

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_link_libraries(${TARGET}
        ${LIBS}
)

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:kng.service.dcs,INCLUDE_DIRECTORIES>)


lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})
mock_target(${TARGET}
    FpFwAssertWithArgs
    _txe_thread_create
    _txe_queue_create
    _txe_queue_receive
    _txe_queue_send
    _txe_event_flags_create
    _txe_event_flags_set
    _txe_block_pool_create
    _txe_block_allocate
    _txe_block_release
    _tx_thread_sleep
    fpfw_icc_base_recv
    fpfw_icc_base_send_sync
)
