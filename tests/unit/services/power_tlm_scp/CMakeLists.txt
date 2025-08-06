#
#  Copyright (c) Microsoft Corporation. All rights reserved.
#
set(TARGET  kng.service.pwr_tlm_scp_test)

set(SOURCES
    pwr_tlm_scp_mock.c
    test_pwr_tlm_scp_service.cpp
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    ms::service::pwr_tlm_scp
    fwlib.lib.status
    ms::cmocka::mocks
    ms::cmocka::testrunner
    ms::lib::error_handler
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:kng.service.pwr_tlm_scp,INCLUDE_DIRECTORIES>)
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:kng.service.pwr_tlm_mcp.cmpnt.in_band_tlm,INCLUDE_DIRECTORIES>)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    _tx_time_get
    _txe_queue_create
    _txe_queue_send
    _txe_queue_receive
    _txe_queue_flush
    _txe_block_pool_create
    _txe_block_release
    mts_client_register
    mts_is_primary_instance
    mts_client_send_new_trp_msg
    mts_client_forward_trp_msg
    mts_client_send_trp_response
    mts_client_flush_incoming_queue
    mts_client_send_dcp_notification
    mts_get_this_die_id
    mts_get_this_core_id
    FpFwAssertWithArgs
)