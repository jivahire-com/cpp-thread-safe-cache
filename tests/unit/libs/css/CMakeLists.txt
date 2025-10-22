set(TARGET css.test)

set(SOURCES
    test_css.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_link_libraries(${TARGET}
    PRIVATE
        atu
        clocks_sequence
        fwlib.lib.init
        fwlib.lib.cfg_mgr
        idhw
        silibs::idsw
        silibs::idsw_kng
        kng_silibs_platform
        ms::cmocka::testrunner
        ms::lib::css
        ppu_v1
        silibs_platform
        tower_system_control
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# Mock the functions needed
mock_target(${TARGET}
    clocks_sequence_css_pre_mesh_init
)
