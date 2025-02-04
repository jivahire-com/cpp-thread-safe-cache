set(TARGET accel_atu_init.test)

set(SOURCES
    test_accel_atu_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/accelerator_ip_scp_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.utils
        atu
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        ms::cmocka::testrunner
        ms::lib::css
        silibs_platform
        ms::service::atu_service
        ms::lib::accelerator_ip
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

#mock the functions needed
mock_target(${TARGET}
    FpFwAssert
    crash_dump_bug_check
    idsw_get_die_id
    atu_map
)
