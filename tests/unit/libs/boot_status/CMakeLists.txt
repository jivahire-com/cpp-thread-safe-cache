set(TARGET boot_status.test)

set(SOURCES
    test_boot_status.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})
target_compile_options(${TARGET} PRIVATE -Wno-c99-designator -Wno-initializer-overrides)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.service.iccBase
        ms::cmocka::testrunner
        ms::lib::boot_status
        ms::lib::hsp_firmware_headers
        ms::lib::system_info
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    fpfw_icc_base_send_sync
    fpfw_icc_base_send
    system_info_is_hsp_present
    system_info_is_init_complete
    idsw_get_cpu_type
    idsw_get_die_id
    crash_dump_bug_check
)