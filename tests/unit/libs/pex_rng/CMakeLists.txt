# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET pex_rng.test)

set(SOURCES
    test_pex_rng.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::testrunner
        ms::lib::pex_rng
        ms::lib::system_info
        ms::lib::register_headers
        silibs::crypto::rng
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
    rng_enable_r
    rng_disable_r
    rng_wait_for_rng_complete_r
    fpfw_init_get_handle
    DfwkDeviceInitialize
    DfwkInterfaceInitialize
    DfwkAsyncRequestInitialize
    DfwkQueueInitialize
    DfwkClientInterfaceOpen
    DfwkAsyncRequestComplete
    hm_submit_cper
)