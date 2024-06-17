set(TARGET ddrss.test)

set(SOURCES
    ddrss_mocks.c
    test_ddrss.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.utils
        ms::cmocka::mocks
        ms::cmocka::testrunner
        ms::lib::ddrss_init
        silibs::atu
        silibs::ddrss
        silibs::idsw
        silibs::kng_silibs_platform
        silibs::ras_arm
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    ddrss_init
    atu_map
    atu_unmap
)
