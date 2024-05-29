set(TARGET ioss_lib.test)

set(SOURCES
    ioss_mocks.c
    ioss_unit_tests.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::testrunner
        ms::cmocka::mocks
        ms::lib::ioss_init
        ms::lib::error_handler
        silibs_platform
        silibs::kng_silibs_platform
        fwlib.assert
        ppu_v1
        atu
        pcr_ioss
        fwlib.utils
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    atu_map
    tower_configure_ioss_apu
    configure_ioss_system_addr_map
    program_ioss_pcr_usb_reset
    program_ioss_pcr_clock_mux
    atu_unmap
)