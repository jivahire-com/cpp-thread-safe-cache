set(TARGET cli_ddr.test)

set(SOURCES
    test_cli_ddr.cpp
    ${CMAKE_SOURCE_DIR}/src/services/cli_ddr/cli_ddr.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_compile_definitions(${TARGET} PUBLIC UNIT_TEST)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.service.cli
        ms::cmocka::testrunner
        silibs::ddrss
        silibs::platform
        silibs::kng_silibs_platform
        silibs::ras_arm
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    FpFwCliRegisterTable
)
