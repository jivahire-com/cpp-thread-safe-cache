set(TARGET d2d_cntr_sync.test)

set(SOURCES
    test_d2d_cntr_sync.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.status
        fwlib.lib.utils
        ms::cmocka::testrunner
        silibs::d2d_counter_sync
        ms::lib::d2d_cntr_sync
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
    d2d_counter_sync_configure_d0_params
    d2d_counter_sync_configure_d1_params
    d2d_counter_sync_enable
)