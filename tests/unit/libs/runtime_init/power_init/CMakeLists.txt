set(TARGET power_init.test)

set(SOURCES
    test_power_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/power_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.platform.dfwkhost.threadx
        idhw
        ms::client_interface::startup_shutdown
        ms::cmocka::testrunner
        ms::lib::threadx
        ms::lib::utils
        ms::service::power
        ms::service::atu_service
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
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
    crash_dump_bug_check
    fpfw_init_get_handle
    idhw_is_single_die_boot_en
    idsw_get_platform_sdv
    power_init
    power_interface_init
    sos_register_ssi
    idsw_get_die_id
    pwr_avs_initialize
)
