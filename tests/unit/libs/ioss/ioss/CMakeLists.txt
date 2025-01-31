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
        ms::lib::ioss_ini
        ms::lib::error_handler
        silibs_platform
        silibs::kng_silibs_platform
        silibs::ioss_init
        fwlib.lib.assert
        ppu_v1
        atu
        fwlib.lib.utils
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    atu_map
    ioss_init
    atu_unmap
)