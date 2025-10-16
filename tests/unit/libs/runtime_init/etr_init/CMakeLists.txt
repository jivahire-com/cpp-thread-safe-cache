set(TARGET etr_init.test)

set(SOURCES
    test_etr_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/etr_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.service.mts
        fwlib.lib.utils
        ms::cmocka::testrunner
        ms::lib::telemetry::fuses
        ms::lib::threadx
        ms::service::event_trace::relay
        silibs::idsw
        silibs::idsw_kng
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# mock the functions needed
mock_target(${TARGET}
    etr_initialize
    idsw_get_die_id
    tlm_fuses_get_ecid
)
