#
# Copyright (c) Microsoft Corporation. All rights reserved.
#

set(TARGET utc_sync.utc_sync_manager.test)

set(SOURCES
    test_utc_sync_manager_mctp.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.mctp
        fwlib.lib.utils
        fwlib.service.mts
        fwlib.service.utc_sync.manager
        ms::cmocka::mocks
        ms::cmocka::testrunner
        ms::lib::error_handler
        ms::lib::gtimer
        ms::lib::threadx
        ms::lib::utc_sync_manager
        silibs::idsw
        silibs::idsw_kng
        silibs_platform
)

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:ms::lib::utc_sync_manager,INCLUDE_DIRECTORIES>)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    fpfw_mctp_send_msg
    fpfw_mctp_recv_msg
    gtimer_prodfw_get_counter
    gtimer_prodfw_get_frequency
    utc_sync_manager_notify_timestamp_ready
    _txe_semaphore_create
    _txe_semaphore_get
    _txe_semaphore_put
)
