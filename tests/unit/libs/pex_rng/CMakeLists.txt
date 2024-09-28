set(TARGET pex_rng.test)

set(SOURCES
    test_pex_rng.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::testrunner
        ms::lib::pex_rng
        ms::lib::system_info
        ms::lib::register_headers
        silibs::crypto::rng
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    FpFwAssert
    rng_enable_r
)