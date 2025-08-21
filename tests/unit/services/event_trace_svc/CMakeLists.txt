# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET event_trace_service.test)

set(SOURCES
    event_trace_svc_test.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE 
        fwlib.lib.interface.platform.specialization.mts
        fwlib.lib.utils
        fwlib.service.mts
        ms::cmocka::mocks
        ms::cmocka::testrunner
        ms::lib::threadx
        ms::mocks::event_trace
        ms::mocks::threadx
        ms::service::event_trace::collector
        ms::service::event_trace::etc_etd
        ms::service::event_trace::et_mts_clnt
        ms::service::event_trace::relay
    PUBLIC
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    _txe_queue_receive
    _txe_block_release

    mts_get_this_die_id
    mts_get_this_core_id
    fpfw_crc32

    # get_etc_buffer_address
    # get_etc_buffer_size

    mts_client_send_trp_response
)