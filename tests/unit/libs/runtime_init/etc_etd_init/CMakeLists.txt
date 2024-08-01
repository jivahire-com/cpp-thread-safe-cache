set(TARGET etc_etd_init.test)

set(SOURCES
    test_etc_etd_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/etc_etd_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        ms::build_data
        ms::cmocka::testrunner
        ms::lib::threadx
        ms::service::event_trace::collector
        ms::service::event_trace::decoder
        ms::mocks::event_trace
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

# mock the functions needed
mock_target(${TARGET}
    etc_initialize
    etd_initialize
)
