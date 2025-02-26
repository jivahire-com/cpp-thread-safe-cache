set(TARGET cxl.test)

set(SOURCES
    test_cxl.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.cfg_mgr
        fwlib.lib.utils
        ms::cmocka::testrunner
        ms::lib::cxl
        silibs::idsw_kng
        silibs::kng_silibs_platform
        silibs_platform
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    config_get_cxl_params_die0
    config_get_cxl_params_die1
)