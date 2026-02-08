# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET etc_etd_init.test)

set(SOURCES
    test_etc_etd_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/etc_etd_init.c
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/et_mts_clnt_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.service.mts
        ms::cmocka::testrunner
        ms::lib::crash_dump
        ms::lib::threadx
        ms::mocks::event_trace
        ms::service::event_trace::collector
        ms::service::event_trace::decoder
        ms::service::event_trace::etc_etd
        ms::service::event_trace::et_mts_clnt
        ms::service::event_trace::relay
        silibs::idsw_kng
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# mock the functions needed
mock_target(${TARGET}
    etc_initialize
    etd_initialize
    mts_client_register

    _txe_queue_create
    _txe_block_pool_create
    _txe_thread_create

    crash_dump_register_address32
    crash_dump_bug_check
    crash_dump_register_pre_dump_callback

    idsw_get_cpu_type

    FPFwETControllerFlushBuffer
)
