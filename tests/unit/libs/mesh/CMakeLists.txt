set(TARGET mesh.test)

set(SOURCES
    test_mesh.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        cmn800
        cmn800_sequence
        fwlib.lib.init
        fwlib.lib.utils
        idsw
        kng_silibs_platform
        ms::cmocka::testrunner
        ms::lib::mesh
        silibs_platform
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    cmn800_sequence
    idhw_is_single_die_boot_en
    d2dss_sequence
    cmn800_sequence_svp_updates
)
