# Copyright Microsoft Corporation

set(TARGET rtosmon_service.test)

set(SOURCES
    test_rtosmon_service.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_include_directories(
    ${TARGET} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.lib.status
        fwlib.lib.rtosmon
        ms::cmocka::testrunner
        ms::lib::register_headers
        silibs::idsw_kng
        silibs::platform
        ms::cmsis::interface
        ms::lib::utils
        ms::lib::systick_update
        ms::lib::threadx
        ms::service::m7_rtosmon_service
)

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
    idsw_get_cpu_type
    idsw_get_die_id
    rtosmon_init
    gtimer_prodfw_get_counter
    gtimer_prodfw_get_frequency
    _txe_thread_create
)
