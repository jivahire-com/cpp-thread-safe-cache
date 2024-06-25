set(TARGET tower.test)

set(SOURCES
    tower_mocks.c
    tower_unit_tests.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.utils
        ms::cmocka::mocks
        ms::cmocka::testrunner
        ms::lib::error_handler
        ms::lib::tower
        silibs::atu
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        silibs::kng_silibs_platform
        silibs::tower_sequence
        silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    tower_sequence_configure_towers
    atu_map
    atu_unmap
    idsw_get_platform_sdv
    tower_configure_sdmss_sam_with_isolation_disabled
)
