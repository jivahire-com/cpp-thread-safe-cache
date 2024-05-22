set(TARGET vab_lib.test)

set(SOURCES
    vab_mocks.c
    vab_unit_tests.cpp
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
    fwlib.utils
    ms::cmocka::mocks
    ms::cmocka::testrunner
    ms::lib::error_handler
    ms::lib::vab
    silibs::atu
    silibs::kng_silibs_platform
    silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    atu_map
    atu_unmap
    configure_vab_system_addr_map
    deassert_pcr_reset
    tower_configure_vab_apu
    vab_pcr_init
)
