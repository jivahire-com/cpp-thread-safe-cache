# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET exception_handler.test)

set(SOURCES
    test_exception_handler.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::testrunner
        ms::lib::cortex_m7::nvic
        ms::lib::error_codes
        ms::lib::error_domain::scp
        ms::lib::exception_handler::scp
        ms::lib::threadx
        ms::lib::crash_dump
        silibs::atu
)

target_include_directories(${TARGET}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/libs/exception_handler/src
)

target_compile_definitions(
    ${TARGET}
    PRIVATE
       SCP_RUNTIME_INIT
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# mock the functions needed
mock_target(${TARGET}
    crash_dump_bug_check
    crash_dump_handler
    crash_dump_bug_check_initiated_dump
    nvic_set_isr_fault
    wdog_cmsdk_apb_disable
    wdog_cmsdk_apb_lock_unlock
    _tx_thread_stack_error_notify
    hm_submit_cper_cd_state
    get_arsm_ecc_atu_entry_die
    get_rsm_ecc_atu_entry_die
    atu_map
    atu_unmap
    atu_find_map
    mmio_read32
    mmio_write32
    mmio_update32
)
