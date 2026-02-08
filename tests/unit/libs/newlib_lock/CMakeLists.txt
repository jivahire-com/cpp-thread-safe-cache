# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET newlib_lock.test)

# Compile test
add_executable(${TARGET} newlib_lock_test.cpp)

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::testrunner
	    ms::lib::newlib_lock
 )

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:ms::lib::newlib_lock,INCLUDE_DIRECTORIES>)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    FpFwLockInitialize
    FpFwLockAcquire
    FpFwLockRelease
    _txe_mutex_create
    _txe_mutex_delete
    _txe_mutex_get
    _txe_mutex_put
    _tx_thread_identify
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")