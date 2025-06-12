set(TARGET  kng.service.pwr_tlm_mcp.cmpnt.exec.test)

set(SOURCES
    exec_mocks.c
    test_exec_cmpnt.cpp
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    kng.service.pwr_tlm_mcp.cmpnt.exec
    fwlib.lib.status
    ms::cmocka::mocks
    ms::cmocka::testrunner
    ms::mocks::threadx
    ms::lib::error_handler
    ms::service::sensor_fifo

)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

# Get private includes for module under test
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:kng.service.pwr_tlm_mcp,INCLUDE_DIRECTORIES>)
target_include_directories(${TARGET} PRIVATE $<TARGET_PROPERTY:kng.service.pwr_tlm_mcp.cmpnt.exec,INCLUDE_DIRECTORIES>)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    FpFwAssertWithArgs
    FpFwLockAcquire
    FpFwLockRelease
    _txe_thread_create
    _txe_timer_create
    _txe_event_flags_create
    _txe_event_flags_set
    _txe_timer_deactivate
    _txe_timer_activate
    _txe_timer_info_get
    _txe_timer_change
    gtimer_prodfw_get_counter
    gtimer_prodfw_get_frequency
)