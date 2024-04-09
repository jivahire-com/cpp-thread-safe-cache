set(TARGET css.test)

set(SOURCES
    test_css.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        atu
        clocks_sequence
        fwlib.init
        idhw
        idsw
        kng_silibs_platform
        ms::cmocka::testrunner
        ms::lib::css
        ppu_v1
        silibs_platform
        tower_system_control
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    atu_init
    clocks_sequence_css_pre_mesh_init
)
